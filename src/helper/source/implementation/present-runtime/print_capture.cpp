#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../../../runtime/nr_publico.h"
#include "print_capture.h"
#include "../../../runtime/expo_trava.h"
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
template<class... A> void log(const std::wstring&,const char*,A&&...) noexcept {}
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

void fail_tela(const std::wstring& folder,unsigned quadros,const char* reason,const std::string& detail=std::string()) noexcept {
    tela_falhas.fetch_add(quadros,std::memory_order_relaxed);
    log(folder,"tela failed: %s%s%s quadros=%u",reason,detail.empty()?"":" ",detail.c_str(),quadros);
}

constexpr unsigned tela_max_voo=256;

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

    std::vector<TelaArmado> armados;std::wstring tela_pasta;UINT64 contado{};
    std::atomic<unsigned>& voo() noexcept {return armados.empty()?salvando:tela_salvando;}
    void soltar_readback() noexcept {
        readback.Reset();
        if(contado){if(auto f=tela_conta.load(std::memory_order_acquire))f(false,contado);contado=0;}
    }
    Job()=default;Job(const Job&)=delete;Job& operator=(const Job&)=delete;
    ~Job(){soltar_readback();}
};

bool gravar_tela(const Job& job,const TelaArmado& a,std::uint64_t quadro_imagem,const std::vector<unsigned char>& rgba,double copy_ms) noexcept {
    try{
        wchar_t base[96]{};std::swprintf(base,96,L"nr_final_%lu_%u_tela",GetCurrentProcessId(),a.indice);
        const std::filesystem::path pasta(job.tela_pasta);
        const auto raw=(job.tela_pasta.empty()?std::filesystem::path(std::wstring(base)+L".raw"):pasta/(std::wstring(base)+L".raw"));
        const auto json=(job.tela_pasta.empty()?std::filesystem::path(std::wstring(base)+L".json"):pasta/(std::wstring(base)+L".json"));
        FILE* f{};
        if(_wfopen_s(&f,raw.c_str(),L"wb")!=0||!f)return false;
        const bool inteiro=std::fwrite(rgba.data(),1,rgba.size(),f)==rgba.size();std::fclose(f);
        if(!inteiro)return false;
        if(_wfopen_s(&f,json.c_str(),L"w")!=0||!f)return false;

        std::fprintf(f,"{\"format\":%u,\"width\":%u,\"height\":%u,\"row_pitch\":%u,\"largura\":%u,\"altura\":%u,\"formato\":\"rgba8_srgb\","
                       "\"formato_backbuffer\":%u,\"index\":%u,\"quadro_ffx\":%llu,\"quadro_armado\":%llu,\"copia_ms\":%.2f%s}\n",
                     unsigned(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB),job.width,job.height,job.width*4u,job.width,job.height,job.format,a.indice,
                     static_cast<unsigned long long>(quadro_imagem),static_cast<unsigned long long>(a.quadro_ffx),copy_ms,
                     job.kind==Kind::Rgba16f?",\"nota\":\"hdr_scrgb_linear_cortado_no_branco_sdr\"":job.kind==Kind::Rgb10a2?",\"nota\":\"rgb10a2_lido_como_sdr\"":"");
        std::fclose(f);return true;
    }catch(...){return false;}
}
void finish(Job* raw) noexcept {
    std::unique_ptr<Job> job(raw);
    bool retired=false;
    std::atomic<unsigned>& voo=job->voo();
    const bool tela=!job->armados.empty();
    const auto falha=[&](const char* reason,const std::string& detail=std::string()){
        if(tela)fail_tela(job?job->folder:std::wstring(),unsigned(job?job->armados.size():0u),reason,detail);else fail(job?job->folder:std::wstring(),reason,detail);};
    try{
        HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        if(!event){falha("event");job.release();voo.fetch_sub(1);return;}
        if(job->fence->GetCompletedValue()<1){
            if(SUCCEEDED(job->fence->SetEventOnCompletion(1,event)))WaitForSingleObject(event,10000);
        }
        CloseHandle(event);
        const UINT64 done=job->fence->GetCompletedValue();
        if(done==UINT64_MAX||done<1){

            falha(done==UINT64_MAX?"device_removed":"gpu_timeout_10s");job.release();voo.fetch_sub(1);return;
        }
        retired=true;
        const double copy_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-job->start).count();
        std::vector<unsigned char> rgba(std::size_t(job->width)*job->height*4);
        void* mapped{};D3D12_RANGE read{0,SIZE_T(job->total)},none{0,0};
        if(FAILED(job->readback->Map(0,&read,&mapped))||!mapped){falha("readback_map");voo.fetch_sub(1);return;}
        for(unsigned y=0;y<job->height;++y)
            convert_row(job->kind,static_cast<unsigned char*>(mapped)+job->footprint.Offset+std::size_t(y)*job->footprint.Footprint.RowPitch,
                        job->width,rgba.data()+std::size_t(y)*job->width*4);
        job->readback->Unmap(0,&none);

        job->list.Reset();job->allocator.Reset();job->soltar_readback();job->fence.Reset();job->device.Reset();
        if(tela){

            const std::uint64_t quadro_imagem=job->armados.back().quadro_ffx;unsigned ok_n=0;
            for(const auto& a:job->armados)if(gravar_tela(*job,a,quadro_imagem,rgba,copy_ms))++ok_n;
            tela_gravados.fetch_add(ok_n);
            if(ok_n!=job->armados.size())fail_tela(job->folder,unsigned(job->armados.size())-ok_n,"write");
            voo.fetch_sub(1);return;
        }
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
        if(tela)fail_tela(job?job->folder:std::wstring(),unsigned(job?job->armados.size():0u),"worker_exception");else fail(job?job->folder:std::wstring(),"worker_exception");
    }
    voo.fetch_sub(1);
}
std::wstring game_folder(){
    std::wstring text(32768,L'\0');
    const DWORD n=GetModuleFileNameW(nullptr,text.data(),DWORD(text.size()));
    if(!n||n>=text.size())return std::wstring();
    text.resize(n);
    return std::filesystem::path(text).parent_path().wstring();
}

