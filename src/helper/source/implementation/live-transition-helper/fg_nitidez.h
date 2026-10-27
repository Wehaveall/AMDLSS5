#pragma once
#include "../../../runtime/nr_publico.h"

#include <windows.h>
#include "../../../runtime/log_fila.h"
#include "../../../runtime/pasta_helper.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <wrl/client.h>
#include "../../../runtime/d3d12_comum.h"
#include "../../../runtime/ini_jogo.h"
#include "../../../runtime/estado_ffx.h"
#include "../../../runtime/gpu_perdida.h"
#include "../../../runtime/rostos_quadro.h"

#if defined(NR_FG_NITIDEZ_CS) && NR_FG_NITIDEZ_CS
#include "nr_fg_nitidez_main.h"
#include "nr_fg_nitidez_energia.h"
#endif
namespace nrfgnitidez {
using Microsoft::WRL::ComPtr;

struct FfxRes{void* resource;std::uint32_t desc[8];std::uint32_t state;std::uint32_t pad;};
static_assert(sizeof(FfxRes)==48,"FfxApiResource");
struct FgDesc{std::uint64_t type;void* next;void* commandList;FfxRes presentColor;FfxRes outputs[4];std::uint32_t numGenerated;};

inline D3D12_RESOURCE_STATES estado(std::uint32_t s){return nrd3d::estado_ffx(s);}

inline DXGI_FORMAT vista(DXGI_FORMAT f){
 switch(f){
 case DXGI_FORMAT_R10G10B10A2_TYPELESS:case DXGI_FORMAT_R10G10B10A2_UNORM:return DXGI_FORMAT_R10G10B10A2_UNORM;
 case DXGI_FORMAT_R8G8B8A8_TYPELESS:case DXGI_FORMAT_R8G8B8A8_UNORM:case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:return DXGI_FORMAT_R8G8B8A8_UNORM;
 case DXGI_FORMAT_B8G8R8A8_TYPELESS:case DXGI_FORMAT_B8G8R8A8_UNORM:case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:return DXGI_FORMAT_B8G8R8A8_UNORM;
 case DXGI_FORMAT_R16G16B16A16_TYPELESS:case DXGI_FORMAT_R16G16B16A16_FLOAT:return DXGI_FORMAT_R16G16B16A16_FLOAT;
 case DXGI_FORMAT_R11G11B10_FLOAT:return DXGI_FORMAT_R11G11B10_FLOAT;
 default:return DXGI_FORMAT_UNKNOWN;}}

inline const char* hlsl=R"(
cbuffer K:register(b0){uint w,h;float k,limite;uint autok;float ganho;uint mw,mh;uint nrost,rp0,rp1,rp2;float4 rost[4];};
Texture2D<float4> src:register(t0);Texture2D<float4> aux:register(t1);RWTexture2D<float4> dst:register(u0);
SamplerState lin:register(s0);
float Y(float3 c){return dot(c,float3(0.2126,0.7152,0.0722));}
float Yat(Texture2D<float4> t,int2 q){return Y(t.Load(int3(clamp(q,int2(0,0),int2(int(w)-1,int(h)-1)),0)).rgb);}
float fino(Texture2D<float4> t,int2 p){float s=0;[unroll]for(int dy=-1;dy<=1;++dy)[unroll]for(int dx=-1;dx<=1;++dx)s+=Yat(t,p+int2(dx,dy))*((dx==0?2.0:1.0)*(dy==0?2.0:1.0));return abs(Yat(t,p)-s/16.0);}
groupshared float eg[64],er[64];
[numthreads(8,8,1)]void energia(uint3 g:SV_GroupID,uint3 t:SV_GroupThreadID,uint gi:SV_GroupIndex){
 float a=0,b=0;int2 o=int2(g.xy)*32+int2(t.xy)*4;
 [unroll]for(int y=0;y<4;y+=2)[unroll]for(int x=0;x<4;x+=2){int2 p=o+int2(x,y);if(p.x<int(w)&&p.y<int(h)){a+=fino(src,p);b+=fino(aux,p);}}
 eg[gi]=a;er[gi]=b;GroupMemoryBarrierWithGroupSync();
 [unroll]for(uint n=32;n>0;n>>=1){if(gi<n){eg[gi]+=eg[gi+n];er[gi]+=er[gi+n];}GroupMemoryBarrierWithGroupSync();}
 if(gi==0){float kk=clamp(ganho*(er[0]/max(eg[0],1e-4)-1.0),0.0,k);dst[g.xy]=float4(kk,0,0,0);}
}
[numthreads(8,8,1)]void main(uint3 id:SV_DispatchThreadID){
 if(id.x>=w||id.y>=h)return;
 float kk=k;if(autok!=0){float2 uv=(float2(id.xy)+0.5)/(32.0*float2(mw,mh));kk=aux.SampleLevel(lin,uv,0).r;}
 if(nrost>0u){float pr=0.0;
  for(uint q=0;q<nrost&&q<4u;++q){float d=rost[q].w>0.0?length((float2(id.xy)+0.5-rost[q].xy)/max(rost[q].zw,1.0)):length((float2(id.xy)+0.5-rost[q].xy)/max(rost[q].z,1.0));pr=max(pr,1.0-smoothstep(0.85,1.15,d));}
  kk*=1.0-pr;}
 float4 c=src.Load(int3(id.xy,0));float y=Y(c.rgb);float s=0;float mn=y,mx=y;
 [unroll]for(int dy=-1;dy<=1;++dy)[unroll]for(int dx=-1;dx<=1;++dx){
  float v=Yat(src,int2(id.xy)+int2(dx,dy));s+=v*((dx==0?2.0:1.0)*(dy==0?2.0:1.0));mn=min(mn,v);mx=max(mx,v);}
 float b=s/16.0;float y2=clamp(y+kk*(y-b),mn-limite*(mx-mn),mx+limite*(mx-mn));
 float3 o=y>1e-5?c.rgb*(y2/y):c.rgb+(y2-y);
 dst[id.xy]=float4(max(o,0),c.a);}
)";
struct Estado{
 std::mutex m;bool tentou=false,falhou=false;float k=0.f;float limite=0.25f;bool autok=false;float ganho=2.7f;std::wstring ini;
 ComPtr<ID3D12Device> dev;ComPtr<ID3D12RootSignature> root;ComPtr<ID3D12PipelineState> pso,pso_energia;ComPtr<ID3D12Resource> mapa;unsigned mapa_w=0,mapa_h=0;D3D12_RESOURCE_STATES mapa_estado=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;ComPtr<ID3D12DescriptorHeap> heap;
 ComPtr<ID3D12Resource> temp;D3D12_RESOURCE_DESC temp_desc{};unsigned slot=0;unsigned long long feitos=0;

