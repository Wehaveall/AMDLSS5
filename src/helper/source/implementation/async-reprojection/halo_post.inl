#include "nr_reproject_halopostmomentos.h"
#include "nr_reproject_halopostmomentosp.h"
#include "nr_reproject_halopostref.h"
#include "nr_reproject_halopostaplicar.h"
namespace nrreproject {
namespace {
struct HaloPostK{unsigned w,h,fatia,tone;float expo,forca,raio;unsigned qw,qh,pad0,pad1,pad2;};
static_assert(sizeof(HaloPostK)==48);
}
bool Backend::Impl::alocar_halo_post(const D3D12_UNORDERED_ACCESS_VIEW_DESC& flags_view){
 auto& g=hpost;
 if(!width||!height||!warped||!history[0]||!history[2]||!flags_buffer)return false;
 if(!root(device.Get(),12,5,3,g.root)||!pipeline(device.Get(),g.root.Get(),g_nr_reproject_halopostmomentos,g.mom_pso)||
    (halo_post_salto>0.f&&!pipeline(device.Get(),g.root.Get(),g_nr_reproject_halopostmomentosp,g.momp_pso))||
    !pipeline(device.Get(),g.root.Get(),g_nr_reproject_halopostref,g.ref_pso)||!pipeline(device.Get(),g.root.Get(),g_nr_reproject_halopostaplicar,g.apl_pso))return false;
 g.qw=(width+3u)/4u;g.qh=(height+3u)/4u;
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
 D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=g.qw;td.Height=g.qh;td.DepthOrArraySize=1;td.MipLevels=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
 td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read_state,nullptr,IID_PPV_ARGS(&g.mom)))||
    FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read_state,nullptr,IID_PPV_ARGS(&g.ref))))return false;
 td.Width=width;td.Height=height;td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
 if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read_state,nullptr,IID_PPV_ARGS(&g.saida))))return false;
 D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,8,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
 if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&g.heap))))return false;
 hd.NumDescriptors=4;
 if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&g.post_heap))))return false;
 const DXGI_FORMAT f=DXGI_FORMAT_R16G16B16A16_FLOAT;
 srv(g.heap.Get(),0,warped.Get(),f);srv(g.heap.Get(),1,history[0].Get(),f,true);srv(g.heap.Get(),2,history[2].Get(),DXGI_FORMAT_R32_FLOAT,true);
 const DXGI_FORMAT f32=DXGI_FORMAT_R32G32B32A32_FLOAT;
 srv(g.heap.Get(),3,g.mom.Get(),f32);srv(g.heap.Get(),4,g.ref.Get(),f32);
 uav(g.heap.Get(),5,g.mom.Get(),f32);uav(g.heap.Get(),6,g.ref.Get(),f32);uav(g.heap.Get(),7,g.saida.Get(),f);
 srv(g.post_heap.Get(),0,g.saida.Get(),f);srv(g.post_heap.Get(),1,g.saida.Get(),f);
 device->CreateUnorderedAccessView(flags_buffer.Get(),nullptr,&flags_view,cpu(g.post_heap.Get(),3));
 return true;
}

bool Backend::Impl::halo_post(ID3D12GraphicsCommandList* c,float exposure,unsigned tone){
 auto& g=hpost;
 if(!(halo_post_forca>0.f)||!g.apl_pso||!g.heap||!g.post_heap||!g.saida||!warped_read||!histories_read)return false;
 if(!std::isfinite(exposure)||!(exposure>0.f))return false;

 const double pct=pct_raio();const double r=(pct>0.&&pct<100.)?halo_post_raio*100./pct:halo_post_raio;
 unsigned salto_u=0u;{const float sv=halo_post_salto;std::memcpy(&salto_u,&sv,sizeof salto_u);}
 HaloPostK k{width,height,constants.slice%Policy::history_count,tone&1u,exposure,halo_post_forca,float(std::min(30.,r)),g.qw,g.qh,salto_u,0u,0u};
 if(!g.logado){g.logado=true;{
  nrlogfila::linha(L"nr_reproject.log","pid=%lu HALO_TETO_POST forca=%.3f raio_rede=%.1f raio_hist=%.1f historico=%ux%u saida=%ux%u reduzido=%u tone=%u salto=%.3f\n",GetCurrentProcessId(),
   double(halo_post_forca),halo_post_raio,double(k.raio),width,height,comp_w,comp_h,reduzido?1u:0u,tone&1u,double(halo_post_salto));}}
 if(halo_post_salto>0.f&&g.momp_pso){

  auto tr=[](ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){D3D12_RESOURCE_BARRIER v{};v.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;v.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,a,b};return v;};
  const D3D12_RESOURCE_BARRIER ini[3]{tr(g.mom.Get(),read_state,write_state),tr(g.ref.Get(),read_state,write_state),tr(g.saida.Get(),read_state,write_state)};
  c->ResourceBarrier(3,ini);
  bind(c,g.heap.Get(),g.root.Get(),g.momp_pso.Get(),12,&k,5);c->Dispatch((width+7)/8,(height+7)/8,1);
  transition(c,g.mom.Get(),write_state,read_state);
  c->SetPipelineState(g.ref_pso.Get());c->Dispatch((g.qw+7)/8,(g.qh+7)/8,1);
  transition(c,g.ref.Get(),write_state,read_state);
  c->SetPipelineState(g.apl_pso.Get());c->Dispatch((width+7)/8,(height+7)/8,1);
  transition(c,g.saida.Get(),write_state,read_state);
  ++g.quadros;return true;
 }
 transition(c,g.mom.Get(),read_state,write_state);
 bind(c,g.heap.Get(),g.root.Get(),g.mom_pso.Get(),12,&k,5);c->Dispatch((g.qw+7)/8,(g.qh+7)/8,1);
 transition(c,g.mom.Get(),write_state,read_state);
 transition(c,g.ref.Get(),read_state,write_state);
 c->SetPipelineState(g.ref_pso.Get());c->Dispatch((g.qw+7)/8,(g.qh+7)/8,1);
 transition(c,g.ref.Get(),write_state,read_state);
 transition(c,g.saida.Get(),read_state,write_state);
 c->SetPipelineState(g.apl_pso.Get());c->Dispatch((width+7)/8,(height+7)/8,1);
 transition(c,g.saida.Get(),write_state,read_state);
 ++g.quadros;return true;
}
}