void start(void* opaque_swap,void* opaque_queue,const std::wstring& folder,const TelaArmado* tela=nullptr,unsigned n=0,const std::wstring& tela_pasta=std::wstring()){
    const auto falha=[&](const char* reason,const std::string& detail=std::string()){if(tela)fail_tela(folder,n,reason,detail);else fail(folder,reason,detail);};
    if(tela?tela_salvando.load()>=tela_max_voo:salvando.load()>=max_in_flight){falha("busy");return;}
    if(!opaque_swap||!opaque_queue){falha("missing_swapchain_or_queue");return;}
    ComPtr<IDXGISwapChain3> swap;ComPtr<ID3D12CommandQueue> queue;
    if(FAILED(static_cast<IUnknown*>(opaque_swap)->QueryInterface(IID_PPV_ARGS(&swap)))){falha("not_dxgi_swapchain3");return;}
    if(FAILED(static_cast<IUnknown*>(opaque_queue)->QueryInterface(IID_PPV_ARGS(&queue)))){falha("not_d3d12_queue");return;}
    auto job=std::make_unique<Job>();
    if(tela){job->armados.assign(tela,tela+n);job->tela_pasta=tela_pasta;}
    ComPtr<ID3D12Resource> image;
    if(FAILED(swap->GetBuffer(swap->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&image)))||!image){falha("backbuffer");return;}
    if(FAILED(image->GetDevice(IID_PPV_ARGS(&job->device)))){falha("device");return;}
    {
        ComPtr<ID3D12Device> other;ComPtr<IUnknown> a,b;
        if(FAILED(queue->GetDevice(IID_PPV_ARGS(&other)))||FAILED(job->device.As(&a))||FAILED(other.As(&b))||a.Get()!=b.Get()){falha("queue_on_other_device");return;}
    }
    const auto d=image->GetDesc();
    job->format=unsigned(d.Format);job->kind=kind_of(job->format);
    if(job->kind==Kind::Unsupported){falha("unsupported_format",std::to_string(job->format));return;}
    if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||d.DepthOrArraySize!=1||d.SampleDesc.Count!=1||!d.Width||!d.Height||d.Width>16384||d.Height>16384){
        falha("backbuffer_geometry");return;}
    job->width=unsigned(d.Width);job->height=d.Height;
    UINT rows{};UINT64 row_bytes{};
    job->device->GetCopyableFootprints(&d,0,1,0,&job->footprint,&rows,&row_bytes,&job->total);
    if(rows!=d.Height||row_bytes<UINT64(d.Width)*bytes_per_pixel(job->kind)||!job->total||job->total>1024ull*1024*1024){falha("footprint");return;}
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;heap.CreationNodeMask=heap.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=job->total;bd.Height=1;bd.DepthOrArraySize=bd.MipLevels=1;
    bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if(FAILED(job->device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&job->readback)))){falha("readback_allocation");return;}

    if(tela)if(auto f=tela_conta.load(std::memory_order_acquire)){f(true,job->total);job->contado=job->total;}
    const auto type=queue->GetDesc().Type;
    if(FAILED(job->device->CreateCommandAllocator(type,IID_PPV_ARGS(&job->allocator)))||
       FAILED(job->device->CreateCommandList(0,type,job->allocator.Get(),nullptr,IID_PPV_ARGS(&job->list)))||
       FAILED(job->device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&job->fence)))){falha("command_objects");return;}

    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={image.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_SOURCE};
    job->list->ResourceBarrier(1,&barrier);
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};
    src.pResource=image.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.SubresourceIndex=0;
    dst.pResource=job->readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=job->footprint;
    job->list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
    std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);
    job->list->ResourceBarrier(1,&barrier);
    if(FAILED(job->list->Close())){falha("list_close");return;}
    job->folder=folder;
    if(!tela){SYSTEMTIME now{};GetLocalTime(&now);job->path=(std::filesystem::path(folder)/L"nr_prints"/file_name(now)).wstring();}
    job->start=std::chrono::steady_clock::now();
    std::atomic<unsigned>& voo=job->voo();
    ID3D12CommandList* lists[]{job->list.Get()};
    queue->ExecuteCommandLists(1,lists);
    voo.fetch_add(1);
    if(FAILED(queue->Signal(job->fence.Get(),1))){

        (void)job.release();voo.fetch_sub(1);falha("queue_signal");return;
    }
    Job* raw=job.release();
    try{std::thread(finish,raw).detach();}
    catch(...){voo.fetch_sub(1);falha("thread"); }
}

