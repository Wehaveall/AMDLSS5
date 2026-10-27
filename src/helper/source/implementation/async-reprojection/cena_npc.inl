#include "../../../runtime/cena_npc.h"
#include "nr_reproject_npcmede.h"
#include "nr_reproject_composepostnpc.h"
namespace nrreproject {
namespace {
struct CenaNpcK{unsigned pack;float fixo,fmin,fmax,alfa_f,alfa_ref,fpx;unsigned prof_ab;float ent[8][4];unsigned z[8];};
static_assert(sizeof(CenaNpcK)==48*4);
constexpr unsigned cnpc_ent=8,cnpc_bytes=cnpc_ent*128u;
}
bool Backend::Impl::alocar_cena_npc(const D3D12_UNORDERED_ACCESS_VIEW_DESC& flags_view){
 auto& g=cnpc;
 if(!width||!height||!warped||!history[2]||!flags_buffer)return false;
 D3D12_DESCRIPTOR_RANGE ranges[2]{};ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3,0,0,0};ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,3,0,0,0};
 D3D12_ROOT_PARAMETER rp[4]{};rp[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;rp[0].Constants={0,0,12};
 rp[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;rp[1].Constants={1,0,48};
 for(unsigned i=0;i<2;++i){rp[2+i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;rp[2+i].DescriptorTable={1,&ranges[i]};}
 D3D12_STATIC_SAMPLER_DESC s{};s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxLOD=D3D12_FLOAT32_MAX;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
 D3D12_ROOT_SIGNATURE_DESC rd{4,rp,1,&s,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> b,e;
 if(FAILED(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&e))||FAILED(device->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&g.root))))return false;
 nrorc_vram::registrar(nrorc::Api::Raiz,0,"cena_npc");
 if(!pipeline(device.Get(),g.root.Get(),g_nr_reproject_npcmede,g.mede)||!pipeline(device.Get(),g.root.Get(),g_nr_reproject_composepostnpc,g.compor))return false;
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
 D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=cnpc_bytes;bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;bd.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,write_state,nullptr,IID_PPV_ARGS(&g.estado))))return false;
 nrd3d::nomear(g.estado.Get(),L"nr_npc_estado");
 nrorc_vram::registrar(nrorc::Api::Recurso,cnpc_bytes,"cena_npc");

 {D3D12_HEAP_PROPERTIES uh{};uh.Type=D3D12_HEAP_TYPE_UPLOAD;D3D12_RESOURCE_DESC zd=bd;zd.Flags=D3D12_RESOURCE_FLAG_NONE;void* m=nullptr;const D3D12_RANGE nada{0,0};
  if(FAILED(device->CreateCommittedResource(&uh,D3D12_HEAP_FLAG_NONE,&zd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&g.zeros)))||FAILED(g.zeros->Map(0,&nada,&m))||!m)return false;
  nrd3d::nomear(g.zeros.Get(),L"nr_npc_zeros");
  std::memset(m,0,cnpc_bytes);g.zeros->Unmap(0,nullptr);nrorc_vram::registrar(nrorc::Api::Recurso,0,"cena_npc_zeros");}
 g.zerado=false;
 if(!criar_heap(6,g.heap))return false;
 nrorc_vram::registrar(nrorc::Api::Heap,0,"cena_npc");
 srv(g.heap.Get(),0,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);srv(g.heap.Get(),1,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
 srv(g.heap.Get(),2,history[2].Get(),DXGI_FORMAT_R32_FLOAT,true);
 uav(g.heap.Get(),3,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);
 device->CreateUnorderedAccessView(flags_buffer.Get(),nullptr,&flags_view,cpu(g.heap.Get(),4));
 {D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=cnpc_bytes/4u;v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
  device->CreateUnorderedAccessView(g.estado.Get(),nullptr,&v,cpu(g.heap.Get(),5));}
 for(auto& o:g.on_ant)o=false;
 nrlogfila::linha(L"nr_reproject.log","pid=%lu CENA_NPC alocada modo=%u fixo=%.3f limites=%.2f..%.2f suave=%.3f ref_alfa=%.4f inicio=%u zonas=%u camada=%.2f..%.2f cabecas viz=%d historico=%ux%u saida=%ux%u\n",
  GetCurrentProcessId(),cnpc_cfg.modo,double(cnpc_cfg.fixo),double(cnpc_cfg.fmin),double(cnpc_cfg.fmax),double(cnpc_cfg.alfa_f),double(cnpc_cfg.alfa_ref),cnpc_cfg.inicio,cnpc_cfg.zonas,
  double(cnpc_cfg.prof_a),double(cnpc_cfg.prof_b),cnpc_cfg.viz?1:0,width,height,comp_w,comp_h);
 return true;
}

