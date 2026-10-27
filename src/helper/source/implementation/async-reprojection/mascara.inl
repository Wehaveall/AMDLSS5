#include "../../../runtime/estencil_externo.h"
#include "nr_reproject_mascaracelulas.h"
#include "nr_reproject_mascaradistancia.h"
#include "nr_reproject_mascarapreenche.h"
#include "nr_reproject_mascaraaplica.h"
#include "nr_reproject_mascaradistz.h"
#include "nr_reproject_mascaracurva.h"
#include "nr_reproject_mascaracurvafim.h"
#include "nr_reproject_mascaralimita.h"
#include "nr_reproject_mascaracurvasoma.h"
#include "nr_reproject_bordaspost.h"
#include "nr_reproject_composepostmascara.h"
namespace nrreproject {
namespace {

struct MascaraK{unsigned largura,altura,celW,celH,mLarg,mAlt,mPitch,mBits,fatia;float r0,r1;unsigned kd,kf;float lnPerto,lnFrente,expoTom;unsigned suave;};
static_assert(sizeof(MascaraK)==17*4);
constexpr unsigned masc_s=8,masc_var=8;
constexpr unsigned masc_nb=20,masc_parcial=120;
}
void Backend::Impl::masc_srv_bruto(ID3D12DescriptorHeap* h,unsigned i,ID3D12Resource* r,std::uint64_t bytes){
 D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
 v.Buffer.NumElements=UINT(r?bytes/4u:1u);v.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;device->CreateShaderResourceView(r,&v,cpu(h,i));publicar(h,i);
}
void Backend::Impl::masc_uav_bruto(ID3D12DescriptorHeap* h,unsigned i,ID3D12Resource* r,std::uint64_t bytes){
 D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=UINT(r?bytes/4u:1u);
 v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;device->CreateUnorderedAccessView(r,nullptr,&v,cpu(h,i));publicar(h,i);
}
bool Backend::Impl::alocar_mascara(const D3D12_UNORDERED_ACCESS_VIEW_DESC& flags_view){
 auto& g=masc;
 if(!width||!height||!warped||!history[2]||!flags_buffer)return false;
 if(!root(device.Get(),17,6,2,g.root)||!pipeline(device.Get(),g.root.Get(),g_nr_reproject_mascaracelulas,g.celulas)||
    !pipeline(device.Get(),g.root.Get(),g_nr_reproject_mascaradistancia,g.distancia)||!pipeline(device.Get(),g.root.Get(),g_nr_reproject_mascarapreenche,g.preenche)||
    !pipeline(device.Get(),g.root.Get(),g_nr_reproject_mascaraaplica,g.aplica))return false;
 {D3D12_DESCRIPTOR_RANGE ranges[2]{};ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3,0,0,0};ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,3,0,0,0};
  D3D12_ROOT_PARAMETER rp[4]{};rp[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;rp[0].Constants={0,0,12};
  rp[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;rp[1].Constants={1,0,4};
  for(unsigned i=0;i<2;++i){rp[2+i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;rp[2+i].DescriptorTable={1,&ranges[i]};}
  D3D12_STATIC_SAMPLER_DESC s{};s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxLOD=D3D12_FLOAT32_MAX;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
  D3D12_ROOT_SIGNATURE_DESC rd{4,rp,1,&s,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> b,e;
  if(FAILED(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&e))||FAILED(device->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&g.post_root))))return false;
  nrorc_vram::registrar(nrorc::Api::Raiz,0,"mascara");}
 if(!pipeline(device.Get(),g.post_root.Get(),g_nr_reproject_composepostmascara,g.post)||!pipeline(device.Get(),g.post_root.Get(),g_nr_reproject_bordaspost,g.bordas_pso))return false;
 g.cw=(width+masc_s-1u)/masc_s;g.ch=(height+masc_s-1u)/masc_s;
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
 D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=g.cw;td.Height=g.ch;td.DepthOrArraySize=1;td.MipLevels=1;td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 struct T{ComPtr<ID3D12Resource>* r;DXGI_FORMAT f;const wchar_t* nome;} ts[]{{&g.cel,DXGI_FORMAT_R32G32B32A32_FLOAT,L"nr_mascara_celula"},{&g.aux,DXGI_FORMAT_R32G32B32A32_FLOAT,L"nr_mascara_aux"},
  {&g.dist,DXGI_FORMAT_R32_FLOAT,L"nr_mascara_distancia"},{&g.fill,DXGI_FORMAT_R32G32B32A32_FLOAT,L"nr_mascara_preenche"}};
 for(auto& t:ts){td.Format=t.f;if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,write_state,nullptr,IID_PPV_ARGS(t.r->GetAddressOf()))))return false;nrd3d::nomear(t.r->Get(),t.nome);}
 nrorc_vram::registrar(nrorc::Api::Recurso,std::uint64_t(g.cw)*g.ch*52u,"mascara");
 if(!criar_heap(8u*masc_var,g.heap)||!criar_heap(6,g.post_heap))return false;
 nrorc_vram::registrar(nrorc::Api::Heap,0,"mascara");
 constexpr auto F4=DXGI_FORMAT_R32G32B32A32_FLOAT,F1=DXGI_FORMAT_R32_FLOAT,H4=DXGI_FORMAT_R16G16B16A16_FLOAT;
 ID3D12DescriptorHeap* h=g.heap.Get();
 for(unsigned v=0;v<8u;++v){for(unsigned i=0;i<6u;++i)srv(h,v*masc_var+i,nullptr,F4);uav(h,v*masc_var+6,nullptr,F4);uav(h,v*masc_var+7,nullptr,F4);}
 srv(h,0,warped.Get(),H4);masc_srv_bruto(h,1,nullptr,0);srv(h,2,history[2].Get(),F1,true);uav(h,6,g.cel.Get(),F4);uav(h,7,g.aux.Get(),F4);
 srv(h,masc_var,g.aux.Get(),F4);uav(h,masc_var+6,g.dist.Get(),F1);
 srv(h,2*masc_var,g.cel.Get(),F4);srv(h,2*masc_var+1,g.aux.Get(),F4);srv(h,2*masc_var+2,g.dist.Get(),F1);uav(h,2*masc_var+6,g.fill.Get(),F4);
 srv(h,3*masc_var,g.cel.Get(),F4);masc_srv_bruto(h,3*masc_var+1,nullptr,0);srv(h,3*masc_var+2,g.aux.Get(),F4);srv(h,3*masc_var+3,g.dist.Get(),F1);
 srv(h,3*masc_var+4,g.fill.Get(),F4);srv(h,3*masc_var+5,history[2].Get(),F1,true);uav(h,3*masc_var+6,warped.Get(),H4);

 if(masc_prof>0.f){bool ok=pipeline(device.Get(),g.root.Get(),g_nr_reproject_mascaradistz,g.distz_pso)&&pipeline(device.Get(),g.root.Get(),g_nr_reproject_mascaracurva,g.curva_pso)&&
   pipeline(device.Get(),g.root.Get(),g_nr_reproject_mascaracurvafim,g.curvafim_pso)&&pipeline(device.Get(),g.root.Get(),g_nr_reproject_mascaralimita,g.limita_pso);
  td.Format=DXGI_FORMAT_R32_FLOAT;td.Width=g.cw;td.Height=g.ch;
  ok=ok&&SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,write_state,nullptr,IID_PPV_ARGS(&g.distz)));
  {const std::uint64_t ng=std::uint64_t((width+15u)/16u)*((height+15u)/16u);g.parcial_bytes=(ng+(ng+63u)/64u)*masc_parcial*4u;}
  D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;bd.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  bd.Width=g.parcial_bytes;ok=ok&&SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,write_state,nullptr,IID_PPV_ARGS(&g.parcial)));
  bd.Width=512;ok=ok&&SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,write_state,nullptr,IID_PPV_ARGS(&g.curva)));
  ok=ok&&pipeline(device.Get(),g.root.Get(),g_nr_reproject_mascaracurvasoma,g.curvasoma_pso);
  if(!ok){g.curvasoma_pso.Reset();g.distz_pso.Reset();g.curva_pso.Reset();g.curvafim_pso.Reset();g.limita_pso.Reset();g.distz.Reset();g.parcial.Reset();g.curva.Reset();
   nrlogfila::linha(L"nr_reproject.log","pid=%lu MASCARA_PROFUNDIDADE falhou ao alocar: so a do personagem\n",GetCurrentProcessId());}
  else{nrd3d::nomear(g.distz.Get(),L"nr_mascara_distz");nrd3d::nomear(g.parcial.Get(),L"nr_mascara_parcial");nrd3d::nomear(g.curva.Get(),L"nr_mascara_curva");
   nrorc_vram::registrar(nrorc::Api::Recurso,g.parcial_bytes+512u+std::uint64_t(g.cw)*g.ch*4u,"mascara_prof");
   const unsigned v4=4u*masc_var,v5=5u*masc_var,v6=6u*masc_var,v7=7u*masc_var;
   srv(h,v4,g.aux.Get(),F4);uav(h,v4+6,g.distz.Get(),F1);
   srv(h,v5,g.aux.Get(),F4);masc_srv_bruto(h,v5+1,nullptr,0);srv(h,v5+2,history[2].Get(),F1,true);srv(h,v5+3,g.distz.Get(),F1);srv(h,v5+4,history[0].Get(),H4,true);
   masc_uav_bruto(h,v5+6,g.parcial.Get(),g.parcial_bytes);uav(h,v5+7,warped.Get(),H4);
   masc_uav_bruto(h,v6+6,g.parcial.Get(),g.parcial_bytes);masc_uav_bruto(h,v6+7,g.curva.Get(),512);
   srv(h,v7,g.aux.Get(),F4);masc_srv_bruto(h,v7+1,nullptr,0);srv(h,v7+2,history[2].Get(),F1,true);srv(h,v7+3,g.distz.Get(),F1);srv(h,v7+4,history[0].Get(),H4,true);
   uav(h,v7+6,warped.Get(),H4);masc_uav_bruto(h,v7+7,g.curva.Get(),512);
   nrlogfila::linha(L"nr_reproject.log","pid=%lu MASCARA_PROFUNDIDADE ligada F=%.2f parcial=%llu bytes\n",GetCurrentProcessId(),double(masc_prof),static_cast<unsigned long long>(g.parcial_bytes));}}
 ID3D12DescriptorHeap* ph=g.post_heap.Get();
 srv(ph,0,warped.Get(),H4);srv(ph,1,warped.Get(),H4);masc_srv_bruto(ph,2,nullptr,0);uav(ph,3,nullptr,H4);
 device->CreateUnorderedAccessView(flags_buffer.Get(),nullptr,&flags_view,cpu(ph,4));publicar(ph,4);

 {D3D12_RESOURCE_DESC bt{};bt.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;bt.Width=(comp_w+7u)/8u;bt.Height=(comp_h+7u)/8u;bt.DepthOrArraySize=1;bt.MipLevels=1;
  bt.Format=DXGI_FORMAT_R32_UINT;bt.SampleDesc.Count=1;bt.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bt,write_state,nullptr,IID_PPV_ARGS(&g.bordas))))return false;
  nrd3d::nomear(g.bordas.Get(),L"nr_mascara_bordas");uav(ph,5,g.bordas.Get(),DXGI_FORMAT_R32_UINT);}
 nrlogfila::linha(L"nr_reproject.log","pid=%lu MASCARA alocada bits=0x%02X raio=%.1f..%.1f px_rede rede=%ux%u celulas=%ux%u saida=%ux%u%s\n",
  GetCurrentProcessId(),masc_bits,double(masc_raio*3.f/7.f),double(masc_raio),width,height,g.cw,g.ch,comp_w,comp_h,
  masc_bits?"":g.limita_pso?" so_profundidade (sem stencil)":" so_profundidade FALHOU (nada roda)");
 return true;
}

