#include "../../../runtime/nr_publico.h"
#include "d3d12_backend.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <vector>

namespace nrscale {
namespace {
using Microsoft::WRL::ComPtr;
struct Set {
  ComPtr<ID3D12Device> device;
  std::array<ComPtr<ID3D12Resource>,4> textures;
  std::array<D3D12_RESOURCE_STATES,4> internal_states{};
  std::array<std::uint32_t,4> ffx_states{};
  ComPtr<ID3D12DescriptorHeap> heap;
  ComPtr<ID3D12RootSignature> root;
  std::array<ComPtr<ID3D12PipelineState>,3> pipelines;
  UINT stride{};
  Geometry geometry{};
  void* originals[4]{};
  bool valid{};
};
std::filesystem::path module_directory() {
  HMODULE module{};wchar_t path[32768]{};
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&make_d3d12_backend),&module))return {};
  const auto size=GetModuleFileNameW(module,path,32768);if(!size||size>=32768)return {};
  return std::filesystem::path(path).parent_path();
}
std::vector<char> bytes(const std::filesystem::path& path){
  std::ifstream stream(path,std::ios::binary|std::ios::ate);if(!stream)return {};
  const auto size=stream.tellg();if(size<=0||size>1024*1024)return {};std::vector<char> result(static_cast<std::size_t>(size));
  stream.seekg(0);stream.read(result.data(),size);if(!stream)return {};return result;
}
D3D12_RESOURCE_STATES state(std::uint32_t value) noexcept{
  switch(value){
    case 1:return D3D12_RESOURCE_STATE_COMMON;
    case 2:return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case 4:return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case 8:return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case 12:return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case 16:return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case 32:return D3D12_RESOURCE_STATE_COPY_DEST;
    case 20:return D3D12_RESOURCE_STATE_COPY_SOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    default:return static_cast<D3D12_RESOURCE_STATES>(UINT_MAX);
  }
}
void barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after) noexcept{
  if(before==after)return;D3D12_RESOURCE_BARRIER value{};value.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;value.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&value);
}
void uav(ID3D12GraphicsCommandList* list,ID3D12Resource* resource) noexcept{
  D3D12_RESOURCE_BARRIER value{};value.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;value.UAV.pResource=resource;list->ResourceBarrier(1,&value);
}
bool same_device(ID3D12Device* expected,ID3D12DeviceChild* child) noexcept{
  if(!expected||!child)return false;ComPtr<ID3D12Device> actual;
  return SUCCEEDED(child->GetDevice(IID_PPV_ARGS(&actual)))&&actual.Get()==expected;
}
bool original_ok(const nrffx::Descriptor& d,ComPtr<ID3D12Device>& device) noexcept{
  auto* input=static_cast<ID3D12Resource*>(d.input);auto* output=static_cast<ID3D12Resource*>(d.output);
  auto* motion=static_cast<ID3D12Resource*>(d.motion);auto* depth=static_cast<ID3D12Resource*>(d.depth);
  if(!input||!output||!motion||!depth||FAILED(input->GetDevice(IID_PPV_ARGS(&device))))return false;
  if(!same_device(device.Get(),output)||!same_device(device.Get(),motion)||!same_device(device.Get(),depth))return false;
  const auto i=input->GetDesc(),o=output->GetDesc(),m=motion->GetDesc(),z=depth->GetDesc();
  const auto texture=[](const D3D12_RESOURCE_DESC& x,DXGI_FORMAT f,unsigned w,unsigned h,bool write){return x.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&x.Width==w&&x.Height==h&&x.DepthOrArraySize==1&&x.MipLevels==1&&x.Format==f&&x.SampleDesc.Count==1&&(!write||(x.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))&&!(x.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);};
  return texture(i,DXGI_FORMAT_R16G16B16A16_FLOAT,d.render_w,d.render_h,true)&&texture(o,DXGI_FORMAT_R16G16B16A16_FLOAT,d.output_w,d.output_h,true)&&
    (m.Format==DXGI_FORMAT_R16G16_FLOAT||m.Format==DXGI_FORMAT_R16G16B16A16_FLOAT)&&texture(m,m.Format,d.render_w,d.render_h,false)&&(z.Format==DXGI_FORMAT_R32_TYPELESS||z.Format==DXGI_FORMAT_R32G8X24_TYPELESS)&&texture(z,z.Format,d.render_w,d.render_h,false);
}
class Backend final:public GpuBackend {
  Set active_{},candidate_{};
  static bool create_texture(ID3D12Device* device,DXGI_FORMAT format,unsigned w,unsigned h,ComPtr<ID3D12Resource>& out) noexcept{
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=w;desc.Height=h;desc.DepthOrArraySize=desc.MipLevels=1;desc.Format=format;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    return SUCCEEDED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&out)));
  }
  static Resources resources(Set& set) noexcept{return {set.textures[0].Get(),set.textures[1].Get(),set.textures[2].Get(),set.textures[3].Get(),set.geometry};}
  static DXGI_FORMAT view_format(unsigned role) noexcept{return role<2?DXGI_FORMAT_R16G16B16A16_FLOAT:role==2?DXGI_FORMAT_R16G16_FLOAT:DXGI_FORMAT_R32_FLOAT;}
  static bool initialize_pipeline(Set& set) {
    D3D12_DESCRIPTOR_HEAP_DESC heap{};heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;heap.NumDescriptors=14;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if(FAILED(set.device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&set.heap))))return false;set.stride=set.device->GetDescriptorHandleIncrementSize(heap.Type);
    D3D12_DESCRIPTOR_RANGE ranges[2]{};ranges[0].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;ranges[0].NumDescriptors=1;ranges[1].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_UAV;ranges[1].NumDescriptors=1;
    D3D12_ROOT_PARAMETER params[3]{};params[0].ParameterType=params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[0].DescriptorTable={1,&ranges[0]};params[1].DescriptorTable={1,&ranges[1]};params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[2].Constants={0,0,4};
    D3D12_ROOT_SIGNATURE_DESC signature{};signature.NumParameters=3;signature.pParameters=params;
    ComPtr<ID3DBlob> blob,error;if(FAILED(D3D12SerializeRootSignature(&signature,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error))||FAILED(set.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&set.root))))return false;
    const auto directory=module_directory();const wchar_t* names[]{L"colorcs.cso",L"motioncs.cso",L"depthcs.cso"};
    for(unsigned i=0;i<3;++i){const auto code=bytes(directory/names[i]);if(code.empty())return false;D3D12_COMPUTE_PIPELINE_STATE_DESC p{};p.pRootSignature=set.root.Get();p.CS={code.data(),code.size()};if(FAILED(set.device->CreateComputePipelineState(&p,IID_PPV_ARGS(&set.pipelines[i]))))return false;}
    auto cpu=set.heap->GetCPUDescriptorHandleForHeapStart();auto at=[&](unsigned index){auto value=cpu;value.ptr+=static_cast<SIZE_T>(index)*set.stride;return value;};
    auto srv=[&](unsigned index,ID3D12Resource* resource,unsigned role){D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=role==2?resource->GetDesc().Format:role==3&&resource->GetDesc().Format==DXGI_FORMAT_R32G8X24_TYPELESS?DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:view_format(role);v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Texture2D.MipLevels=1;set.device->CreateShaderResourceView(resource,&v,at(index));};
    auto write=[&](unsigned index,ID3D12Resource* resource,unsigned role){D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=view_format(role);v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;set.device->CreateUnorderedAccessView(resource,nullptr,&v,at(index));};
    for(unsigned i=0;i<4;++i){srv(i,static_cast<ID3D12Resource*>(set.originals[i]),i);srv(4+i,set.textures[i].Get(),i);write(10+i,set.textures[i].Get(),i);}
    write(8,static_cast<ID3D12Resource*>(set.originals[0]),0);write(9,static_cast<ID3D12Resource*>(set.originals[1]),1);
    return true;
  }
  static bool command(void* opaque,Set& set,ComPtr<ID3D12GraphicsCommandList>& list) noexcept{
    if(!opaque||FAILED(static_cast<IUnknown*>(opaque)->QueryInterface(IID_PPV_ARGS(&list)))||list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT)return false;
    return same_device(set.device.Get(),list.Get());
  }
  static void dispatch(Set& set,ID3D12GraphicsCommandList* list,unsigned pipeline,unsigned srv,unsigned write,unsigned sw,unsigned sh,unsigned dw,unsigned dh) noexcept{
    ID3D12DescriptorHeap* heaps[]{set.heap.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(set.root.Get());list->SetPipelineState(set.pipelines[pipeline].Get());
    auto gpu=set.heap->GetGPUDescriptorHandleForHeapStart();auto at=[&](unsigned index){auto value=gpu;value.ptr+=static_cast<UINT64>(index)*set.stride;return value;};
    list->SetComputeRootDescriptorTable(0,at(srv));list->SetComputeRootDescriptorTable(1,at(write));const unsigned dims[]{sw,sh,dw,dh};list->SetComputeRoot32BitConstants(2,4,dims,0);list->Dispatch((dw+7)/8,(dh+7)/8,1);
  }
public:
  GpuStatus prepare(const nrffx::Descriptor& d,const Target& target,Resources& out)noexcept override{
    ComPtr<ID3D12Device> device;if(!original_ok(d,device)||!(target.original==Geometry{d.render_w,d.render_h,d.output_w,d.output_h}))return GpuStatus::Unsupported;
    void* originals[4]{d.input,d.output,d.motion,d.depth};
    bool matches=active_.valid&&active_.device.Get()==device.Get()&&active_.geometry==target.scaled;
    for(unsigned i=0;i<4&&matches;++i)matches=active_.originals[i]==originals[i];
    if(matches){candidate_={};out=resources(active_);return GpuStatus::Ready;}
    Set next{};next.device=device;next.geometry=target.scaled;for(unsigned i=0;i<4;++i)next.originals[i]=originals[i];
    if(!create_texture(device.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,target.scaled.render_w,target.scaled.render_h,next.textures[0])||
       !create_texture(device.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,target.scaled.output_w,target.scaled.output_h,next.textures[1])||
       !create_texture(device.Get(),DXGI_FORMAT_R16G16_FLOAT,target.scaled.render_w,target.scaled.render_h,next.textures[2])||
       !create_texture(device.Get(),DXGI_FORMAT_R32_TYPELESS,target.scaled.render_w,target.scaled.render_h,next.textures[3]))return GpuStatus::FailedBeforeRecord;
    const std::uint32_t states[]{d.input_state,d.output_state,d.motion_state,d.depth_state};for(unsigned i=0;i<4;++i){if(state(states[i])==static_cast<D3D12_RESOURCE_STATES>(UINT_MAX))return GpuStatus::Unsupported;next.ffx_states[i]=states[i];next.internal_states[i]=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;}
    try{if(!initialize_pipeline(next))return GpuStatus::FailedBeforeRecord;}catch(...){return GpuStatus::FailedBeforeRecord;}
    next.valid=true;candidate_=std::move(next);out=resources(candidate_);return GpuStatus::Ready;
  }
  GpuStatus activate()noexcept override{if(candidate_.valid)active_=std::move(candidate_);return active_.valid?GpuStatus::Ready:GpuStatus::FailedBeforeRecord;}
  GpuStatus deactivate()noexcept override{candidate_={};active_={};return GpuStatus::Ready;}
  GpuStatus downsample(void* opaque,std::uint32_t stage)noexcept override{
    if(!active_.valid||(stage!=1&&stage!=2))return GpuStatus::Unsupported;ComPtr<ID3D12GraphicsCommandList> list;if(!command(opaque,active_,list))return GpuStatus::Unsupported;
    const unsigned roles[]{stage==1?0u:1u,2u,3u};
    for(const auto role:roles){auto* source=static_cast<ID3D12Resource*>(active_.originals[role]);auto* destination=active_.textures[role].Get();const auto source_state=state(active_.ffx_states[role]),destination_state=active_.internal_states[role];
      barrier(list.Get(),source,source_state,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);barrier(list.Get(),destination,destination_state,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
      const unsigned sw=static_cast<unsigned>(source->GetDesc().Width),sh=source->GetDesc().Height;const unsigned dw=role==1?active_.geometry.output_w:active_.geometry.render_w,dh=role==1?active_.geometry.output_h:active_.geometry.render_h;
      dispatch(active_,list.Get(),role<2?0:role==2?1:2,role,10+role,sw,sh,dw,dh);uav(list.Get(),destination);
      barrier(list.Get(),destination,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,state(active_.ffx_states[role]));barrier(list.Get(),source,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,source_state);active_.internal_states[role]=state(active_.ffx_states[role]);
    }
    return GpuStatus::Ready;
  }
  GpuStatus reconstruct(void* opaque,std::uint32_t stage)noexcept override{
    if(!active_.valid||(stage!=1&&stage!=2))return GpuStatus::Unsupported;ComPtr<ID3D12GraphicsCommandList> list;if(!command(opaque,active_,list))return GpuStatus::Unsupported;
    const unsigned role=stage==1?0u:1u;auto* source=active_.textures[role].Get();auto* destination=static_cast<ID3D12Resource*>(active_.originals[role]);const auto source_state=active_.internal_states[role],destination_state=state(active_.ffx_states[role]);
    barrier(list.Get(),source,source_state,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);barrier(list.Get(),destination,destination_state,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    dispatch(active_,list.Get(),0,4+role,8+role,static_cast<unsigned>(source->GetDesc().Width),source->GetDesc().Height,static_cast<unsigned>(destination->GetDesc().Width),destination->GetDesc().Height);uav(list.Get(),destination);
    barrier(list.Get(),destination,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,destination_state);barrier(list.Get(),source,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,source_state);return GpuStatus::Ready;
  }
};
}
std::unique_ptr<GpuBackend> make_d3d12_backend(){return std::make_unique<Backend>();}
}