 nrd3d::Aposentados<32> aposentados;unsigned aplicacoes=0;
 void aposentar(ComPtr<ID3D12Resource>& r){if(aposentados.aposentar(r,aplicacoes))log_aposentados_cheios();}
 void soltar_aposentados(){++aplicacoes;aposentados.soltar(aplicacoes);}
 static void log_aposentados_cheios();
};

inline Estado& estado_(){static Estado* e=new Estado;return *e;}
inline void log(const char* t){
(void)t;
}
inline void Estado::log_aposentados_cheios(){log("APOSENTADOS cheios: recurso trocado fica vivo (nao solto em voo)");}

inline float forca(const std::wstring& ini){
 auto& e=estado_();
 if(!e.tentou){e.tentou=true;e.ini=ini;

  double v=0.;nrinijogo::ler_wcstod_em(ini.c_str(),L"NeuralFgNitidez",v,64);e.k=(v>0.&&v<=2.)?float(v):0.f;
  double l=0.;nrinijogo::ler_wcstod_em(ini.c_str(),L"NeuralFgNitidezLimite",l,64);if(l>0.&&l<=4.)e.limite=float(l);
  e.autok=NRCFG_INT(L"DlssNrOnAmd",L"NeuralFgNitidezAuto",0,ini.c_str())==1;
  double gg=0.;nrinijogo::ler_wcstod_em(ini.c_str(),L"NeuralFgNitidezGanho",gg,64);if(gg>0.&&gg<=10.)e.ganho=float(gg);
  char s[200];NRLOGF(s,sizeof s,"FG_NITIDEZ NeuralFgNitidez=%.3f limite=%.3f auto=%d ganho=%.2f",e.k,e.limite,int(e.autok),e.ganho);log(s);}
 return e.falhou?0.f:e.k;}