bool Backend::Impl::mascara_pronta(){
 auto& g=masc;
 auto sem=[&](const char* m){++g.sem;if(g.motivo_ult!=m){g.motivo_ult=m;
   nrlogfila::linha(L"nr_reproject.log","pid=%lu MASCARA_SEM motivo=%s quadro=%llu usados=%llu sem=%llu\n",GetCurrentProcessId(),m,
    static_cast<unsigned long long>(policy.frame),static_cast<unsigned long long>(g.quadros),static_cast<unsigned long long>(g.sem));}
  return false;};
 if(!g.aplica||!g.post||!g.heap||!g.post_heap||!g.cel)return sem("sem_recursos");

 {ComPtr<ID3D12Resource> velho;bool novo=false;
  if(const char* m=nrestext::pronto(g,device.Get(),velho,novo))return sem(m);
  if(novo){g.buf_velho=std::move(velho);
   nrlogfila::linha(L"nr_reproject.log","pid=%lu MASCARA_ESTENCIL geracao=%u %ux%u pitch=%u buffer=%llu execucao=%llu\n",GetCurrentProcessId(),g.geracao,g.larg,g.alt,g.pitch,
    static_cast<unsigned long long>(g.bytes),static_cast<unsigned long long>(g.exec_ult));}}
 g.motivo_ult=nullptr;++g.quadros;
 if(!g.logado||(g.quadros%3000u)==0u){g.logado=true;
  nrlogfila::linha(L"nr_reproject.log","pid=%lu MASCARA usada quadros=%llu sem=%llu execucao=%llu quadro=%llu\n",GetCurrentProcessId(),static_cast<unsigned long long>(g.quadros),
   static_cast<unsigned long long>(g.sem),static_cast<unsigned long long>(g.exec_ult),static_cast<unsigned long long>(policy.frame));}
 return true;
}

