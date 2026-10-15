#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../../../runtime/nr_publico.h"
#include "print_capture.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace nrprint {
namespace {
using Microsoft::WRL::ComPtr;
constexpr unsigned max_in_flight=3;
std::mutex& log_mutex(){static auto* m=new std::mutex;return *m;}
void log(const std::wstring& folder,const char* format,...) noexcept {
    try{
        char text[768]{};va_list args;va_start(args,format);std::vsnprintf(text,sizeof text,format,args);va_end(args);
        SYSTEMTIME t{};GetLocalTime(&t);
        std::lock_guard<std::mutex> lock(log_mutex());
        FILE* f{};
        if(_wfopen_s(&f,(std::filesystem::path(folder)/L"nr_print.log").c_str(),L"a")==0&&f){
            std::fprintf(f,"%04u-%02u-%02u %02u:%02u:%02u.%03u pid=%lu %s\n",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,
                         t.wMilliseconds,GetCurrentProcessId(),text);
            std::fclose(f);
        }
    }catch(...){}
}
std::string narrow(const std::wstring& w){
    std::string out;
    const int n=WideCharToMultiByte(CP_UTF8,0,w.c_str(),int(w.size()),nullptr,0,nullptr,nullptr);
    if(n>0){out.resize(std::size_t(n));WideCharToMultiByte(CP_UTF8,0,w.c_str(),int(w.size()),out.data(),n,nullptr,nullptr);}
    return out;
}
float half_float(std::uint16_t h) noexcept {
    const unsigned e=(h>>10)&31u,m=h&1023u;float v;
    if(!e)v=std::ldexp(float(m),-24);else if(e==31)v=m?NAN:INFINITY;else v=std::ldexp(float(1024u+m),int(e)-25);
    return (h&0x8000u)?-v:v;
}
void fail(const std::wstring& folder,const char* reason,const std::string& detail=std::string()) noexcept {
    ultima_falhou.store(1);
    log(folder,"print failed: %s%s%s",reason,detail.empty()?"":" ",detail.c_str());
}

struct Job {
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12Resource> readback;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> fence;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT64 total{};
    Kind kind{};
    unsigned format{},width{},height{};
    std::wstring folder,path;
    std::chrono::steady_clock::time_point start{};
};
void finish(Job* raw) noexcept {
    std::unique_ptr<Job> job(raw);
    bool retired=false;
    try{
        HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        if(!event){fail(job->folder,"event");job.release();salvando.fetch_sub(1);return;}
        if(job->fence->GetCompletedValue()<1){
            if(SUCCEEDED(job->fence->SetEventOnCompletion(1,event)))WaitForSingleObject(event,10000);
        }
        CloseHandle(event);
        const UINT64 done=job->fence->GetCompletedValue();
        if(done==UINT64_MAX||done<1){

            fail(job->folder,done==UINT64_MAX?"device_removed":"gpu_timeout_10s");job.release();salvando.fetch_sub(1);return;
        }
        retired=true;
        const double copy_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-job->start).count();
        std::vector<unsigned char> rgba(std::size_t(job->width)*job->height*4);
        void* mapped{};D3D12_RANGE read{0,SIZE_T(job->total)},none{0,0};
        if(FAILED(job->readback->Map(0,&read,&mapped))||!mapped){fail(job->folder,"readback_map");salvando.fetch_sub(1);return;}
        for(unsigned y=0;y<job->height;++y)
            convert_row(job->kind,static_cast<unsigned char*>(mapped)+job->footprint.Offset+std::size_t(y)*job->footprint.Footprint.RowPitch,
                        job->width,rgba.data()+std::size_t(y)*job->width*4);
        job->readback->Unmap(0,&none);

        job->list.Reset();job->allocator.Reset();job->readback.Reset();job->fence.Reset();job->device.Reset();
        std::error_code ec;std::filesystem::create_directories(std::filesystem::path(job->path).parent_path(),ec);
        std::string error;
        if(!encode_png(job->path,rgba.data(),job->width,job->height,error)){
            std::filesystem::remove(job->path,ec);fail(job->folder,"png",error);salvando.fetch_sub(1);return;
        }
        const double total_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-job->start).count();
        salvos.fetch_add(1);ultima_falhou.store(0);
        log(job->folder,"print ok path=%s size=%ux%u format=%u copy_ms=%.1f total_ms=%.1f%s",narrow(job->path).c_str(),job->width,job->height,
            job->format,copy_ms,total_ms,
            job->kind==Kind::Rgba16f?" note=hdr_scrgb_linear_clamped_to_sdr_white":
            job->kind==Kind::Rgb10a2?" note=rgb10a2_read_as_sdr":"");
    }catch(...){
        if(!retired)job.release();
        fail(job?job->folder:std::wstring(),"worker_exception");
    }
    salvando.fetch_sub(1);
}
std::wstring game_folder(){
    std::wstring text(32768,L'\0');
    const DWORD n=GetModuleFileNameW(nullptr,text.data(),DWORD(text.size()));
    if(!n||n>=text.size())return std::wstring();
    text.resize(n);
    return std::filesystem::path(text).parent_path().wstring();
}
void start(void* opaque_swap,void* opaque_queue,const std::wstring& folder){
    if(salvando.load()>=max_in_flight){fail(folder,"busy");return;}
    if(!opaque_swap||!opaque_queue){fail(folder,"missing_swapchain_or_queue");return;}
    ComPtr<IDXGISwapChain3> swap;ComPtr<ID3D12CommandQueue> queue;
    if(FAILED(static_cast<IUnknown*>(opaque_swap)->QueryInterface(IID_PPV_ARGS(&swap)))){fail(folder,"not_dxgi_swapchain3");return;}
    if(FAILED(static_cast<IUnknown*>(opaque_queue)->QueryInterface(IID_PPV_ARGS(&queue)))){fail(folder,"not_d3d12_queue");return;}
    auto job=std::make_unique<Job>();
    ComPtr<ID3D12Resource> image;
    if(FAILED(swap->GetBuffer(swap->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&image)))||!image){fail(folder,"backbuffer");return;}
    if(FAILED(image->GetDevice(IID_PPV_ARGS(&job->device)))){fail(folder,"device");return;}
    {
        ComPtr<ID3D12Device> other;ComPtr<IUnknown> a,b;
        if(FAILED(queue->GetDevice(IID_PPV_ARGS(&other)))||FAILED(job->device.As(&a))||FAILED(other.As(&b))||a.Get()!=b.Get()){fail(folder,"queue_on_other_device");return;}
    }
    const auto d=image->GetDesc();
    job->format=unsigned(d.Format);job->kind=kind_of(job->format);
    if(job->kind==Kind::Unsupported){fail(folder,"unsupported_format",std::to_string(job->format));return;}
    if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||d.DepthOrArraySize!=1||d.SampleDesc.Count!=1||!d.Width||!d.Height||d.Width>16384||d.Height>16384){
        fail(folder,"backbuffer_geometry");return;}
    job->width=unsigned(d.Width);job->height=d.Height;
    UINT rows{};UINT64 row_bytes{};
    job->device->GetCopyableFootprints(&d,0,1,0,&job->footprint,&rows,&row_bytes,&job->total);
    if(rows!=d.Height||row_bytes<UINT64(d.Width)*bytes_per_pixel(job->kind)||!job->total||job->total>1024ull*1024*1024){fail(folder,"footprint");return;}
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;heap.CreationNodeMask=heap.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=job->total;bd.Height=1;bd.DepthOrArraySize=bd.MipLevels=1;
    bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if(FAILED(job->device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&job->readback)))){fail(folder,"readback_allocation");return;}
    const auto type=queue->GetDesc().Type;
    if(FAILED(job->device->CreateCommandAllocator(type,IID_PPV_ARGS(&job->allocator)))||
       FAILED(job->device->CreateCommandList(0,type,job->allocator.Get(),nullptr,IID_PPV_ARGS(&job->list)))||
       FAILED(job->device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&job->fence)))){fail(folder,"command_objects");return;}

    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={image.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_SOURCE};
    job->list->ResourceBarrier(1,&barrier);
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};
    src.pResource=image.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.SubresourceIndex=0;
    dst.pResource=job->readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=job->footprint;
    job->list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
    std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);
    job->list->ResourceBarrier(1,&barrier);
    if(FAILED(job->list->Close())){fail(folder,"list_close");return;}
    SYSTEMTIME now{};GetLocalTime(&now);
    job->folder=folder;job->path=(std::filesystem::path(folder)/L"nr_prints"/file_name(now)).wstring();
    job->start=std::chrono::steady_clock::now();
    ID3D12CommandList* lists[]{job->list.Get()};
    queue->ExecuteCommandLists(1,lists);
    salvando.fetch_add(1);
    if(FAILED(queue->Signal(job->fence.Get(),1))){

        (void)job.release();salvando.fetch_sub(1);fail(folder,"queue_signal");return;
    }
    Job* raw=job.release();
    try{std::thread(finish,raw).detach();}
    catch(...){salvando.fetch_sub(1);fail(folder,"thread"); }
}
}
Kind kind_of(unsigned f) noexcept {
    switch(DXGI_FORMAT(f)){
    case DXGI_FORMAT_R8G8B8A8_UNORM:case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:return Kind::Rgba8;
    case DXGI_FORMAT_B8G8R8A8_UNORM:case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:return Kind::Bgra8;
    case DXGI_FORMAT_R10G10B10A2_UNORM:return Kind::Rgb10a2;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:return Kind::Rgba16f;
    default:return Kind::Unsupported;
    }
}
unsigned bytes_per_pixel(Kind k) noexcept {return k==Kind::Rgba16f?8u:k==Kind::Unsupported?0u:4u;}
unsigned char linear_half_to_srgb8(std::uint16_t h) noexcept {
    float v=half_float(h);
    if(!(v>0.f))return 0;
    if(v>=1.f)return 255;
    const float s=v<=.0031308f?12.92f*v:1.055f*std::pow(v,1.f/2.4f)-.055f;
    return static_cast<unsigned char>(std::clamp(s,0.f,1.f)*255.f+.5f);
}
void convert_row(Kind kind,const void* source,unsigned width,unsigned char* out) noexcept {
    const auto* in=static_cast<const unsigned char*>(source);
    for(unsigned x=0;x<width;++x,out+=4){
        switch(kind){
        case Kind::Rgba8:out[0]=in[x*4];out[1]=in[x*4+1];out[2]=in[x*4+2];break;
        case Kind::Bgra8:out[0]=in[x*4+2];out[1]=in[x*4+1];out[2]=in[x*4];break;
        case Kind::Rgb10a2:{std::uint32_t p;std::memcpy(&p,in+x*4,4);
            for(unsigned c=0;c<3;++c)out[c]=static_cast<unsigned char>((((p>>(10*c))&1023u)*255u+511u)/1023u);break;}
        case Kind::Rgba16f:{std::uint16_t h[4];std::memcpy(h,in+std::size_t(x)*8,8);
            for(unsigned c=0;c<3;++c)out[c]=linear_half_to_srgb8(h[c]);break;}
        default:out[0]=out[1]=out[2]=0;break;
        }
        out[3]=255;
    }
}
std::wstring file_name(const SYSTEMTIME& t) {
    wchar_t text[64]{};
    std::swprintf(text,64,L"print_%04u%02u%02u_%02u%02u%02u_%03u.png",unsigned(t.wYear),unsigned(t.wMonth),unsigned(t.wDay),
                  unsigned(t.wHour),unsigned(t.wMinute),unsigned(t.wSecond),unsigned(t.wMilliseconds));
    return text;
}
bool encode_png(const std::wstring& path,const unsigned char* rgba,unsigned width,unsigned height,std::string& error) noexcept {
    try{
        const HRESULT co=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
        bool ok=false;
        {
            auto step=[&](HRESULT hr,const char* what){if(FAILED(hr)){char t[96];std::snprintf(t,sizeof t,"%s hr=0x%08lx",what,static_cast<unsigned long>(hr));error=t;}return SUCCEEDED(hr);};
            ComPtr<IWICImagingFactory> factory;ComPtr<IWICStream> stream;ComPtr<IWICBitmapEncoder> encoder;
            ComPtr<IWICBitmapFrameEncode> frame;ComPtr<IPropertyBag2> properties;
            WICPixelFormatGUID format=GUID_WICPixelFormat24bppBGR;
            std::vector<unsigned char> bgr(std::size_t(width)*height*3);
            for(std::size_t i=0,n=std::size_t(width)*height;i<n;++i){bgr[i*3]=rgba[i*4+2];bgr[i*3+1]=rgba[i*4+1];bgr[i*3+2]=rgba[i*4];}
            ok=step(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)),"wic_factory")&&
               step(factory->CreateStream(&stream),"stream")&&
               step(stream->InitializeFromFilename(path.c_str(),GENERIC_WRITE),"open_file")&&
               step(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder),"png_encoder")&&
               step(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache),"encoder_init")&&
               step(encoder->CreateNewFrame(&frame,&properties),"frame")&&
               step(frame->Initialize(properties.Get()),"frame_init")&&
               step(frame->SetSize(width,height),"size")&&
               step(frame->SetPixelFormat(&format),"pixel_format");
            if(ok&&format!=GUID_WICPixelFormat24bppBGR){error="pixel_format_negotiated";ok=false;}
            ok=ok&&step(frame->WritePixels(height,width*3,UINT(bgr.size()),bgr.data()),"write")&&
               step(frame->Commit(),"frame_commit")&&step(encoder->Commit(),"encoder_commit");
        }
        if(SUCCEEDED(co))CoUninitialize();
        return ok;
    }catch(...){error="exception";return false;}
}
void capture_to(void* swapchain,void* queue,const std::wstring& folder) noexcept {
 (void)swapchain;(void)queue;(void)folder;
}
void capture(void* swapchain,void* queue) noexcept {
 (void)swapchain;(void)queue;
}
bool wait_idle(unsigned milliseconds) noexcept {
    const auto end=GetTickCount64()+milliseconds;
    while(salvando.load()!=0){if(GetTickCount64()>end)return false;Sleep(5);}
    return true;
}
}