void capture_em(void* swapchain,void* queue,const std::wstring& log_folder,const std::wstring& tela_pasta) noexcept {
    if(pedido.load(std::memory_order_relaxed)!=0&&pedido.exchange(0)!=0){
        try{start(swapchain,queue,log_folder);}
        catch(const std::exception& e){fail(log_folder,"exception",e.what());}
        catch(...){fail(log_folder,"exception");}
    }
    if(tela_pendentes.load(std::memory_order_acquire)!=0){
        TelaArmado armados[tela_cap];const unsigned n=tomar_armados(armados,tela_cap);
        if(n){
            try{start(swapchain,queue,log_folder,armados,n,tela_pasta);}
            catch(const std::exception& e){fail_tela(log_folder,n,"exception",e.what());}
            catch(...){fail_tela(log_folder,n,"exception");}
        }
    }
}
}

namespace {
struct TelaEstado {
    ComPtr<ID3D12Device> device;ComPtr<ID3D12Resource> rb;ComPtr<ID3D12CommandAllocator> alloc;ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> fence;UINT64 valor{};D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 total{};Kind kind{};unsigned w{},h{},fmt{};
    D3D12_COMMAND_LIST_TYPE tipo{};std::atomic<int> ocupado{0};

    std::atomic<ULONGLONG> ult{0};std::atomic<bool> falhou{false};
    HANDLE trabalho{};
};
TelaEstado& tela(){static auto* s=new TelaEstado;return *s;}
void tela_calcular() noexcept {
    auto& s=tela();
    try{
        if(s.fence->GetCompletedValue()<s.valor){
            HANDLE ev=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!ev){s.ocupado.store(0);return;}
            if(SUCCEEDED(s.fence->SetEventOnCompletion(s.valor,ev)))WaitForSingleObject(ev,10000);CloseHandle(ev);}
        const UINT64 feito=s.fence->GetCompletedValue();
        if(feito==UINT64_MAX||feito<s.valor){s.falhou.store(true);return;}
        void* m{};D3D12_RANGE r{0,SIZE_T(s.total)},nada{0,0};
        if(FAILED(s.rb->Map(0,&r,&m))||!m){s.ocupado.store(0);return;}

        const std::size_t bpp=bytes_per_pixel(s.kind);double soma=0.;unsigned long long n=0;unsigned char p[4]{};
        for(unsigned y=4;y<s.h;y+=8){
            const auto* linha=static_cast<const unsigned char*>(m)+s.fp.Offset+std::size_t(y)*s.fp.Footprint.RowPitch;
            for(unsigned x=4;x<s.w;x+=8){convert_row(s.kind,linha+std::size_t(x)*bpp,1,p);soma+=(.2126*p[0]+.7152*p[1]+.0722*p[2])/255.;++n;}}
        s.rb->Unmap(0,&nada);
        if(n>0){nrexpotrava::tela_media.store(float(soma/double(n)),std::memory_order_relaxed);nrexpotrava::tela_ms.store(GetTickCount64(),std::memory_order_relaxed);}
    }catch(...){}
    s.ocupado.store(0,std::memory_order_release);
}

