#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "present_transport.h"
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

namespace nrpresentgpu {
namespace {
using Microsoft::WRL::ComPtr;
struct Global {std::mutex mutex;bool poisoned{};};

Global& global(){static auto* value=new Global;return *value;}
struct Work {
 ComPtr<ID3D12Device> device;
 ComPtr<ID3D12CommandQueue> queue;
 ComPtr<ID3D12Resource> image,readback,upload;
 ComPtr<ID3D12CommandAllocator> allocator;
 ComPtr<ID3D12GraphicsCommandList> list;
 ComPtr<ID3D12Fence> fence;
 HANDLE event{};bool in_flight{};UINT64 sequence{};
 ~Work(){if(event)CloseHandle(event);}
};
struct Owner {
 Work* work=new Work;
 ~Owner(){if(work->in_flight){global().poisoned=true; }else delete work;}
};
bool fail(std::string& error,const char* reason){error=reason;return false;}
bool same_device(ID3D12Device* expected,ID3D12DeviceChild* child){ComPtr<ID3D12Device> actual;if(FAILED(child->GetDevice(IID_PPV_ARGS(&actual))))return false;ComPtr<IUnknown> a,b;return SUCCEEDED(expected->QueryInterface(IID_PPV_ARGS(&a)))&&SUCCEEDED(actual.As(&b))&&a.Get()==b.Get();}
void barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* image,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){D3D12_RESOURCE_BARRIER value{};value.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;value.Transition={image,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&value);}
bool buffer(Work& w,UINT64 bytes,D3D12_HEAP_TYPE kind,ComPtr<ID3D12Resource>& result){D3D12_HEAP_PROPERTIES heap{};heap.Type=kind;heap.CreationNodeMask=heap.VisibleNodeMask=1;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;return SUCCEEDED(w.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,kind==D3D12_HEAP_TYPE_UPLOAD?D3D12_RESOURCE_STATE_GENERIC_READ:D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&result)));}
bool submit(Work& w,std::string& diagnostic){
 if(FAILED(w.list->Close()))return fail(diagnostic,"failed: command list close");
 ID3D12CommandList* lists[]{w.list.Get()};w.in_flight=true;w.queue->ExecuteCommandLists(1,lists);
 const UINT64 value=++w.sequence;if(FAILED(w.queue->Signal(w.fence.Get(),value)))return fail(diagnostic,"failed: queue fence signal; transport poisoned");
 auto completed=w.fence->GetCompletedValue();if(completed==UINT64_MAX)return fail(diagnostic,"failed: device removed during fence; transport poisoned");
 if(completed<value){
  if(FAILED(w.fence->SetEventOnCompletion(value,w.event)))return fail(diagnostic,"failed: fence event; transport poisoned");
  if(WaitForSingleObject(w.event,10000)!=WAIT_OBJECT_0)return fail(diagnostic,"failed: GPU retirement timeout; transport poisoned");
  completed=w.fence->GetCompletedValue();
 }
 if(completed==UINT64_MAX||completed<value)return fail(diagnostic,"failed: invalid GPU retirement; transport poisoned");
 w.in_flight=false;
 return SUCCEEDED(w.device->GetDeviceRemovedReason())||fail(diagnostic,"failed: device removed");
}
std::uint16_t normalized_half(unsigned value,unsigned maximum=255){

 if(!value)return 0;const float f=static_cast<float>(value)/static_cast<float>(maximum);std::uint32_t bits{};std::memcpy(&bits,&f,4);
 const auto exponent=((bits>>23)&255u)-112u;const auto mantissa=bits&0x7fffffu;
 const auto rounded=mantissa+0xfffu+((mantissa>>13)&1u);
 return static_cast<std::uint16_t>((exponent<<10)+(rounded>>13));
}
float half_float(std::uint16_t h){const unsigned e=(h>>10)&31u,m=h&1023u;float v{};if(!e)v=std::ldexp(static_cast<float>(m),-24);else if(e==31)v=m?NAN:INFINITY;else v=std::ldexp(static_cast<float>(1024u+m),static_cast<int>(e)-25);return h&0x8000u?-v:v;}
unsigned char quantize(float value){return static_cast<unsigned char>(std::floor(std::clamp(value,0.0f,1.0f)*255.0f+0.5f));}
bool run(void* opaque_swap,void* opaque_queue,Evaluate evaluate,void* context,std::string& diagnostic){
 if(!opaque_swap||!opaque_queue||!evaluate)return fail(diagnostic,"unsupported: missing swapchain, exact queue, or evaluator");
 if(global().poisoned)return fail(diagnostic,"failed: transport poisoned by unretired GPU submission; restart required");
 Owner owner;auto& w=*owner.work;ComPtr<IDXGISwapChain3> swap;
 if(FAILED(static_cast<IUnknown*>(opaque_swap)->QueryInterface(IID_PPV_ARGS(&swap)))||FAILED(static_cast<IUnknown*>(opaque_queue)->QueryInterface(IID_PPV_ARGS(&w.queue))))return fail(diagnostic,"unsupported: D3D12 swapchain/queue interfaces");
 if(w.queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)return fail(diagnostic,"unsupported: queue is not DIRECT");
 if(FAILED(swap->GetBuffer(swap->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&w.image)))||FAILED(w.image->GetDevice(IID_PPV_ARGS(&w.device))))return fail(diagnostic,"failed: current D3D12 backbuffer");
 if(!same_device(w.device.Get(),w.queue.Get()))return fail(diagnostic,"unsupported: swapchain and queue devices differ");
 const auto d=w.image->GetDesc();const bool bgra=d.Format==DXGI_FORMAT_B8G8R8A8_UNORM;const bool ten=d.Format==DXGI_FORMAT_R10G10B10A2_UNORM;
 if(d.Format!=DXGI_FORMAT_R8G8B8A8_UNORM&&!bgra&&!ten){diagnostic="unsupported: SDR Present format="+std::to_string(d.Format)+"; requires RGBA8/BGRA8/RGB10A2";return false;}
 if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||d.DepthOrArraySize!=1||d.MipLevels!=1||d.SampleDesc.Count!=1||!d.Width||!d.Height||d.Width>8192||d.Height>8192||d.Width*d.Height>33554432ull)return fail(diagnostic,"unsupported: backbuffer geometry");

 D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT rows{};UINT64 row_bytes{},total{};w.device->GetCopyableFootprints(&d,0,1,0,&fp,&rows,&row_bytes,&total);
 if(rows!=d.Height||row_bytes!=d.Width*4||fp.Footprint.RowPitch<row_bytes||!total||total>512ull*1024*1024)return fail(diagnostic,"unsupported: backbuffer copy footprint");
 const auto width=static_cast<unsigned>(d.Width),height=d.Height;const size_t pixels=size_t(width)*height;
 std::vector<std::uint16_t> input(pixels*4),output(pixels*4);std::vector<unsigned char> original(pixels*4),encoded(pixels*4);
 if(!buffer(w,total,D3D12_HEAP_TYPE_READBACK,w.readback)||!buffer(w,total,D3D12_HEAP_TYPE_UPLOAD,w.upload))return fail(diagnostic,"failed: staging allocation");
 if(FAILED(w.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&w.allocator)))||FAILED(w.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,w.allocator.Get(),nullptr,IID_PPV_ARGS(&w.list)))||FAILED(w.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&w.fence))))return fail(diagnostic,"failed: owned command/fence allocation");
 w.event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!w.event)return fail(diagnostic,"failed: owned event allocation");
 D3D12_TEXTURE_COPY_LOCATION image{},staging{};image.pResource=w.image.Get();image.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;staging.pResource=w.readback.Get();staging.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;staging.PlacedFootprint=fp;
 barrier(w.list.Get(),w.image.Get(),D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_SOURCE);w.list->CopyTextureRegion(&staging,0,0,0,&image,nullptr);barrier(w.list.Get(),w.image.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PRESENT);
 if(!submit(w,diagnostic))return false;
 void* mapped{};D3D12_RANGE read{0,static_cast<SIZE_T>(total)},none{0,0};if(FAILED(w.readback->Map(0,&read,&mapped)))return fail(diagnostic,"failed: readback map");
 for(unsigned y=0;y<height;++y)std::memcpy(original.data()+size_t(y)*width*4,static_cast<unsigned char*>(mapped)+fp.Offset+size_t(y)*fp.Footprint.RowPitch,size_t(width)*4);
 w.readback->Unmap(0,&none);
 for(size_t i=0;i<pixels;++i){
  if(ten){std::uint32_t packed{};std::memcpy(&packed,original.data()+i*4,4);for(unsigned c=0;c<3;++c)input[i*4+c]=normalized_half((packed>>(c*10))&1023u,1023);input[i*4+3]=normalized_half(packed>>30,3);}
  else {input[i*4]=normalized_half(original[i*4+(bgra?2:0)]);input[i*4+1]=normalized_half(original[i*4+1]);input[i*4+2]=normalized_half(original[i*4+(bgra?0:2)]);input[i*4+3]=normalized_half(original[i*4+3]);}
 }
 std::string eval_error;if(!evaluate(context,input.data(),output.data(),width,height,eval_error)){diagnostic="failed: evaluator: "+eval_error;return false;}
 for(size_t i=0;i<pixels;++i){const float r=half_float(output[i*4]),g=half_float(output[i*4+1]),b=half_float(output[i*4+2]);if(!std::isfinite(r)||!std::isfinite(g)||!std::isfinite(b))return fail(diagnostic,"failed: evaluator returned nonfinite RGB; original retained");if(ten){std::uint32_t packed{};std::memcpy(&packed,original.data()+i*4,4);packed&=0xc0000000u;const float rgb[]{r,g,b};for(unsigned c=0;c<3;++c)packed|=static_cast<std::uint32_t>(std::floor(std::clamp(rgb[c],0.0f,1.0f)*1023.0f+.5f))<<(c*10);std::memcpy(encoded.data()+i*4,&packed,4);}
 else {encoded[i*4+(bgra?2:0)]=quantize(r);encoded[i*4+1]=quantize(g);encoded[i*4+(bgra?0:2)]=quantize(b);encoded[i*4+3]=original[i*4+3];}}
 if(FAILED(w.upload->Map(0,&none,&mapped)))return fail(diagnostic,"failed: upload map");
 for(unsigned y=0;y<height;++y)std::memcpy(static_cast<unsigned char*>(mapped)+fp.Offset+size_t(y)*fp.Footprint.RowPitch,encoded.data()+size_t(y)*width*4,size_t(width)*4);
 w.upload->Unmap(0,&read);
 if(FAILED(w.allocator->Reset())||FAILED(w.list->Reset(w.allocator.Get(),nullptr)))return fail(diagnostic,"failed: retired allocator reset");
 staging.pResource=w.upload.Get();barrier(w.list.Get(),w.image.Get(),D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_DEST);w.list->CopyTextureRegion(&image,0,0,0,&staging,nullptr);barrier(w.list.Get(),w.image.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PRESENT);
 if(!submit(w,diagnostic))return false;
 size_t changed{};for(size_t i=0;i<pixels;++i)if(std::memcmp(original.data()+i*4,encoded.data()+i*4,4))++changed;
 diagnostic="active: SDR stateless Present copyback retired; width="+std::to_string(width)+" height="+std::to_string(height)+" format="+std::to_string(d.Format)+" changed_pixels="+std::to_string(changed);return true;
}
}
bool process(void* swapchain,void* queue,Evaluate evaluate,void* context,std::string& diagnostic){
 std::lock_guard<std::mutex> lock(global().mutex);
 try{return run(swapchain,queue,evaluate,context,diagnostic);}catch(const std::exception& e){diagnostic=std::string("failed: Present exception: ")+e.what();return false;}catch(...){diagnostic="failed: Present unknown exception";return false;}
}
}