inline std::atomic<bool> teste_compilar{false};
inline const char* teste_hlsl=nullptr;
inline bool preparar(Estado& e,ID3D12Device* d){
 if(e.pso&&e.dev.Get()==d)return true;
 e.dev=d;ComPtr<ID3DBlob> cs,cs2,err;D3D12_SHADER_BYTECODE b_main{},b_energia{};bool embutido=false;
#if defined(NR_FG_NITIDEZ_CS) && NR_FG_NITIDEZ_CS
 if(!teste_compilar.load()){embutido=true;b_main={g_nr_fg_nitidez_main,sizeof(g_nr_fg_nitidez_main)};b_energia={g_nr_fg_nitidez_energia,sizeof(g_nr_fg_nitidez_energia)};}
#endif
 if(!embutido){
 HMODULE c=LoadLibraryExW(L"d3dcompiler_47.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);    if(!c){log("FALHA d3dcompiler_47 ausente");return false;}
 auto compila=reinterpret_cast<decltype(&D3DCompile)>(GetProcAddress(c,"D3DCompile"));if(!compila){log("FALHA D3DCompile");return false;}
 if(FAILED(compila(teste_hlsl?teste_hlsl:hlsl,std::strlen(teste_hlsl?teste_hlsl:hlsl),"fg_nitidez",nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&cs,&err))||
  FAILED(compila(teste_hlsl?teste_hlsl:hlsl,std::strlen(teste_hlsl?teste_hlsl:hlsl),"fg_nitidez",nullptr,nullptr,"energia","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&cs2,&err))){log("FALHA compilacao");return false;}
 b_main={cs->GetBufferPointer(),cs->GetBufferSize()};b_energia={cs2->GetBufferPointer(),cs2->GetBufferSize()};
 }
 if(!nrd3d::raiz_filtro(d,28,2,1,e.root)){log("FALHA root");return false;}
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=e.root.Get();pd.CS=b_main;
 if(FAILED(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(&e.pso)))){log("FALHA pso");return false;}
 pd.CS=b_energia;if(FAILED(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(&e.pso_energia)))){log("FALHA pso energia");return false;}
 D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,48,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
 if(FAILED(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&e.heap)))){log("FALHA heap");return false;}
 log(embutido?"FG_NITIDEZ pronto (bytecode do build)":"FG_NITIDEZ pronto (HLSL compilado agora)");return true;}
inline void teste_recriar(){auto& e=estado_();std::lock_guard<std::mutex> l(e.m);e.pso.Reset();e.pso_energia.Reset();e.root.Reset();e.dev.Reset();}