void Backend::Impl::mascara_halo(ID3D12GraphicsCommandList* c,bool personagem){
 auto& g=masc;
 const unsigned fatia=constants.slice%Policy::history_count;
 const float r1=masc_raio,r0=masc_raio*3.f/7.f;
 const bool prof=masc_prof>0.f&&g.limita_pso&&g.distz;
 if(!personagem&&!prof)return;
 const MascaraK k=personagem?MascaraK{width,height,g.cw,g.ch,g.larg,g.alt,g.pitch,masc_bits,fatia,r0,r1,unsigned(std::ceil(r1/float(masc_s)))+1u,unsigned(std::ceil((r1+64.f)/float(masc_s))),
  std::log(1.5f),prof?std::log(masc_prof):0.f,constants.current_exposure,masc_suave?1u:0u}:
  nrestext::mascara_k_so_prof<MascaraK>(width,height,g.cw,g.ch,fatia,r1,masc_s,std::log(masc_prof),constants.current_exposure);
 if(personagem){masc_srv_bruto(g.heap.Get(),1,g.buf.Get(),g.bytes);masc_srv_bruto(g.heap.Get(),3*masc_var+1,g.buf.Get(),g.bytes);
  if(prof){masc_srv_bruto(g.heap.Get(),5*masc_var+1,g.buf.Get(),g.bytes);masc_srv_bruto(g.heap.Get(),7*masc_var+1,g.buf.Get(),g.bytes);}}
 nrd3d::marcador(c,L"nr_mascara");
 ID3D12DescriptorHeap* heaps[]{g.heap.Get()};c->SetDescriptorHeaps(1,heaps);c->SetComputeRootSignature(g.root.Get());c->SetComputeRoot32BitConstants(0,17,&k,0);
 auto passe=[&](unsigned v,ID3D12PipelineState* pso,unsigned gx,unsigned gy){c->SetPipelineState(pso);
  c->SetComputeRootDescriptorTable(1,gpu(g.heap.Get(),v*masc_var));c->SetComputeRootDescriptorTable(2,gpu(g.heap.Get(),v*masc_var+6));c->Dispatch(gx,gy,1);};
 passe(0,g.celulas.Get(),g.cw,g.ch);
 transition(c,g.cel.Get(),write_state,read_state);transition(c,g.aux.Get(),write_state,read_state);
 if(personagem){
 passe(1,g.distancia.Get(),(g.cw+7u)/8u,(g.ch+7u)/8u);
 transition(c,g.dist.Get(),write_state,read_state);
 passe(2,g.preenche.Get(),(g.cw+7u)/8u,(g.ch+7u)/8u);
 transition(c,g.fill.Get(),write_state,read_state);}
 transition(c,warped.Get(),read_state,write_state);
 if(personagem)passe(3,g.aplica.Get(),(width+7u)/8u,(height+7u)/8u);
 if(prof){
  passe(4,g.distz_pso.Get(),(g.cw+7u)/8u,(g.ch+7u)/8u);transition(c,g.distz.Get(),write_state,read_state);
  order_uav(c,warped.Get());
  passe(5,g.curva_pso.Get(),(width+15u)/16u,(height+15u)/16u);order_uav(c,g.parcial.Get());
  {const unsigned ng=((width+15u)/16u)*((height+15u)/16u);passe(6,g.curvasoma_pso.Get(),(ng+63u)/64u,1u);order_uav(c,g.parcial.Get());}
  passe(6,g.curvafim_pso.Get(),1u,1u);order_uav(c,g.curva.Get());
  passe(7,g.limita_pso.Get(),(width+7u)/8u,(height+7u)/8u);
  transition(c,g.distz.Get(),read_state,write_state);}
 transition(c,warped.Get(),write_state,read_state);
 for(ID3D12Resource* r:{g.cel.Get(),g.aux.Get()})transition(c,r,read_state,write_state);
 if(personagem)for(ID3D12Resource* r:{g.dist.Get(),g.fill.Get()})transition(c,r,read_state,write_state);
}

