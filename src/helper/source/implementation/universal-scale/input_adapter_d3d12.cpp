#include "d3d12_backend.h"
#include "nr_adapt_color.h"
#include "nr_adapt_motion.h"
#include "nr_adapt_output.h"
#include "nr_adapt_depth.h"
#include "nr_adapt_copyout.h"
#include "nr_adapt_exposure.h"

#include <d3d12.h>
#include <wrl/client.h>
#include "../../../runtime/d3d12_comum.h"
#include "../../../runtime/estado_ffx.h"
#include <array>
#include <climits>
#include <cstring>
#include <deque>

namespace nrscale {
namespace {
using Microsoft::WRL::ComPtr;

constexpr std::uint32_t kFfxComputeRead=4;

D3D12_RESOURCE_STATES d3d12_state(std::uint32_t value) noexcept{return nrd3d::estado_ffx(value);}
constexpr auto kUnknownState=nrd3d::estado_recusado;
class Adapter final:public InputAdapter {

  static constexpr UINT kBlocks=32,kPerBlock=10;
  ComPtr<ID3D12Device> device_;ComPtr<ID3D12RootSignature> root_;std::array<ComPtr<ID3D12PipelineState>,6> pipelines_;
  ComPtr<ID3D12DescriptorHeap> heap_;UINT stride_{},block_{};
  ComPtr<ID3D12Resource> color_,motion_,depth_;D3D12_RESOURCE_STATES color_state_{},motion_state_{},depth_state_{};
  ComPtr<ID3D12Resource> output_;D3D12_RESOURCE_STATES output_state_{};
  ComPtr<ID3D12Resource> exposure_;D3D12_RESOURCE_STATES exposure_state_{};