bool Backend::Impl::cena_npc(ID3D12GraphicsCommandList* c,ID3D12Resource* dst,DXGI_FORMAT fmt,const void* k){
 auto& g=cnpc;
 if(!cnpc_cfg.modo||!g.compor||!g.mede||!g.heap||!g.estado||!histories_read||!warped_read||!c||!dst||!comp_w||!comp_h)return false;
 if(!nrrj::ativo())return false;
 CenaNpcK cn{};
 bool algum=false;float gx=0.f,gy=0.f,gt=0.f;bool g_on=false;
 for(unsigned e=0;e<cnpc_ent;++e){
  bool on=false;float cx=0.f,cy=0.f,t=0.f,zx=0.f,zy=0.f;std::uint64_t ger=0;
  if(e<unsigned(nrrj::n_inst)){
   nrrj::Inst i1(static_cast<int>(e));
   const auto det=nrrj::ler_deteccao();const auto& q=nrrj::quadro_render();ger=nrrj::ger().load(std::memory_order_acquire);
   on=det.on&&det.tem&&q.lado>0.f&&std::isfinite(q.cx)&&std::isfinite(q.cy)&&std::isfinite(q.lado);
   if(e>=1u&&(!nrrj::npc2.load(std::memory_order_relaxed)||int(e)>nrrj::npc_n()))on=false;
   cx=q.cx;cy=q.cy;t=q.lado;zx=q.cx+det.zdx;zy=q.cy+det.zdy;

   if(on&&e>=1u&&g_on&&std::fabs(cx-gx)<.6f*gt&&std::fabs(cy-gy)<.6f*gt)on=false;
   if(e==0u&&on){g_on=true;gx=cx;gy=cy;gt=t;}
  }
  const nrcnpc::Vista ant{g.on_ant[e],g.ger[e],g.cx_ant[e],g.cy_ant[e],g.t_ant[e]},at{on,ger,cx,cy,t};
  const bool reinicio=nrcnpc::reiniciar(ant,at);
  g.on_ant[e]=on;g.ger[e]=ger;g.cx_ant[e]=cx;g.cy_ant[e]=cy;g.t_ant[e]=t;
  if(!on)continue;
  const unsigned bits=1u|(reinicio?2u:0u);
  cn.ent[e][0]=cx;cn.ent[e][1]=cy;cn.ent[e][2]=t;std::memcpy(&cn.ent[e][3],&bits,4);
  cn.z[e]=nrcnpc::f16(std::clamp(zx,0.f,float(comp_w)))|(nrcnpc::f16(std::clamp(zy,0.f,float(comp_h)))<<16);
  if(e>=1u)algum=true;
 }
 ++g.quadros;
 if(!algum)return false;
 const unsigned fatia=constants.slice%Policy::history_count;
 cn.pack=(cnpc_cfg.modo&15u)|((std::min(cnpc_cfg.inicio,255u))<<4)|(cnpc_ent<<12)|((fatia&255u)<<16)|((cnpc_cfg.zonas&7u)<<24)|(cnpc_cfg.viz?(1u<<27):0u)|((std::clamp(cnpc_cfg.ref_n,1u,15u))<<28);
 cn.fixo=cnpc_cfg.fixo;cn.fmin=cnpc_cfg.fmin;cn.fmax=cnpc_cfg.fmax;cn.alfa_f=cnpc_cfg.alfa_f;cn.alfa_ref=cnpc_cfg.alfa_ref;
 cn.fpx=nrcnpc::fpx(nrrj::cam_fov.load(std::memory_order_relaxed),comp_h);
 cn.prof_ab=nrcnpc::f16(cnpc_cfg.prof_a)|(nrcnpc::f16(cnpc_cfg.prof_b)<<16);
 uav(g.heap.Get(),3,dst,fmt);
 if(!g.zerado){
  transition(c,g.estado.Get(),write_state,D3D12_RESOURCE_STATE_COPY_DEST);c->CopyBufferRegion(g.estado.Get(),0,g.zeros.Get(),0,cnpc_bytes);
  transition(c,g.estado.Get(),D3D12_RESOURCE_STATE_COPY_DEST,write_state);
  for(unsigned e=0;e<cnpc_ent;++e){unsigned b2=0;std::memcpy(&b2,&cn.ent[e][3],4);if(b2&1u){b2|=2u;std::memcpy(&cn.ent[e][3],&b2,4);}}
  g.zerado=true;}
 ID3D12DescriptorHeap* heaps[]{g.heap.Get()};c->SetDescriptorHeaps(1,heaps);c->SetComputeRootSignature(g.root.Get());
 c->SetComputeRoot32BitConstants(0,12,k,0);c->SetComputeRoot32BitConstants(1,48,&cn,0);
 c->SetComputeRootDescriptorTable(2,gpu(g.heap.Get(),0));c->SetComputeRootDescriptorTable(3,gpu(g.heap.Get(),3));
 nrd3d::marcador(c,L"nr_npc_mede");c->SetPipelineState(g.mede.Get());c->Dispatch(cnpc_ent,1,1);
 order_uav(c,g.estado.Get());order_uav(c,dst);
 c->SetPipelineState(g.compor.Get());
 ++g.usados;
 if(!g.logado){g.logado=true;nrlogfila::linha(L"nr_reproject.log","pid=%lu CENA_NPC ligada fpx=%.1f fatia=%u\n",GetCurrentProcessId(),double(cn.fpx),fatia);}
 return true;
}
}