bool Backend::Impl::mascara_post(ID3D12GraphicsCommandList* c,ID3D12Resource* dst,DXGI_FORMAT fmt,const void* k){
 auto& g=masc;
 if(!g.post||!g.bordas_pso||!g.bordas||!g.post_root||!g.post_heap||!g.buf||!c||!dst)return false;
 const unsigned cm[4]{g.larg,g.alt,g.pitch,masc_bits};
 masc_srv_bruto(g.post_heap.Get(),2,g.buf.Get(),g.bytes);uav(g.post_heap.Get(),3,dst,fmt);
 ID3D12DescriptorHeap* heaps[]{g.post_heap.Get()};c->SetDescriptorHeaps(1,heaps);c->SetComputeRootSignature(g.post_root.Get());
 c->SetComputeRoot32BitConstants(0,12,k,0);c->SetComputeRoot32BitConstants(1,4,cm,0);
 c->SetComputeRootDescriptorTable(2,gpu(g.post_heap.Get(),0));c->SetComputeRootDescriptorTable(3,gpu(g.post_heap.Get(),3));

 c->SetPipelineState(g.bordas_pso.Get());c->Dispatch(((comp_w+7u)/8u+7u)/8u,((comp_h+7u)/8u+7u)/8u,1);order_uav(c,g.bordas.Get());
 nrestext::bordas_quadro={g.bordas.Get(),comp_w,comp_h,g.exec_ult};
 c->SetPipelineState(g.post.Get());
 return true;
}
}