  struct Retired {ComPtr<ID3D12Resource> resource;unsigned due{};};
  std::deque<Retired> retired_;unsigned conversions_{};
  bool initialize(ID3D12Device* device) noexcept{
    if(device_.Get()==device)return root_&&heap_;
    if(device_)return false;
    device_=device;
    D3D12_DESCRIPTOR_RANGE ranges[2]{};ranges[0].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;ranges[0].NumDescriptors=1;
    ranges[1].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_UAV;ranges[1].NumDescriptors=1;
    D3D12_ROOT_PARAMETER params[3]{};params[0].ParameterType=params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[0].DescriptorTable={1,&ranges[0]};params[1].DescriptorTable={1,&ranges[1]};
    params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[2].Constants={0,0,6};
    D3D12_ROOT_SIGNATURE_DESC signature{};signature.NumParameters=3;signature.pParameters=params;
    ComPtr<ID3DBlob> blob,error;
    if(FAILED(D3D12SerializeRootSignature(&signature,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error))||
       FAILED(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root_))))return false;
    const std::pair<const void*,SIZE_T> code[6]{{g_nr_adapt_color,sizeof(g_nr_adapt_color)},{g_nr_adapt_motion,sizeof(g_nr_adapt_motion)},
                                                {g_nr_adapt_output,sizeof(g_nr_adapt_output)},{g_nr_adapt_depth,sizeof(g_nr_adapt_depth)},
                                                {g_nr_adapt_copyout,sizeof(g_nr_adapt_copyout)},{g_nr_adapt_exposure,sizeof(g_nr_adapt_exposure)}};
    for(unsigned i=0;i<6;++i){D3D12_COMPUTE_PIPELINE_STATE_DESC p{};p.pRootSignature=root_.Get();p.CS={code[i].first,code[i].second};
      if(FAILED(device->CreateComputePipelineState(&p,IID_PPV_ARGS(&pipelines_[i]))))return false;}
    D3D12_DESCRIPTOR_HEAP_DESC heap{};heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;heap.NumDescriptors=kBlocks*kPerBlock;
    heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if(FAILED(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&heap_))))return false;
    stride_=device->GetDescriptorHandleIncrementSize(heap.Type);return true;
  }
  bool texture(DXGI_FORMAT format,UINT64 w,UINT h,ComPtr<ID3D12Resource>& slot,D3D12_RESOURCE_STATES& state) noexcept{
    if(slot){const auto d=slot->GetDesc();if(d.Width==w&&d.Height==h&&d.Format==format)return true;
      retired_.push_back({slot,conversions_+64});slot.Reset();}
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=w;desc.Height=h;
    desc.DepthOrArraySize=desc.MipLevels=1;desc.Format=format;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    if(FAILED(device_->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&slot))))return false;
    state=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;return true;
  }
  D3D12_CPU_DESCRIPTOR_HANDLE cpu(UINT index) const noexcept{auto h=heap_->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(block_*kPerBlock+index)*stride_;return h;}
  D3D12_GPU_DESCRIPTOR_HANDLE gpu(UINT index) const noexcept{auto h=heap_->GetGPUDescriptorHandleForHeapStart();h.ptr+=UINT64(block_*kPerBlock+index)*stride_;return h;}
  void srv(UINT index,ID3D12Resource* resource,std::uint32_t view) noexcept{
    D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=static_cast<DXGI_FORMAT>(view);v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Texture2D.MipLevels=1;device_->CreateShaderResourceView(resource,&v,cpu(index));
  }
  void uav(UINT index,ID3D12Resource* resource,DXGI_FORMAT format) noexcept{
    D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=format;v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device_->CreateUnorderedAccessView(resource,nullptr,&v,cpu(index));
  }
  void run(ID3D12GraphicsCommandList* list,unsigned pipeline,UINT source,UINT destination,const float mul[4],UINT w,UINT h) noexcept{
    ID3D12DescriptorHeap* heaps[]{heap_.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root_.Get());
    list->SetPipelineState(pipelines_[pipeline].Get());list->SetComputeRootDescriptorTable(0,gpu(source));list->SetComputeRootDescriptorTable(1,gpu(destination));
    std::uint32_t constants[6]{};std::memcpy(constants,mul,4*sizeof(float));constants[4]=w;constants[5]=h;
    list->SetComputeRoot32BitConstants(2,6,constants,0);list->Dispatch((w+7)/8,(h+7)/8,1);
  }
  bool command(void* opaque,ComPtr<ID3D12GraphicsCommandList>& list) noexcept{
    if(!opaque||FAILED(static_cast<IUnknown*>(opaque)->QueryInterface(IID_PPV_ARGS(&list))))return false;
    const auto type=list->GetType();
    return (type==D3D12_COMMAND_LIST_TYPE_DIRECT||type==D3D12_COMMAND_LIST_TYPE_COMPUTE)&&nrd3d::mesmo_dispositivo(device_.Get(),list.Get());
  }

  void copy(ID3D12GraphicsCommandList* list,ID3D12Resource* source,D3D12_RESOURCE_STATES source_state,std::uint32_t view,
            ID3D12Resource* destination,D3D12_RESOURCE_STATES& destination_state,DXGI_FORMAT destination_format,unsigned pipeline,
            const float mul[4],UINT w,UINT h,UINT slot) noexcept{
    srv(slot,source,view);uav(slot+1,destination,destination_format);
    const bool readable=(source_state&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)!=0;
    if(!readable)nrd3d::transicao(list,source,source_state,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    nrd3d::transicao(list,destination,destination_state,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    run(list,pipeline,slot,slot+1,mul,w,h);nrd3d::barreira_uav(list,destination);
    nrd3d::transicao(list,destination,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    destination_state=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if(!readable)nrd3d::transicao(list,source,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,source_state);
  }
public:
  bool describe(void* resource,NrResourceDesc& out) noexcept override{
    if(!resource)return false;ComPtr<ID3D12Resource> r;
    if(FAILED(static_cast<IUnknown*>(resource)->QueryInterface(IID_PPV_ARGS(&r))))return false;
    const auto d=r->GetDesc();out={};out.dimension=static_cast<std::uint32_t>(d.Dimension);out.alignment=d.Alignment;out.width=d.Width;
    out.height=d.Height;out.depth_or_array_size=d.DepthOrArraySize;out.mip_levels=d.MipLevels;out.format=static_cast<std::uint32_t>(d.Format);
    out.sample_count=d.SampleDesc.Count;out.sample_quality=d.SampleDesc.Quality;out.layout=static_cast<std::uint32_t>(d.Layout);
    out.flags=static_cast<std::uint32_t>(d.Flags);return true;
  }
  GpuStatus convert(const nrffx::RawDescriptor& d,const AdapterPlan& plan,AdaptedInputs& out) noexcept override{
    try{
      if(!plan.adapt)return GpuStatus::Unsupported;
      ComPtr<ID3D12Resource> input,motion,depth;ComPtr<ID3D12Device> device;
      if(FAILED(static_cast<IUnknown*>(d.input)->QueryInterface(IID_PPV_ARGS(&input)))||
         FAILED(static_cast<IUnknown*>(d.motion)->QueryInterface(IID_PPV_ARGS(&motion)))||
         FAILED(static_cast<IUnknown*>(d.depth)->QueryInterface(IID_PPV_ARGS(&depth)))||FAILED(input->GetDevice(IID_PPV_ARGS(&device))))return GpuStatus::Unsupported;
      if(!initialize(device.Get())||!nrd3d::mesmo_dispositivo(device_.Get(),motion.Get())||!nrd3d::mesmo_dispositivo(device_.Get(),depth.Get()))return GpuStatus::Unsupported;
      ComPtr<ID3D12GraphicsCommandList> list;if(!command(d.command,list))return GpuStatus::Unsupported;
      const auto input_state=d3d12_state(d.input_state),motion_state=d3d12_state(d.motion_state),depth_state=d3d12_state(d.depth_state);
      if((plan.color&&input_state==kUnknownState)||(plan.motion&&motion_state==kUnknownState)||(plan.depth&&depth_state==kUnknownState))return GpuStatus::Unsupported;

      if(list->GetType()==D3D12_COMMAND_LIST_TYPE_COMPUTE&&((plan.color&&!nrffx_estado::valido_em_compute(unsigned(input_state)))||
         (plan.motion&&!nrffx_estado::valido_em_compute(unsigned(motion_state)))||(plan.depth&&!nrffx_estado::valido_em_compute(unsigned(depth_state)))))
        return GpuStatus::Unsupported;
      if(plan.color&&!texture(DXGI_FORMAT_R16G16B16A16_FLOAT,d.render_w,d.render_h,color_,color_state_))return GpuStatus::FailedBeforeRecord;
      if(plan.motion&&!texture(DXGI_FORMAT_R16G16_FLOAT,d.render_w,d.render_h,motion_,motion_state_))return GpuStatus::FailedBeforeRecord;
      if(plan.depth&&!texture(DXGI_FORMAT_R32_FLOAT,d.render_w,d.render_h,depth_,depth_state_))return GpuStatus::FailedBeforeRecord;
      if(plan.output&&!texture(DXGI_FORMAT_R16G16B16A16_FLOAT,d.output_w,d.output_h,output_,output_state_))return GpuStatus::FailedBeforeRecord;
      ComPtr<ID3D12Resource> exposure;D3D12_RESOURCE_STATES exposure_state{};
      if(plan.exposure){
        if(!d.exposure||FAILED(static_cast<IUnknown*>(d.exposure)->QueryInterface(IID_PPV_ARGS(&exposure)))||!nrd3d::mesmo_dispositivo(device_.Get(),exposure.Get()))return GpuStatus::Unsupported;
        exposure_state=d3d12_state(d.exposure_state);if(exposure_state==kUnknownState)return GpuStatus::Unsupported;
        if(!texture(DXGI_FORMAT_R32_FLOAT,1,1,exposure_,exposure_state_))return GpuStatus::FailedBeforeRecord;
      }
      block_=(block_+1)%kBlocks;++conversions_;
      while(!retired_.empty()&&int(conversions_-retired_.front().due)>=0)retired_.pop_front();
      if(plan.color){const float mul[4]{plan.color_scale,1.f,1.f,1.f};
        copy(list.Get(),input.Get(),input_state,plan.color_view,color_.Get(),color_state_,DXGI_FORMAT_R16G16B16A16_FLOAT,0,mul,d.render_w,d.render_h,0);
        out.color=color_.Get();out.color_state=kFfxComputeRead;}
      if(plan.motion){const float mul[4]{plan.motion_x,plan.motion_y,plan.motion_amostra_x,plan.motion_amostra_y};
        copy(list.Get(),motion.Get(),motion_state,plan.motion_view,motion_.Get(),motion_state_,DXGI_FORMAT_R16G16_FLOAT,1,mul,d.render_w,d.render_h,2);
        out.motion=motion_.Get();out.motion_state=kFfxComputeRead;}
      if(plan.depth){const float mul[4]{1.f,1.f,1.f,1.f};
        copy(list.Get(),depth.Get(),depth_state,plan.depth_view,depth_.Get(),depth_state_,DXGI_FORMAT_R32_FLOAT,3,mul,d.render_w,d.render_h,6);
        out.depth=depth_.Get();out.depth_state=kFfxComputeRead;}
      if(plan.exposure){const float mul[4]{1.f,1.f,1.f,1.f};
        copy(list.Get(),exposure.Get(),exposure_state,plan.exposure_view,exposure_.Get(),exposure_state_,DXGI_FORMAT_R32_FLOAT,5,mul,1,1,8);
        out.exposure=exposure_.Get();out.exposure_state=kFfxComputeRead;}
      if(plan.output){nrd3d::transicao(list.Get(),output_.Get(),output_state_,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);output_state_=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        out.output=output_.Get();out.output_state=2;}
      return GpuStatus::Ready;
    }catch(...){return GpuStatus::FailedAfterRecord;}
  }
  GpuStatus finish(const nrffx::RawDescriptor& d,const AdapterPlan& plan) noexcept override{
    try{
      if(plan.output_scale==1.f&&!plan.output)return GpuStatus::Ready;
      ComPtr<ID3D12Resource> output;ComPtr<ID3D12GraphicsCommandList> list;
      if(!device_||FAILED(static_cast<IUnknown*>(d.output)->QueryInterface(IID_PPV_ARGS(&output)))||!nrd3d::mesmo_dispositivo(device_.Get(),output.Get())||
         !command(d.command,list))return GpuStatus::Unsupported;
      const auto state=d3d12_state(d.output_state);if(state==kUnknownState)return GpuStatus::Unsupported;
      if(list->GetType()==D3D12_COMMAND_LIST_TYPE_COMPUTE&&!nrffx_estado::valido_em_compute(unsigned(state)))return GpuStatus::Unsupported;
      if(plan.output){

        if(!output_)return GpuStatus::Unsupported;
        uav(4,output.Get(),static_cast<DXGI_FORMAT>(plan.output_view));srv(5,output_.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
        nrd3d::transicao(list.Get(),output.Get(),state,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        if(state==D3D12_RESOURCE_STATE_UNORDERED_ACCESS)nrd3d::barreira_uav(list.Get(),output.Get());
        nrd3d::transicao(list.Get(),output_.Get(),output_state_,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        const float mul[4]{plan.output_scale,1.f,1.f,1.f};run(list.Get(),4,5,4,mul,d.output_w,d.output_h);
        nrd3d::barreira_uav(list.Get(),output.Get());nrd3d::transicao(list.Get(),output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,state);
        nrd3d::transicao(list.Get(),output_.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);output_state_=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        return GpuStatus::Ready;
      }
      uav(4,output.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
      {D3D12_SHADER_RESOURCE_VIEW_DESC none{};none.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;none.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
       none.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;none.Texture2D.MipLevels=1;device_->CreateShaderResourceView(nullptr,&none,cpu(5));}
      nrd3d::transicao(list.Get(),output.Get(),state,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
      if(state==D3D12_RESOURCE_STATE_UNORDERED_ACCESS)nrd3d::barreira_uav(list.Get(),output.Get());
      const float mul[4]{plan.output_scale,1.f,1.f,1.f};run(list.Get(),2,5,4,mul,d.output_w,d.output_h);
      nrd3d::barreira_uav(list.Get(),output.Get());nrd3d::transicao(list.Get(),output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,state);
      return GpuStatus::Ready;
    }catch(...){return GpuStatus::FailedAfterRecord;}
  }
};
}
std::unique_ptr<InputAdapter> make_d3d12_input_adapter(){return std::make_unique<Adapter>();}
}