inline void aplicar(void* descriptor,const std::wstring& ini) noexcept {
 try{
  const float k=forca(ini);if(k<=0.f)return;
  if(nrgpu::perdida())return;
  auto& e=estado_();std::lock_guard<std::mutex> l(e.m);if(e.falhou)return;
  auto* g=static_cast<FgDesc*>(descriptor);auto* lista=static_cast<ID3D12GraphicsCommandList*>(g->commandList);
  if(!lista)return;e.soltar_aposentados();const unsigned n=g->numGenerated>=1&&g->numGenerated<=4?g->numGenerated:1;
  for(unsigned i=0;i<n;++i){
   auto* out=static_cast<ID3D12Resource*>(g->outputs[i].resource);if(!out)continue;
   ComPtr<ID3D12Device> d;if(FAILED(out->GetDevice(IID_PPV_ARGS(&d))))continue;
   if(!preparar(e,d.Get())){e.falhou=true;return;}
   const auto od=out->GetDesc();const auto vf=vista(od.Format);
   if(od.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||vf==DXGI_FORMAT_UNKNOWN||!(od.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)){
    char s[160];NRLOGF(s,sizeof s,"FALHA saida do FG sem UAV tipado: formato=%u flags=0x%x",unsigned(od.Format),unsigned(od.Flags));log(s);e.falhou=true;return;}
   if(!e.temp||e.temp_desc.Width!=od.Width||e.temp_desc.Height!=od.Height||e.temp_desc.Format!=od.Format){
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC td=od;td.Flags=D3D12_RESOURCE_FLAG_NONE;td.MipLevels=1;td.Alignment=0;
    e.aposentar(e.temp);if(FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&e.temp)))){log("FALHA textura");e.falhou=true;return;}
    e.temp_desc=od;char s[160];NRLOGF(s,sizeof s,"FG_NITIDEZ saida %llux%u formato=%u estado_ffx=0x%x",static_cast<unsigned long long>(od.Width),od.Height,unsigned(od.Format),g->outputs[i].state);log(s);}
   const auto inc=d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

   const unsigned base=(e.slot++%8u)*6u;
   auto h=[&](unsigned i){auto c=e.heap->GetCPUDescriptorHandleForHeapStart();c.ptr+=SIZE_T(base+i)*inc;return c;};
   auto hg=[&](unsigned i){auto c=e.heap->GetGPUDescriptorHandleForHeapStart();c.ptr+=UINT64(base+i)*inc;return c;};
   D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=vf;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.Texture2D.MipLevels=1;
   D3D12_UNORDERED_ACCESS_VIEW_DESC uv{};uv.Format=vf;uv.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
   const unsigned mw=(unsigned(od.Width)+31u)/32u,mh=(od.Height+31u)/32u;
   auto* real=static_cast<ID3D12Resource*>(g->presentColor.resource);
   bool adapt=e.autok&&real;
   if(adapt){const auto rd=real->GetDesc();if(rd.Width!=od.Width||rd.Height!=od.Height||vista(rd.Format)==DXGI_FORMAT_UNKNOWN)adapt=false;}

   if(estado(g->outputs[i].state)==nrd3d::estado_recusado)continue;
   if(adapt&&estado(g->presentColor.state)==nrd3d::estado_recusado)adapt=false;
   if(lista->GetType()==D3D12_COMMAND_LIST_TYPE_COMPUTE){
    if(!nrffx_estado::valido_em_compute(static_cast<std::uint32_t>(estado(g->outputs[i].state))))continue;
    if(adapt&&!nrffx_estado::valido_em_compute(static_cast<std::uint32_t>(estado(g->presentColor.state))))adapt=false;}
   if(adapt&&(!e.mapa||e.mapa_w!=mw||e.mapa_h!=mh)){
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC md{};md.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;md.Width=mw;md.Height=mh;md.DepthOrArraySize=1;md.MipLevels=1;
    md.Format=DXGI_FORMAT_R32_FLOAT;md.SampleDesc.Count=1;md.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    e.aposentar(e.mapa);if(FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&md,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&e.mapa)))){log("FALHA mapa");e.falhou=true;return;}
    e.mapa_w=mw;e.mapa_h=mh;e.mapa_estado=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;}
   d->CreateShaderResourceView(e.temp.Get(),&sv,h(0));d->CreateShaderResourceView(e.temp.Get(),&sv,h(3));
   d->CreateUnorderedAccessView(out,nullptr,&uv,h(5));
   if(adapt){
    auto svr=sv;svr.Format=vista(real->GetDesc().Format);d->CreateShaderResourceView(real,&svr,h(1));
    D3D12_UNORDERED_ACCESS_VIEW_DESC um{};um.Format=DXGI_FORMAT_R32_FLOAT;um.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;d->CreateUnorderedAccessView(e.mapa.Get(),nullptr,&um,h(2));
    auto sm=sv;sm.Format=DXGI_FORMAT_R32_FLOAT;d->CreateShaderResourceView(e.mapa.Get(),&sm,h(4));
   }else{d->CreateShaderResourceView(e.temp.Get(),&sv,h(4));}
   const auto s0=estado(g->outputs[i].state);
   auto tr=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){nrd3d::transicao(lista,r,a,b);};
   if(s0==D3D12_RESOURCE_STATE_UNORDERED_ACCESS){D3D12_RESOURCE_BARRIER u{};u.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;u.UAV.pResource=out;lista->ResourceBarrier(1,&u);}
   tr(out,s0,D3D12_RESOURCE_STATE_COPY_SOURCE);
   D3D12_TEXTURE_COPY_LOCATION a{},b{};a.pResource=e.temp.Get();a.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;b.pResource=out;b.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
   lista->CopyTextureRegion(&a,0,0,0,&b,nullptr);
   tr(out,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);tr(e.temp.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
   ID3D12DescriptorHeap* hs[]{e.heap.Get()};lista->SetDescriptorHeaps(1,hs);
   lista->SetComputeRootSignature(e.root.Get());

   struct{unsigned w,h;float k,lim;unsigned autok;float ganho;unsigned mw,mh;unsigned nrost,rp[3];float rost[16];}c{unsigned(od.Width),od.Height,k,e.limite,adapt?1u:0u,e.ganho,mw,mh,0u,{},{}};
   static_assert(sizeof(c)==28*4,"28 constantes");
   {const auto rq=nrrostosquadro::ler();for(unsigned r=0;r<rq.n&&r<4u;++r){if(!(rq.a[r].rx>1.f))continue;float* q=c.rost+4u*c.nrost++;q[0]=rq.a[r].x;q[1]=rq.a[r].y;q[2]=rq.a[r].rx;q[3]=rq.a[r].ry>1.f?rq.a[r].ry:0.f;}}
   lista->SetComputeRoot32BitConstants(0,28,&c,0);
   const auto sr=estado(g->presentColor.state);
   if(adapt){
    tr(real,sr,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);tr(e.mapa.Get(),e.mapa_estado,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    lista->SetPipelineState(e.pso_energia.Get());lista->SetComputeRootDescriptorTable(1,hg(0));lista->Dispatch(mw,mh,1);
    tr(e.mapa.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);e.mapa_estado=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    tr(real,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,sr);
   }
   lista->SetPipelineState(e.pso.Get());lista->SetComputeRootDescriptorTable(1,hg(3));lista->Dispatch((c.w+7)/8,(c.h+7)/8,1);
   {D3D12_RESOURCE_BARRIER u{};u.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;u.UAV.pResource=out;lista->ResourceBarrier(1,&u);}
   tr(out,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,s0);tr(e.temp.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
   if(++e.feitos==1||e.feitos%6000==0){char s[96];NRLOGF(s,sizeof s,"FG_NITIDEZ aplicada %llu quadros gerados",e.feitos);log(s);}
  }
 }catch(...){estado_().falhou=true;}
}
}