bool tela_disparar() noexcept {
    auto& s=tela();
    try{
        static std::once_flag uma;
        std::call_once(uma,[&s]{
            HANDLE ev=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!ev)return;
            try{std::thread([ev]{for(;;){if(WaitForSingleObject(ev,INFINITE)!=WAIT_OBJECT_0)return;tela_calcular();}}).detach();}
            catch(...){CloseHandle(ev);return;}
            s.trabalho=ev;
        });
    }catch(...){return false;}
    return s.trabalho&&SetEvent(s.trabalho)!=FALSE;
}
}
void medir_tela(void* opaque_swap,void* opaque_queue) noexcept {
    if(!nrexpotrava::tela_ligada.load(std::memory_order_relaxed)||!opaque_swap||!opaque_queue)return;
    auto& s=tela();if(s.falhou.load(std::memory_order_relaxed)||s.ocupado.load(std::memory_order_acquire))return;
    const ULONGLONG agora=GetTickCount64();if(agora-s.ult.load(std::memory_order_relaxed)<200)return;s.ult.store(agora,std::memory_order_relaxed);
    try{
        ComPtr<IDXGISwapChain3> swap;ComPtr<ID3D12CommandQueue> queue;ComPtr<ID3D12Resource> image;ComPtr<ID3D12Device> dev;
        if(FAILED(static_cast<IUnknown*>(opaque_swap)->QueryInterface(IID_PPV_ARGS(&swap)))||FAILED(static_cast<IUnknown*>(opaque_queue)->QueryInterface(IID_PPV_ARGS(&queue))))return;
        if(FAILED(swap->GetBuffer(swap->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&image)))||!image||FAILED(image->GetDevice(IID_PPV_ARGS(&dev))))return;
        {ComPtr<ID3D12Device> outro;ComPtr<IUnknown> a,b;if(FAILED(queue->GetDevice(IID_PPV_ARGS(&outro)))||FAILED(dev.As(&a))||FAILED(outro.As(&b))||a.Get()!=b.Get())return;}
        const auto d=image->GetDesc();const Kind k=kind_of(unsigned(d.Format));const auto tipo=queue->GetDesc().Type;
        if(k==Kind::Unsupported||d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||d.DepthOrArraySize!=1||d.SampleDesc.Count!=1||!d.Width||!d.Height||d.Width>16384||d.Height>16384)return;
        if(s.device.Get()!=dev.Get()||s.w!=unsigned(d.Width)||s.h!=d.Height||s.fmt!=unsigned(d.Format)||s.tipo!=tipo||!s.rb){
            s.rb.Reset();s.alloc.Reset();s.list.Reset();s.fence.Reset();s.device=dev;s.valor=0;
            UINT rows{};UINT64 rb{};dev->GetCopyableFootprints(&d,0,1,0,&s.fp,&rows,&rb,&s.total);
            if(rows!=d.Height||!s.total||s.total>1024ull*1024*1024){s.falhou.store(true);return;}
            D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;hp.CreationNodeMask=hp.VisibleNodeMask=1;
            D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=s.total;bd.Height=1;bd.DepthOrArraySize=bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            if(FAILED(dev->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&s.rb)))||
               FAILED(dev->CreateCommandAllocator(tipo,IID_PPV_ARGS(&s.alloc)))||
               FAILED(dev->CreateCommandList(0,tipo,s.alloc.Get(),nullptr,IID_PPV_ARGS(&s.list)))||FAILED(s.list->Close())||
               FAILED(dev->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&s.fence)))){s.falhou.store(true);return;}
            s.kind=k;s.w=unsigned(d.Width);s.h=d.Height;s.fmt=unsigned(d.Format);s.tipo=tipo;}
        if(FAILED(s.alloc->Reset())||FAILED(s.list->Reset(s.alloc.Get(),nullptr)))return;
        D3D12_RESOURCE_BARRIER bar{};bar.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        bar.Transition={image.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_SOURCE};
        s.list->ResourceBarrier(1,&bar);
        D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=image.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.SubresourceIndex=0;
        dst.pResource=s.rb.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=s.fp;
        s.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
        std::swap(bar.Transition.StateBefore,bar.Transition.StateAfter);s.list->ResourceBarrier(1,&bar);
        if(FAILED(s.list->Close()))return;
        ID3D12CommandList* ls[]{s.list.Get()};queue->ExecuteCommandLists(1,ls);
        s.ocupado.store(1,std::memory_order_release);
        if(FAILED(queue->Signal(s.fence.Get(),++s.valor))){s.falhou.store(true);return;}
        if(!tela_disparar())s.falhou.store(true);
    }catch(...){}
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
    while(salvando.load()!=0||tela_salvando.load()!=0){if(GetTickCount64()>end)return false;Sleep(5);}
    return true;
}
}
