#include "../../../runtime/nr_publico.h"
#include "../../../runtime/custo_perfil.h"
#include "../../../runtime/tempo_gpu.h"
#include "backend.h"
#include "../../../runtime/rosto_jogador_state.h"
#include "policy.h"
#include "late_compose.h"
#include "formatos.h"
#include "reproject_controls.h"
#include "reproject_chaves.h"
#include "estab_fontes.h"
#include "../../../runtime/estencil_externo.h"
#include "../compose-controls/realce.h"
#include "../compat-diag/compat.h"
#include "../../../runtime/estado_ffx.h"
#include "../../../runtime/jogo_conhecido.h"
#include "../../../runtime/pre_scale_state.h"
#include "../../../runtime/ini_jogo.h"
#include "../../../runtime/log_fila.h"
#include "../../../runtime/orcamento_vram.h"
#include "../../../runtime/post_teto.h"
#include "../../../runtime/exposicao_fresca.h"
#include "post_historico.h"
#include "nr_reproject_capture.h"
#include "nr_reproject_reduzir.h"
#include "nr_reproject_warp.h"
#include "nr_reproject_compose.h"
#include "nr_reproject_composepost.h"
#include "nr_reproject_estabilizar.h"
#include "nr_reproject_estabilizarcr.h"
#include "nr_reproject_croma.h"
#include "nr_reproject_cromalds.h"
#include "nr_reproject_suave.h"
#include "nr_reproject_luzgrossa.h"
#include "nr_reproject_basetom.h"
#include "nr_reproject_cromabase.h"
#include "nr_reproject_cromaldsbase.h"
#include "nr_reproject_luzgrossabase.h"
#include "nr_reproject_tlmederef.h"
#include "../../../runtime/expo_trava.h"
#include "../../../runtime/protege_realce.h"
#include "suave_config.h"
#include "nr_reproject_fill.h"
#include "nr_reproject_fillrapido.h"
#include "nr_reproject_luzmomentos.h"
#include "nr_reproject_luzcoeficientes.h"
#include "nr_reproject_luztemporal.h"
#include "nr_reproject_luzprox.h"
#include "nr_reproject_luzhalo.h"
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "../../../runtime/d3d12_comum.h"
#include "../../../runtime/gpu_perdida.h"
#include "../present-runtime/print_state.h"
#include <array>
#include <atomic>
#include <thread>
#include <mutex>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <cwchar>
#include <algorithm>
#include <limits>
#include <vector>

struct Thresholds { float depth_absolute, motion_reference; };
const Thresholds& thresholds() {
    static const Thresholds value = []{
        auto read=[](const wchar_t* key,float fallback){
            double parsed{};
            if(!nrinijogo::ler_double(key,parsed)||parsed<0)return fallback;
            return float(parsed);
        };
        return Thresholds{read(L"NeuralReprojectDepthAbsolute",.0001f),

                          read(L"NeuralReprojectMotionReference",.01f)};
    }();

    static const bool seeded=[]{
        unsigned indices[nrreproject::ControlCount]{};
        for(unsigned i=0;i<nrreproject::ControlCount;++i){wchar_t t[32]{};nrinijogo::ler_texto(nrreproject::chaves[i],t,32);(void)nrreproject::indice_do_ini(i,t,indices[i]);}
        return nrreproject::semear_indices(nrreproject::controls,indices);
    }();
    (void)seeded;
    return value;
}

namespace nrreproject {namespace {void rj_limpa_preparar(ID3D12GraphicsCommandList* c,ID3D12Resource* dst,unsigned estado,std::uint64_t quadro) noexcept;}}
namespace nrreproject {
namespace {
using Microsoft::WRL::ComPtr;
constexpr auto read_state=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
constexpr auto write_state=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
struct Constants {
    unsigned width,height,slice,age,mask,count,flags,pad;
    float scale_x,scale_y,jitter_x,jitter_y,inverse_x,inverse_y,depth_relative,depth_absolute;
    float color_threshold,current_exposure,source_exposure,residual_exposure;
    float clamp_strength,confidence_rate;
    float motion_reference;
    float motion_divergence;

    float deform_tolerance;

    float gate_smooth;

    float deform_blur;

    float detail_motion;

    float gate_coherent;

    float occlusion_tolerance;

    float residual_floor;
    float confidence_floor;
};
static_assert(sizeof(Constants)==128);

struct ComposeConstants{unsigned tone,width,height;float exposure;unsigned estilo;float tom,ev,contraste,saturacao;unsigned p0,p1,p2;
                        float prof_rel,prof_abs;unsigned p3,p4;};
static_assert(sizeof(ComposeConstants)==64);

struct LuzConstants{ComposeConstants k;float alfa0,delta,eps;unsigned bits;};
static_assert(sizeof(LuzConstants)==80);

constexpr unsigned compose_variante=12,compose_uavs=6;

constexpr unsigned luz_variante=19,luz_uavs=10;

static_assert(nrffx_estado::NON_PIXEL==D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE&&nrffx_estado::PIXEL==D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE&&
              nrffx_estado::UAV==D3D12_RESOURCE_STATE_UNORDERED_ACCESS&&nrffx_estado::RT==D3D12_RESOURCE_STATE_RENDER_TARGET&&
              nrffx_estado::DEPTH_WRITE==D3D12_RESOURCE_STATE_DEPTH_WRITE&&nrffx_estado::DEPTH_READ==D3D12_RESOURCE_STATE_DEPTH_READ&&
              nrffx_estado::INDIRECT==D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT&&nrffx_estado::COPY_DEST==D3D12_RESOURCE_STATE_COPY_DEST&&
              nrffx_estado::COPY_SOURCE==D3D12_RESOURCE_STATE_COPY_SOURCE&&nrffx_estado::COMMON==D3D12_RESOURCE_STATE_COMMON&&
              nrffx_estado::leitura==unsigned(read_state)&&nrffx_estado::escrita==unsigned(write_state));
std::atomic<std::uint64_t> estado_tolerado_visto[16]{};
D3D12_RESOURCE_STATES state(unsigned s){
 const auto t=nrffx_estado::traduzir(s);
 if(t.tolerado&&s<1024u){const std::uint64_t bit=1ull<<(s&63u);
  if(!(estado_tolerado_visto[s>>6].fetch_or(bit,std::memory_order_relaxed)&bit)){char d[96];
   std::snprintf(d,sizeof d,"estado_ffx=0x%x -> d3d12=0x%x",s,unsigned(t.d3d12));nrcompat::tolerado("tolerado_estado_ffx",d);}}
 return t.d3d12==nrffx_estado::recusa?D3D12_RESOURCE_STATES(~0u):D3D12_RESOURCE_STATES(t.d3d12);}
void transition(ID3D12GraphicsCommandList* c,ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){nrd3d::transicao(c,r,a,b);}
void order_uav(ID3D12GraphicsCommandList* c,ID3D12Resource* r){D3D12_RESOURCE_BARRIER v{};v.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;v.UAV.pResource=r;c->ResourceBarrier(1,&v);}
bool device_matches(ID3D12Device* d,ID3D12DeviceChild* r){ComPtr<ID3D12Device> v;return r&&SUCCEEDED(r->GetDevice(IID_PPV_ARGS(&v)))&&v.Get()==d;}

namespace expoanel {

ComPtr<ID3D12Resource> leitura,envio;ID3D12Device* device_do_anel{};std::uint32_t* envio_mapa{};std::uint32_t contador{0};
std::atomic<bool> falhou{false};
bool ler(unsigned slot,float& e,std::uint32_t& etiqueta){
 auto* r=leitura.Get();if(!r||slot>=nrexpofresca::slots)return false;
 const D3D12_RANGE faixa{SIZE_T(slot)*nrexpofresca::passo,SIZE_T(slot)*nrexpofresca::passo+nrexpofresca::etiqueta_em+4};void* m{};
 if(FAILED(r->Map(0,&faixa,&m))||!m)return false;
 const auto* b=static_cast<const unsigned char*>(m)+SIZE_T(slot)*nrexpofresca::passo;
 std::memcpy(&e,b,4);std::memcpy(&etiqueta,b+nrexpofresca::etiqueta_em,4);
 const D3D12_RANGE nada{0,0};r->Unmap(0,&nada);return true;
}
bool preparar(ID3D12Device* d){
 if(falhou.load())return false;
 if(leitura){return d==device_do_anel;}
 D3D12_HEAP_PROPERTIES hl{};hl.Type=D3D12_HEAP_TYPE_READBACK;D3D12_HEAP_PROPERTIES he{};he.Type=D3D12_HEAP_TYPE_UPLOAD;
 D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
 bd.Width=UINT64(nrexpofresca::slots)*nrexpofresca::passo;
 ComPtr<ID3D12Resource> l,u;
 if(FAILED(d->CreateCommittedResource(&hl,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&l)))){falhou=true;return false;}nrd3d::nomear(l.Get(),L"nr_expo_leitura");
 bd.Width=UINT64(nrexpofresca::slots)*256u;
 if(FAILED(d->CreateCommittedResource(&he,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&u)))){falhou=true;return false;}nrd3d::nomear(u.Get(),L"nr_expo_envio");
 void* m{};const D3D12_RANGE nada{0,0};if(FAILED(u->Map(0,&nada,&m))||!m){falhou=true;return false;}
 leitura=l;envio=u;envio_mapa=static_cast<std::uint32_t*>(m);device_do_anel=d;
 nrexpofresca::ler_slot.store(&ler,std::memory_order_release);
 nrexpofresca::log("ANEL pronto: 64 slots de leitura (exposicao + etiqueta)");
 return true;
}

void gravar(ID3D12GraphicsCommandList* c,ID3D12Device* d,ID3D12Resource* exposicao,D3D12_RESOURCE_STATES estado){
 nrexpofresca::etiqueta_pendente.store(0,std::memory_order_relaxed);
 if(!c||!d||!exposicao||!preparar(d))return;
 std::uint32_t etiqueta=++contador;if(!etiqueta)etiqueta=++contador;
 const unsigned slot=etiqueta%nrexpofresca::slots;
 envio_mapa[slot*64u]=etiqueta;
 const auto origem_estado=D3D12_RESOURCE_STATE_COPY_SOURCE;
 if(estado!=origem_estado){D3D12_RESOURCE_BARRIER v{};v.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;v.Transition={exposicao,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,estado,origem_estado};c->ResourceBarrier(1,&v);}
 D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=leitura.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
 dst.PlacedFootprint.Offset=UINT64(slot)*nrexpofresca::passo;dst.PlacedFootprint.Footprint={DXGI_FORMAT_R32_FLOAT,1,1,1,256};
 src.pResource=exposicao;src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.SubresourceIndex=0;
 c->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
 c->CopyBufferRegion(leitura.Get(),UINT64(slot)*nrexpofresca::passo+nrexpofresca::etiqueta_em,envio.Get(),UINT64(slot)*256u,4);
 if(estado!=origem_estado){D3D12_RESOURCE_BARRIER v{};v.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;v.Transition={exposicao,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,origem_estado,estado};c->ResourceBarrier(1,&v);}
 nrexpofresca::etiqueta_pendente.store(etiqueta,std::memory_order_release);
}
}

bool texture(ID3D12Resource* r,DXGI_FORMAT f){if(!r)return false;auto d=r->GetDesc();return d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Format==f&&d.Width&&d.Height&&d.Width<=nrformato::lado_maximo&&d.Height<=nrformato::lado_maximo&&d.DepthOrArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1;}

using nrformato::vista_cor;using nrformato::bits_cor;using nrformato::vista_profundidade;using nrformato::vista_movimento;using nrformato::entrada;

bool format_support(ID3D12Device* d,DXGI_FORMAT f,bool uav){
 const LUID luid=d->GetAdapterLuid();
 struct Resposta{LUID a;DXGI_FORMAT f;bool uav,ok;};static Resposta cache[8]{};static unsigned n{},proximo{};static std::mutex m;
 {std::lock_guard<std::mutex> l(m);for(unsigned i=0;i<n;++i)if(cache[i].a.LowPart==luid.LowPart&&cache[i].a.HighPart==luid.HighPart&&cache[i].f==f&&cache[i].uav==uav)return cache[i].ok;}
 D3D12_FEATURE_DATA_FORMAT_SUPPORT q{f};if(FAILED(d->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&q,sizeof(q))))return false;
 const bool ok=(q.Support1&D3D12_FORMAT_SUPPORT1_SHADER_LOAD)&&(!uav||((q.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD)&&(q.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE)));
 std::lock_guard<std::mutex> l(m);cache[proximo]={luid,f,uav,ok};proximo=(proximo+1)%8u;if(n<8)++n;return ok;}

struct CachePso{
 std::mutex m;ComPtr<ID3D12Device> dev;
 struct Raiz{unsigned k,s,u;ComPtr<ID3D12RootSignature> r;};std::vector<Raiz> raizes;
 struct Pso{ID3D12RootSignature* r;const void* b;ComPtr<ID3D12PipelineState> p;};std::vector<Pso> psos;
 std::uint64_t criados{},reusados{};
 void do_dispositivo(ID3D12Device* d){if(dev.Get()!=d){raizes.clear();psos.clear();dev=d;}}
};
CachePso& cache_pso(){static CachePso* c=new CachePso;return *c;}
bool root(ID3D12Device* d,unsigned constants,unsigned srvs,unsigned uavs,ComPtr<ID3D12RootSignature>& result){
 auto& cache=cache_pso();std::lock_guard<std::mutex> trava(cache.m);cache.do_dispositivo(d);
 for(const auto& x:cache.raizes)if(x.k==constants&&x.s==srvs&&x.u==uavs){result=x.r;++cache.reusados;return true;}
 D3D12_DESCRIPTOR_RANGE ranges[2]{};ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,srvs,0,0,0};ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,uavs,0,0,0};
 D3D12_ROOT_PARAMETER p[3]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[0].Constants={0,0,constants};
 for(unsigned i=0;i<2;++i){p[i+1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[i+1].DescriptorTable={1,&ranges[i]};}
 D3D12_STATIC_SAMPLER_DESC s{};s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxLOD=D3D12_FLOAT32_MAX;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
 D3D12_ROOT_SIGNATURE_DESC rd{3,p,1,&s,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob>b,e;
 const bool ok=SUCCEEDED(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&e))&&SUCCEEDED(d->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&result)));
 if(ok){nrorc_vram::registrar(nrorc::Api::Raiz,0,"raiz");cache.raizes.push_back({constants,srvs,uavs,result});++cache.criados;}
 return ok;
}
template<class T,std::size_t N>bool pipeline(ID3D12Device* d,ID3D12RootSignature* r,const T(&bytes)[N],ComPtr<ID3D12PipelineState>& p){
 auto& cache=cache_pso();std::lock_guard<std::mutex> trava(cache.m);cache.do_dispositivo(d);
 for(const auto& x:cache.psos)if(x.r==r&&x.b==static_cast<const void*>(bytes)){p=x.p;++cache.reusados;return true;}
 D3D12_COMPUTE_PIPELINE_STATE_DESC q{};q.pRootSignature=r;q.CS={bytes,sizeof(bytes)};const bool ok=SUCCEEDED(d->CreateComputePipelineState(&q,IID_PPV_ARGS(&p)));
 if(ok){nrorc_vram::registrar(nrorc::Api::Pso,0,"pso");cache.psos.push_back({r,static_cast<const void*>(bytes),p});++cache.criados;}
 return ok;}

void registrar_realce(const char* modo,unsigned antes,unsigned depois,float luz,float detalhe){
(void)modo;(void)antes;(void)depois;(void)luz;(void)detalhe;
}

unsigned teto_unico(unsigned tone){
    const float luz=nrreproject::value(nrreproject::controls,nrreproject::LightGuard),det=nrreproject::value(nrreproject::controls,nrreproject::DetailGuard);
    const unsigned t=nrrealce::tom(tone,luz,det);registrar_realce("post_teto_unico",tone,t,luz,det);return t;
}

bool ler_ini(const wchar_t* chave,double& valor){return nrinijogo::ler_double(chave,valor);}

unsigned protege_realce_codigo(){
 static const unsigned c=[]{const unsigned k=nrprotreal::codigo(nrprotreal::ler([](const wchar_t* n,double& v){return ler_ini(n,v);}));
  nrlogfila::linha(L"nr_reproject.log","pid=%lu PROTEGE_REALCE %s ini=%.2f fim=%.2f\n",GetCurrentProcessId(),k?"ligada":"desligada",double(nrprotreal::ini_de(k)),double(nrprotreal::fim_de(k)));
  return k;}();
 return c;}

unsigned bits_cor_efetivo(DXGI_FORMAT vista){
 const unsigned b=bits_cor(vista);if(b)return b;
 const unsigned a=nrreproject::cor_bits_adaptada.load(std::memory_order_relaxed);if(a)return a;
 return nrreproject::cor_ffx_srgb.load(std::memory_order_relaxed)?(1u<<14):0u;
}
bool prof_normal(){
 static const int chave=[]{double v=-1.;if(!ler_ini(L"NeuralProfundidadeNormal",v))v=-1.;return v<-.5?-1:v>.5?1:0;}();
 static const bool conhecido=[]{wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);return nrjogo::conhecido(exe);}();
 const bool sim=chave==1||(chave<0&&!conhecido&&nrreproject::prof_invertida_jogo.load(std::memory_order_relaxed)==0);
 static std::atomic<bool> avisado{false};
 if(sim&&!avisado.exchange(true))nrcompat::tolerado("tolerado_profundidade_normal",chave==1?"NeuralProfundidadeNormal=1":"jogo declara Z normal ao FSR: captura usa 1-d");
 return sim;
}

bool q_chave(const wchar_t* k){double v=0.;return ler_ini(k,v)&&v>.5;}

constexpr unsigned despejo_maximo=120;
unsigned despejos_reservados{};
struct Despejo { unsigned restantes{},espera{},intervalo{},contador{};float ganho{16.f};bool profundidade{},etapas_fill{}; };
Despejo ler_despejo(){
 return Despejo{};
}

bool ler_pontual(){double v{};return ler_ini(L"NeuralReprojectPointMotion",v)&&v==1.;}

float ler_luz_eps(){double v{};return ler_ini(L"NeuralLightGuideEps",v)&&v>=1e-5&&v<=1.?float(v):.01f;}
float ler_luz_delta(){double v{};return ler_ini(L"NeuralLightSmoothDelta",v)&&v>=1e-3&&v<=10.?float(v):.1f;}

unsigned ler_luz_raio(){double v{};return ler_ini(L"NeuralLightGuideRadius",v)&&v>=0&&v<=4&&v==std::floor(v)?unsigned(v):2u;}

unsigned ler_raio_ini(const wchar_t* chave,unsigned padrao,unsigned menor,unsigned maior){double v{};
 return ler_ini(chave,v)&&v>=menor&&v<=maior&&v==std::floor(v)?unsigned(v):padrao;}

struct HaloAjuste{float forca;unsigned raio;float degrau;};
HaloAjuste ler_halo(){
 HaloAjuste a{-1.f,0u,.05f};double v{};
 if(ler_ini(L"NeuralHaloStrength",v)&&v>=0&&v<=1)a.forca=float(v);
 if(ler_ini(L"NeuralHaloRadius",v)&&v>=1&&v<=15&&v==std::floor(v))a.raio=unsigned(v);
 if(ler_ini(L"NeuralHaloDepthStep",v)&&v>=.005&&v<=.25)a.degrau=float(v);
 return a;
}

unsigned halo_bits(unsigned nivel,const HaloAjuste& ini,double rede_pct=100.){
 if(nivel==0||nivel>3)return 0u;
 static constexpr float forca[4]{0.f,.5f,.75f,1.f};static constexpr unsigned raio[4]{0u,4u,6u,8u};
 const float f=ini.forca>=0.f?ini.forca:forca[nivel];const unsigned r=nrreproject::halo_raio_rede(ini.raio?ini.raio:raio[nivel],rede_pct);
 const unsigned fb=unsigned(std::lround(std::clamp(f,0.f,1.f)*255.f)),db=unsigned(std::lround(std::clamp(ini.degrau,.005f,.25f)*1000.f));
 return 4u|(fb<<8)|((r&15u)<<16)|((db&255u)<<20)|((r&16u)<<2);
}

unsigned modo_preenchimento(){return nrreproject::value(nrreproject::controls,nrreproject::FillMode)>1.f?2u:0u;}
float ler_preenchimento(){double v{};return ler_ini(L"NeuralReprojectFill",v)&&v>0&&v<=1.?float(v):0.f;}
float meio_para_float(unsigned short h){
    const unsigned sinal=(h>>15)&1u,expoente=(h>>10)&31u,mantissa=h&1023u;
    float v;
    if(expoente==0)v=std::ldexp(float(mantissa),-24);
    else if(expoente==31)v=mantissa?std::numeric_limits<float>::quiet_NaN():std::numeric_limits<float>::infinity();
    else v=std::ldexp(float(mantissa+1024u),int(expoente)-25);
    return sinal?-v:v;
}

unsigned char amostra_movimento(unsigned short h,float pixels_por_unidade){
    const float v=meio_para_float(h)*pixels_por_unidade*8.f;
    if(!std::isfinite(v))return 0;
    const float m=std::fabs(v)+.5f;
    return m>=255.5f?255:(unsigned char)(m);
}
unsigned char amostra(unsigned short h,float ganho){
    const float v=meio_para_float(h)*ganho;
    if(!std::isfinite(v))return 0;
    const float m=std::fabs(v)+.5f;
    return m>=255.5f?255:(unsigned char)(m);
}
}
struct Backend::Impl {

 const char* captura_relatada{};const char* composicao_relatada{};

 bool nova(nrcompat::Origem origem,const char* relatada,const char* id)const noexcept{
  return !(relatada&&std::strcmp(relatada,id)==0&&nrcompat::blocked(origem));
 }
 void relatar(nrcompat::Origem origem,const char*& relatada,const char* id,const char* detalhe,const char* campo){
  relatada=id;nrcompat::block(origem,id,detalhe,campo);
 }

 void recuperar(nrcompat::Origem origem,const char*& relatada){relatada=nullptr;nrcompat::unblock(origem);}
 Policy policy;ComPtr<ID3D12Device> device;ComPtr<ID3D12RootSignature> temporal_root,compose_root,post_root;
 ComPtr<ID3D12PipelineState> capture_pso,warp_pso,compose_pso;
 ComPtr<ID3D12DescriptorHeap> temporal_heap,compose_heap;
 std::array<ComPtr<ID3D12Resource>,3> history;
 ComPtr<ID3D12Resource> warped,flags_buffer;

 ComPtr<ID3D12RootSignature> fill_root;ComPtr<ID3D12PipelineState> fill_pso;ComPtr<ID3D12DescriptorHeap> fill_heap;

 ComPtr<ID3D12Resource> fill_copia;ComPtr<ID3D12PipelineState> fill_rapido_pso;D3D12_RESOURCE_STATES fill_copia_estado{};

 ComPtr<ID3D12RootSignature> est_root;ComPtr<ID3D12PipelineState> est_pso;ComPtr<ID3D12DescriptorHeap> est_heap;
 ComPtr<ID3D12Resource> est[2];D3D12_RESOURCE_STATES est_estado[2]{};unsigned est_cur{};bool est_valido{};

 ComPtr<ID3D12Resource> ganho[2];D3D12_RESOURCE_STATES ganho_estado[2]{};unsigned ganho_cur{};bool ganho_ok{},ganho_valido{};std::uint64_t ganho_quadro{},ganho_limitados{};

 bool ganho_dividido{};

 ComPtr<ID3D12RootSignature> luz_root;ComPtr<ID3D12PipelineState> luz_mom_pso,luz_coef_pso,luz_temp_pso;ComPtr<ID3D12DescriptorHeap> luz_heap;

 ComPtr<ID3D12PipelineState> luz_prox_pso,luz_halo_pso;ComPtr<ID3D12Resource> luz_prox,luz_halo;
 unsigned halo_teto_raio{},halo_teto_quadros{};
 unsigned halo_exc_r100{ler_raio_ini(L"NeuralHaloExcessRadius",12u,4u,24u)};
 unsigned halo_exc_raio{};std::uint64_t halo_exc_quadros{};

 unsigned corte_raio_comp{};std::uint64_t corte_quadros{};float idade_media{};
 ComPtr<ID3D12Resource> luz_mom,luz_base,luz_ab,luz_coef,luz_hist[2];unsigned luz_cur{},luz_qw{},luz_qh{};bool luz_ok{},luz_valido{};
 std::uint64_t luz_quadro{},luz_quadros{},luz_filtrados{};
 float luz_eps{ler_luz_eps()},luz_delta{ler_luz_delta()};unsigned luz_raio{ler_luz_raio()};
 HaloAjuste halo_ini{ler_halo()};std::uint64_t halo_quadros{};unsigned halo_ultimo{};
 std::uint64_t luz_prof_quadros{};double halo_rede_pct{100.};
 float preenchimento{ler_preenchimento()};

 bool rj_sem_job{};int rj_slot{-1};int rj_grade{-1};
 int rj_slotn[nrrj::npc_slots]{-1,-1,-1,-1,-1,-1,-1};
 bool pontual{ler_pontual()};

 bool portao_exposicao=[]{double v=1.;const bool desligado=ler_ini(L"NeuralReprojectExposureGate",v)&&v==0.;
  static std::atomic<bool> dito{false};if(desligado&&!dito.exchange(true)){{nrlogfila::linha(L"nr_reproject.log","pid=%lu EXPOSURE_GATE desligado (NeuralReprojectExposureGate=0): alfa da captura = 1\n",GetCurrentProcessId());}}
  return !desligado;}();
 unsigned width{},height{},stride{};bool histories_read{},warped_read{},ready{},composed_frame{};void* captured_command{};

 unsigned comp_w{},comp_h{};bool reduzido{},res_reduzido_read{};ComPtr<ID3D12Resource> res_reduzido;ComPtr<ID3D12PipelineState> reduzir_pso;ComPtr<ID3D12DescriptorHeap> reduzir_heap;
 double pct_raio()const{return nrreproject::percent_raio(nrprescale::efetivo.load(std::memory_order_relaxed),comp_w,width);}

 Extensao post_relatado{};bool post_atual{};float post_estab=[]{double v=.10;if(!ler_ini(L"NeuralPostStabilize",v)||!(v>=0.&&v<1.))v=.10;return float(v);}();

 float post_alargar=[]{double v=2.;if(!ler_ini(L"NeuralPostStabilizeWiden",v)||!(v>=0.&&v<=8.))v=2.;return float(v);}();

 float cro_alfa=[]{double v=0.;if(!ler_ini(L"NeuralCromaEstavel",v)||!(v>0.&&v<1.))v=1.;return float(v);}();
 float cro_mov=[this]{double v=cro_alfa;if(!ler_ini(L"NeuralCromaEstavelMov",v)||!(v>0.&&v<=1.))v=cro_alfa;return float(v);}();
 float cro_ref=[]{double v=0.;if(!ler_ini(L"NeuralCromaEstavelRef",v)||!(v>=0.&&v<=64.))v=0.;return float(v);}();
 float cro_delta=[]{double v=.05;if(!ler_ini(L"NeuralCromaDelta",v)||!(v>=0.&&v<=4.))v=.05;return float(v);}();
 unsigned cro_raio=[]{double v=2.;if(!ler_ini(L"NeuralCromaRaio",v)||!(v>=0.&&v<=8.))v=2.;return unsigned(v+.5);}();
 unsigned cro_passo=[]{double v=2.;if(!ler_ini(L"NeuralCromaPasso",v)||!(v>=1.&&v<=8.))v=2.;return unsigned(v+.5);}();
 float cro_sigma=[]{double v=.15;if(!ler_ini(L"NeuralCromaSigma",v)||!(v>0.&&v<=8.))v=.15;return float(v);}();
 float cro_limite=[]{double v=0.;if(!ler_ini(L"NeuralCromaLimite",v)||!(v>=0.&&v<=4.))v=0.;return float(v);}();
 bool cro_ligado()const{return cro_alfa<1.f||cro_limite>0.f;}

 float lg_alfa=[]{double v=0.;if(!ler_ini(L"NeuralLuzGrossa",v)||!(v>0.&&v<1.))v=1.;return float(v);}();
 float lg_mov=[this]{double v=lg_alfa;if(!ler_ini(L"NeuralLuzGrossaMov",v)||!(v>0.&&v<=1.))v=lg_alfa;return float(v);}();
 float lg_ref=[]{double v=8.;if(!ler_ini(L"NeuralLuzGrossaRef",v)||!(v>=0.&&v<=64.))v=8.;return float(v);}();
 float lg_delta=[]{double v=.5;if(!ler_ini(L"NeuralLuzGrossaDelta",v)||!(v>=0.&&v<=8.))v=.5;return float(v);}();
 unsigned lg_raio=[]{double v=3.;if(!ler_ini(L"NeuralLuzGrossaRaio",v)||!(v>=1.&&v<=8.))v=3.;return unsigned(v+.5);}();
 unsigned lg_passo=[]{double v=6.;if(!ler_ini(L"NeuralLuzGrossaPasso",v)||!(v>=1.&&v<=32.))v=6.;return unsigned(v+.5);}();
 float lg_limite=[]{double v=.15;if(!ler_ini(L"NeuralLuzGrossaLimite",v)||!(v>0.&&v<=2.))v=.15;return float(v);}();
 float lg_teto=[]{double v=0.;if(!ler_ini(L"NeuralLuzGrossaTeto",v)||!(v>0.&&v<=4.))v=0.;return float(v);}();
 float lg_piso=[]{double v=0.;if(!ler_ini(L"NeuralLuzGrossaPiso",v)||!(v>0.&&v<=4.))v=0.;return float(v);}();
 float lg_cor=[]{double v=0.;if(!ler_ini(L"NeuralLuzGrossaCor",v))v=0.;return v>.5?1.f:0.f;}();
 bool lg_ligado()const{return lg_alfa<1.f;}
 ComPtr<ID3D12RootSignature> lg_root;ComPtr<ID3D12PipelineState> lg_pso;ComPtr<ID3D12DescriptorHeap> lg_heap;
 ComPtr<ID3D12Resource> lg[2],lg_out;D3D12_RESOURCE_STATES lg_estado[2]{},lg_out_estado{};unsigned lg_cur{};bool lg_valido{};std::uint64_t lg_quadro{},lg_quadros{};

 bool base_tom_lig=[]{double v=0.;return ler_ini(L"NeuralCustoBaseTom",v)&&v>.5;}();
 ComPtr<ID3D12RootSignature> base_tom_root;ComPtr<ID3D12PipelineState> base_tom_pso;ComPtr<ID3D12DescriptorHeap> base_tom_heap;ComPtr<ID3D12Resource> base_tom;
 D3D12_RESOURCE_STATES base_tom_estado{};std::uint64_t base_tom_quadros{};

 unsigned tl_bloco=[]{double v=8.;if(!ler_ini(L"NeuralTravaLuzBloco",v)||!(v>=4.&&v<=32.))v=8.;return unsigned(v+.5);}();
 ComPtr<ID3D12RootSignature> tl_root;ComPtr<ID3D12PipelineState> tl_pso[1];ComPtr<ID3D12DescriptorHeap> tl_heap;ComPtr<ID3D12Resource> tl_tex[1];
 unsigned tl_w{},tl_h{},tl_cl{},tl_ch{};bool tl_falhou{};ID3D12Resource* tl_dst{};

 bool cfg_croma_lds=[]{double v=0.;return ler_ini(L"NeuralCustoCromaLds",v)&&v>.5;}();

 bool cfg_near_norm=[]{double v=0;return ler_ini(L"NeuralReprojectNearNorm",v)&&v==1.;}();
 float cfg_halo_teto_forca=[]{double v=0;return ler_ini(L"NeuralHaloTeto",v)&&v>0.?float(std::min(v,1.)):0.f;}();
 double cfg_halo_teto_raio=[]{double v=16;return ler_ini(L"NeuralHaloTetoRaio",v)&&v>=2.&&v<=30.?v:16.;}();
 unsigned cfg_halo_teto_forca15=[]{double v=0;return ler_ini(L"NeuralHaloTeto",v)&&v>0.?unsigned(std::lround(std::min(v,1.)*15.)):0u;}();

 float tl_media=[]{double v=0.;return ler_ini(L"NeuralExposicaoMedia",v)?nrexpotrava::faixa_media(v):0.f;}();

 float tl_tela=[]{double v=0.;return ler_ini(L"NeuralExposicaoTela",v)?nrexpotrava::faixa_tela(v):0.f;}();
 float tl_media_fator=[]{double v=1.2;const bool tem=ler_ini(L"NeuralExposicaoMediaFator",v);return nrexpotrava::faixa_media_fator(v,tem);}();

 bool cfg_diag_f9=[]{
  return false;
 }();
 ComPtr<ID3D12Resource> diag_warp,diag_cena,diag_rostos;ComPtr<ID3D12DescriptorHeap> diag_heap;bool diag_falhou{};
 bool diag_gravando() const noexcept {return cfg_diag_f9&&!diag_falhou&&nrreproject::despejo_final.load(std::memory_order_relaxed)>0u;}
 void diag_rodar_warp(ID3D12GraphicsCommandList* c,ID3D12Resource* res_warp,DXGI_FORMAT res_fmt){
  if(!diag_gravando()||!res_warp||!warp_pso||!temporal_root||!warped)return;
  if(!diag_warp){
   D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC td=warped->GetDesc();td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
   if(!criar_heap(12,diag_heap)||FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,write_state,nullptr,IID_PPV_ARGS(&diag_warp)))){diag_falhou=true;diag_warp.Reset();
    nrlogfila::linha(L"nr_reproject.log","pid=%lu DIAG_F9 falhou (heap/textura do Warp de diagnostico)\n",GetCurrentProcessId());return;}
   nrd3d::nomear(diag_warp.Get(),L"nr_diag_warp");
   const DXGI_FORMAT formats[]{DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R16G16_FLOAT,DXGI_FORMAT_R32_FLOAT};
   srv(diag_heap.Get(),0,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);srv(diag_heap.Get(),1,nullptr,DXGI_FORMAT_R16G16_FLOAT);srv(diag_heap.Get(),2,nullptr,DXGI_FORMAT_R32_FLOAT);
   for(unsigned i=0;i<3;++i){srv(diag_heap.Get(),3+i,history[i].Get(),formats[i],true);uav(diag_heap.Get(),8+i,history[i].Get(),formats[i],true);}
   srv(diag_heap.Get(),7,nullptr,DXGI_FORMAT_R32_FLOAT);uav(diag_heap.Get(),11,diag_warp.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
   nrlogfila::linha(L"nr_reproject.log","pid=%lu DIAG_F9 ligado: lados warpdiag/realce/cena na F9 (rede %ux%u)\n",GetCurrentProcessId(),width,height);
  }else transition(c,diag_warp.Get(),read_state,write_state);
  srv(diag_heap.Get(),6,res_warp,res_fmt);
  Constants k=constants;k.pad|=1u;
  bind(c,diag_heap.Get(),temporal_root.Get(),warp_pso.Get(),32,&k,8);c->Dispatch((width+7)/8,(height+7)/8,1);
  transition(c,diag_warp.Get(),write_state,read_state);
  nrreproject::diag_warp_recurso.store(diag_warp.Get());
 }

 void diag_copiar(ID3D12GraphicsCommandList* c,ID3D12Resource* dst,ComPtr<ID3D12Resource>& alvo,std::atomic<void*>& destino,const wchar_t* nome){
  if(!diag_gravando()||!dst)return;
  const auto dd=dst->GetDesc();
  if(alvo){const auto cd=alvo->GetDesc();if(cd.Width!=dd.Width||cd.Height!=dd.Height||cd.Format!=dd.Format)return;}
  if(!alvo){D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC td=dd;td.Flags=D3D12_RESOURCE_FLAG_NONE;td.Alignment=0;
   if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read_state,nullptr,IID_PPV_ARGS(&alvo)))){diag_falhou=true;return;}
   nrd3d::nomear(alvo.Get(),nome);}
  transition(c,dst,write_state,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(c,alvo.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);
  c->CopyResource(alvo.Get(),dst);
  transition(c,alvo.Get(),D3D12_RESOURCE_STATE_COPY_DEST,read_state);transition(c,dst,D3D12_RESOURCE_STATE_COPY_SOURCE,write_state);
  destino.store(alvo.Get());
 }
 ComPtr<ID3D12Resource> tl_rb[4];std::uint64_t tl_rb_n{};unsigned tl_rb_pitch{};std::uint64_t tl_medidas{};
 static float tl_enc(float v){v=v/(1.f+v);return v<=.0031308f?v*12.92f:1.055f*std::pow(v,1.f/2.4f)-.055f;}
 void tl_ler_media(unsigned slot){
  void* m=nullptr;const D3D12_RANGE r{0,SIZE_T(tl_rb_pitch)*tl_ch};if(FAILED(tl_rb[slot]->Map(0,&r,&m))||!m)return;
  constexpr int NB=144;unsigned h[NB]{};unsigned n=0;
  for(unsigned y=0;y<tl_ch;y+=2){const float* l=reinterpret_cast<const float*>(static_cast<const char*>(m)+SIZE_T(y)*tl_rb_pitch);
   for(unsigned x=0;x<tl_cl;x+=2){const float v=l[4*x+3];if(!(v>=0.f)||!std::isfinite(v))continue;const float l2=std::log2((std::max)(v,1e-8f));
    const int b=std::clamp(int((l2+24.f)*4.f),0,NB-1);++h[b];++n;}}
  if(ta_ativo){float mx=0.f;double s=0.;unsigned k=0;
   for(unsigned y=0;y<tl_ch;++y){const float* l=reinterpret_cast<const float*>(static_cast<const char*>(m)+SIZE_T(y)*tl_rb_pitch);
    for(unsigned x=0;x<tl_cl;++x){const float v=l[4*x+3];if(!(v>=0.f)||!std::isfinite(v))continue;mx=(std::max)(mx,v);s+=v;++k;}}
   if(k>=64)ta_passo(mx,float(s/k));}
  const D3D12_RANGE nada{0,0};tl_rb[slot]->Unmap(0,&nada);
  if(n<64||!(tl_media>0.f))return;
  auto media=[&](float e){double s=0;for(int b=0;b<NB;++b)if(h[b])s+=double(h[b])*tl_enc(std::exp2((float(b)+.5f)/4.f-24.f)*e);return s/n;};
  float alvo=tl_media,tm=0.f;
  if(tl_tela>0.f){tm=nrexpotrava::tela_media.load(std::memory_order_relaxed);const auto tms=nrexpotrava::tela_ms.load(std::memory_order_relaxed);
   if(tm>0.f&&GetTickCount64()-tms<1500)alvo=std::clamp(tl_tela*tm,.02f,.6f);}
  float lo=1e-4f,hi=1e4f;for(int i=0;i<40;++i){const float mid=std::sqrt(lo*hi);if(media(mid)<alvo)lo=mid;else hi=mid;}
  const float e=std::sqrt(lo*hi)*tl_media_fator;
  if(std::isfinite(e)&&e>0.f){nrexpotrava::medida_expo.store(e,std::memory_order_relaxed);nrexpotrava::medida_ms.store(GetTickCount64(),std::memory_order_relaxed);}
  if((tl_medidas++&255u)==0)nrlogfila::linha(L"nr_reproject.log","pid=%lu EXPO_MEDIDA exposicao=%.4f alvo=%.3f tela=%.3f fator=%.2f blocos=%u medidas=%llu\n",GetCurrentProcessId(),double(e),double(alvo),double(tm),double(tl_media_fator),n,static_cast<unsigned long long>(tl_medidas));
 }
 void tl_copiar_media(ID3D12GraphicsCommandList* c){
  if(tl_tela>0.f&&tl_media>0.f)nrexpotrava::tela_ligada.store(1,std::memory_order_relaxed);
  if(!(tl_media>0.f||ta_ativo)||(tl_rb_n++&3u))return;
  const std::uint64_t k=tl_rb_n>>2;const unsigned slot=unsigned(k&3u);
  if(!tl_rb[slot]){D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;tl_rb_pitch=(tl_cl*16u+255u)&~255u;
   D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=UINT64(tl_rb_pitch)*tl_ch;bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
   if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&tl_rb[slot]))))return;nrd3d::nomear(tl_rb[slot].Get(),L"nr_expo_tela_leitura");}
  else if(k>=4)tl_ler_media(slot);
  transition(c,tl_tex[0].Get(),write_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
  D3D12_TEXTURE_COPY_LOCATION de{};de.pResource=tl_rb[slot].Get();de.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  de.PlacedFootprint.Footprint={DXGI_FORMAT_R32G32B32A32_FLOAT,tl_cl,tl_ch,1,tl_rb_pitch};
  D3D12_TEXTURE_COPY_LOCATION fo{};fo.pResource=tl_tex[0].Get();fo.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;fo.SubresourceIndex=0;
  c->CopyTextureRegion(&de,0,0,0,&fo,nullptr);
  transition(c,tl_tex[0].Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,write_state);
 }
 struct TlK{unsigned largura,altura,cl,ch,bloco,raio,p0,p1;float cima,baixo,forca,eps;};

 int ta_chave=[]{double v=-1.;if(!ler_ini(L"NeuralTomAuto",v))v=-1.;return v<-.5?-1:v>1.5?2:v>.5?1:0;}();
 int ta_forca=[]{double v=-1.;return ler_ini(L"NeuralTomAutoForca",v)&&(v==0.||v==1.)?int(v):-1;}();
 unsigned ta_validas{};float ta_max{};bool ta_voltou{},ta_ativo{};
 bool ta_mede(){
  if(ta_forca>=0)return false;
  if(ta_chave==0||ta_voltou)return false;
  static const bool conhecido=[]{wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);return nrjogo::conhecido(exe);}();
  if(conhecido&&ta_chave<0)return false;
  if(ta_chave<2&&nrreproject::tom_ini.load(std::memory_order_relaxed)!=-1)return false;
  return nrreproject::tom_pedido.load(std::memory_order_relaxed)==0||nrreproject::tom_ef.load(std::memory_order_relaxed)==1;
 }
 void ta_passo(float maior,float media){
  if(!(media>.01f))return;
  const int p=nrreproject::tom_pedido.load(std::memory_order_relaxed);
  if(p!=0){++ta_validas;ta_max=(std::max)(ta_max,maior);
   if(ta_validas==1||ta_validas%64u==0)nrlogfila::linha(L"nr_reproject.log","pid=%lu TOM_AUTO medindo validas=%u maior=%.3f media=%.3f (desliga com 150 e maior <= 1)\n",GetCurrentProcessId(),ta_validas,double(ta_max),double(media));
   if(ta_validas>=150&&ta_max<=1.f){nrreproject::tom_pedido.store(0,std::memory_order_relaxed);
    char t[160];std::snprintf(t,sizeof t,"flag HDR com a cor de exibicao (maior bloco %.3f em %u medidas): tonemap da entrada da rede desligado (= Tonemap=0)",double(ta_max),ta_validas);
    nrcompat::tolerado("tolerado_cor_exibicao_hdr",t);nrlogfila::linha(L"nr_reproject.log","pid=%lu TOM_AUTO %s\n",GetCurrentProcessId(),t);}}
  else if(maior>1.5f){nrreproject::tom_pedido.store(1,std::memory_order_relaxed);ta_voltou=true;
   char t[128];std::snprintf(t,sizeof t,"bloco %.3f > 1,5 depois de desligar: tonemap automatico de volta",double(maior));
   nrcompat::tolerado("tolerado_cor_exibicao_hdr_volta",t);nrlogfila::linha(L"nr_reproject.log","pid=%lu TOM_AUTO %s\n",GetCurrentProcessId(),t);}
 }
 bool tl_preparar(ID3D12Resource* dst,DXGI_FORMAT f,unsigned w,unsigned h){
  ta_ativo=ta_mede();
  if(!(tl_media>0.f||ta_ativo)||tl_falhou||!dst)return false;

  if(f!=DXGI_FORMAT_R16G16B16A16_FLOAT&&f!=DXGI_FORMAT_R11G11B10_FLOAT){tl_falhou=true;nrlogfila::linha(L"nr_reproject.log","pid=%lu EXPO_MEDIDA desligada: formato da saida %d (so RGBA16F/R11G11B10)\n",GetCurrentProcessId(),int(f));return false;}
  if(!tl_root){
   if(!root(device.Get(),sizeof(TlK)/4,1,4,tl_root)||!pipeline(device.Get(),tl_root.Get(),g_nr_reproject_tlmederef,tl_pso[0])){
    tl_falhou=true;nrlogfila::linha(L"nr_reproject.log","pid=%lu EXPO_MEDIDA desligada: raiz/PSO recusados\n",GetCurrentProcessId());return false;}
   if(!criar_heap(5,tl_heap)){tl_falhou=true;return false;}
   D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT_R32_FLOAT;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.Texture2D.MipLevels=1;
   device->CreateShaderResourceView(nullptr,&sv,cpu(tl_heap.Get(),0));}
  if(w!=tl_w||h!=tl_h){
   tl_cl=(w+tl_bloco-1)/tl_bloco;tl_ch=(h+tl_bloco-1)/tl_bloco;
   D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;rd.Width=tl_cl;rd.Height=tl_ch;
   rd.DepthOrArraySize=1;rd.MipLevels=1;rd.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;rd.SampleDesc.Count=1;rd.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

   {Aposentadas lote;lote.vencimento=begins+3;auto drop=[&](ComPtr<ID3D12Resource>& r){if(r){lote.objetos.emplace_back(r.Get());r.Reset();}};
    for(auto& x:tl_rb)drop(x);drop(tl_tex[0]);if(!lote.objetos.empty())retired.push_back(std::move(lote));}
   tl_rb_n=0;
   if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,write_state,nullptr,IID_PPV_ARGS(&tl_tex[0])))){tl_falhou=true;return false;}nrd3d::nomear(tl_tex[0].Get(),L"nr_expo_tela");
   D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(tl_tex[0].Get(),nullptr,&v,cpu(tl_heap.Get(),2));

   device->CreateUnorderedAccessView(nullptr,nullptr,&v,cpu(tl_heap.Get(),3));device->CreateUnorderedAccessView(nullptr,nullptr,&v,cpu(tl_heap.Get(),4));
   tl_w=w;tl_h=h;tl_dst=nullptr;
   nrlogfila::linha(L"nr_reproject.log","pid=%lu EXPO_MEDIDA grade bloco=%u %ux%u grade=%ux%u\n",GetCurrentProcessId(),tl_bloco,w,h,tl_cl,tl_ch);}
  if(dst!=tl_dst){uav(tl_heap.Get(),1,dst,f);tl_dst=dst;}
  return true;
 }
 void tl_passo(ID3D12GraphicsCommandList* c,unsigned i,unsigned gx,unsigned gy){
  const TlK k{tl_w,tl_h,tl_cl,tl_ch,tl_bloco,0u,0u,0u,0.f,0.f,0.f,1e-5f};
  ID3D12DescriptorHeap* hs[]{tl_heap.Get()};c->SetDescriptorHeaps(1,hs);c->SetComputeRootSignature(tl_root.Get());c->SetPipelineState(tl_pso[i].Get());
  c->SetComputeRoot32BitConstants(0,sizeof(TlK)/4,&k,0);c->SetComputeRootDescriptorTable(1,gpu(tl_heap.Get(),0));c->SetComputeRootDescriptorTable(2,gpu(tl_heap.Get(),1));
  c->Dispatch((gx+7)/8,(gy+7)/8,1);
 }

 void tl_medir_jogo(ID3D12GraphicsCommandList* c,ID3D12Resource* dst,DXGI_FORMAT f,unsigned w,unsigned h){
  if(!tl_preparar(dst,f,w,h))return;
  tl_passo(c,0,tl_cl,tl_ch);order_uav(c,dst);order_uav(c,tl_tex[0].Get());
  tl_copiar_media(c);
 }

 unsigned segura_max=[]{double v=6.;if(!ler_ini(L"NeuralSeguraMax",v)||!(v>=1.&&v<=240.))v=6.;return unsigned(v+.5);}();
 unsigned segura_fade=[]{double v=0.;if(!ler_ini(L"NeuralSeguraFade",v)||!(v>=0.&&v<=240.))v=0.;return unsigned(v+.5);}();
 ComPtr<ID3D12RootSignature> cro_root;ComPtr<ID3D12PipelineState> cro_pso;ComPtr<ID3D12DescriptorHeap> cro_heap;
 ComPtr<ID3D12Resource> cro[2],cro_out;D3D12_RESOURCE_STATES cro_estado[2]{},cro_out_estado{};unsigned cro_cur{};bool cro_valido{};std::uint64_t cro_quadro{},cro_quadros{};

 nrsuave::Config su_cfg=nrsuave::ler([](const wchar_t* n,double& v){return ler_ini(n,v);});
 ComPtr<ID3D12RootSignature> su_root;ComPtr<ID3D12PipelineState> su_pso;ComPtr<ID3D12DescriptorHeap> su_heap;
 ComPtr<ID3D12Resource> su[2],su_out;D3D12_RESOURCE_STATES su_estado[2]{},su_out_estado{};unsigned su_cur{};bool su_valido{};std::uint64_t su_quadro{},su_quadros{};
 bool post_filtro_mov=[]{double v=1.;if(!ler_ini(L"NeuralPostMotionFilter",v))v=1.;return v>.5;}();

 float fg_troca_luz=[]{double v=0.;if(!ler_ini(L"NeuralFgTrocaLuz",v)||!(v>0.&&v<1.))v=0.;return float(v);}();
 float post_luz_mov=[]{double v=0.;if(!ler_ini(L"NeuralPostLuzMov",v)||!(v>0.&&v<1.))v=0.;return float(v);}();

 bool fg_troca_sempre=[]{double v=0.;return ler_ini(L"NeuralFgTrocaLuzSempre",v)&&v>.5;}();
 float est_luz_mov{};int est_ultimo_job{-1};int fg_log_estado{-1};
 nrestab::Fonte est_fonte_base{nrestab::Nenhuma},est_fonte_mov{nrestab::Ini};unsigned est_fontes_log{~0u},est_fontes_trocas{};
 bool fg_cross=[]{double v=0.;return ler_ini(L"NeuralFgCrossfade",v)&&v>.5;}();
 bool fg_cross_sempre=[]{double v=0.;return ler_ini(L"NeuralFgCrossfadeSempre",v)&&v>.5;}();
 float est_cross{},cross_P{};int cross_job{-1};unsigned cross_j{};

 bool post_estab_rosto=[]{double v=0.;return ler_ini(L"NeuralPostEstabRosto",v)&&v>.5;}();float est_rosto{};

 bool post_det_rampa=[]{double v=0.;return ler_ini(L"NeuralPostEstabDetalheRampa",v)&&v>.5;}();

 bool post_estab_buraco=[]{double v=0.;return ler_ini(L"NeuralPostEstabBuraco",v)&&v>.5;}();

 bool tempo_gpu=[]{double v=0.,n=600.;const bool on=ler_ini(L"NeuralTempoGpu",v)&&v>.5;if(!ler_ini(L"NeuralTempoGpuQuadros",n)||!(n>=30.&&n<=100000.))n=600.;
  nrtgpu::configurar(on,unsigned(n));return on;}();
 float post_delta_claro=[]{double v=0.;return ler_ini(L"NeuralPostDeltaClaroMax",v)&&v>0.&&v<=1.?float(v):0.f;}();
 float post_estab_luz=[]{double v=0.;if(!ler_ini(L"NeuralPostEstabLuz",v)||!(v>0.&&v<1.))v=0.;return float(v);}();
 float post_det_parado=[]{double v=.05;if(!ler_ini(L"NeuralPostEstabDetalheParado",v)||!(v>0.&&v<=4.))v=.05;return float(v);}();
 float post_alfa_mov=[]{double v=.5;if(!ler_ini(L"NeuralPostStabilizeMotion",v)||!(v>0.&&v<=1.))v=.5;return float(v);}();
 Constants constants{};std::uint64_t composed{},skipped{},unsupported{};

 LateState late;ID3D12DescriptorHeap* warp_heap_override{};bool suprimir_despejo{};

 ID3D12DescriptorHeap* reduzir_heap_override{};bool late_post=[]{double v=0.;return ler_ini(L"NeuralLateComposePost",v)&&v>.5;}();

 bool red_cache_lig=[]{double v=0.;return ler_ini(L"NeuralCustoReduzirCache",v)&&v>.5;}();

 bool red_cache_valido{};const void* red_cache_res{};const void* red_cache_dst{};int red_cache_job{-1};std::uint64_t red_cache_hits{},red_cache_misses{};

 ComPtr<ID3D12PipelineState> post_pso;ComPtr<ID3D12DescriptorHeap> post_heap;
 struct Pendente{bool ativo{};void* command{};int job{};float exposure{1};unsigned tone{};} post;
 bool post_recusado{},post_formato_logado{};std::uint64_t post_composed{},post_refused{},post_lost{};

 struct CenaNpcCfg{unsigned modo{},inicio{6},zonas{1},ref_n{10};float fixo{1.f},fmin{.4f},fmax{1.3f},alfa_f{.25f},alfa_ref{0.f},prof_a{1.5f},prof_b{3.f};bool viz{};};
 CenaNpcCfg cnpc_cfg=[]{CenaNpcCfg c{};double v=0.;

  if(ler_ini(L"NeuralCenaNpcNormaliza",v)&&v>=.5&&v<2.5){c.modo=v<1.5?1u:2u;if(c.modo==2u){c.fmin=.7f;c.fmax=1.5f;c.zonas=2u;c.alfa_f=.08f;}}
  else if(ler_ini(L"NeuralCenaNpcForca",v)&&v>=0.&&v<=2.){c.modo=3u;c.fixo=float(v);}
  auto f=[](const wchar_t* k,float& x,double lo,double hi){double t=0.;if(ler_ini(k,t)&&t>=lo&&t<=hi)x=float(t);};
  f(L"NeuralCenaNpcMin",c.fmin,0.,1.);f(L"NeuralCenaNpcMax",c.fmax,1.,4.);f(L"NeuralCenaNpcSuave",c.alfa_f,.01,1.);

  {double q=0.;if(ler_ini(L"NeuralCenaNpcRefLenta",q)&&q>=1.&&q<=100000.)c.alfa_ref=float(1./q);}
  {double q=0.;if(ler_ini(L"NeuralCenaNpcRefFixo",q)&&q>.5)c.alfa_ref=-1.f;}
  {double q=0.;if(ler_ini(L"NeuralCenaNpcRefMedia",q)&&q>=64.&&q<=960.)c.ref_n=unsigned(q/64.+.5);}
  {double q=0.;if(ler_ini(L"NeuralCenaNpcInicio",q)&&q>=1.&&q<=200.)c.inicio=unsigned(q+.5);}
  {double q=0.;if(ler_ini(L"NeuralCenaNpcZonas",q)&&q>=.5&&q<=6.5){const unsigned z=unsigned(q+.5);c.zonas=z>=6u?6u:z>=4u?4u:z>=2u?2u:1u;}}
  f(L"NeuralCenaNpcProfA",c.prof_a,.1,50.);f(L"NeuralCenaNpcProfB",c.prof_b,.2,100.);if(!(c.prof_b>c.prof_a))c.prof_b=c.prof_a+.5f;
  {double q=0.;c.viz=ler_ini(L"NeuralCenaNpcViz",q)&&q>.5;}
  return c;}();
 struct CenaNpc{ComPtr<ID3D12RootSignature> root;ComPtr<ID3D12PipelineState> mede,compor;ComPtr<ID3D12DescriptorHeap> heap;ComPtr<ID3D12Resource> estado,leitura,zeros;
  bool logado{},zerado{};std::uint64_t quadros{},usados{},leit_q{};std::uint64_t ger[8]{};bool on_ant[8]{};float cx_ant[8]{},cy_ant[8]{},t_ant[8]{};
  void soltar(){root.Reset();mede.Reset();compor.Reset();heap.Reset();estado.Reset();leitura.Reset();zeros.Reset();zerado=false;}} cnpc;
 bool alocar_cena_npc(const D3D12_UNORDERED_ACCESS_VIEW_DESC& flags_view);
 bool cena_npc(ID3D12GraphicsCommandList* c,ID3D12Resource* dst,DXGI_FORMAT fmt,const void* k);

 unsigned masc_bits=[]{double a=0.,b=0.;const bool ta=ler_ini(L"NeuralMascaraPersonagem",a),tb=ler_ini(L"NeuralFiltroMascara",b);const char* av=nullptr;
  const unsigned m=nrestext::bits_mascara(ta,a,tb,b,&av);
  if(av)nrlogfila::linha(L"nr_reproject.log","pid=%lu MASCARA_CHAVE %s: NeuralMascaraPersonagem=%s NeuralFiltroMascara=%.0f -> bits=0x%02X (a chave e NeuralMascaraPersonagem; NeuralFiltroMascara e sinonimo)\n",
   GetCurrentProcessId(),av,ta?"presente":"ausente",b,m);
  return m;}();
 float masc_raio=[]{double v=112.;if(!ler_ini(L"NeuralMascaraRaio",v)||!(v>=16.&&v<=256.))v=112.;return float(v);}();

 float masc_prof=[]{double v=0.;return ler_ini(L"NeuralMascaraProfundidade",v)&&v>1.&&v<=16.?float(v):0.f;}();

 bool masc_suave=[]{double v=0.;return ler_ini(L"NeuralMascaraSuave",v)&&v>=1.;}();

 struct Mascara:nrestext::Leitor{ComPtr<ID3D12RootSignature> root,post_root;ComPtr<ID3D12PipelineState> celulas,distancia,preenche,aplica,post;ComPtr<ID3D12DescriptorHeap> heap,post_heap;
  ComPtr<ID3D12Resource> cel,aux,dist,fill;unsigned cw{},ch{};
  ComPtr<ID3D12PipelineState> distz_pso,curva_pso,curvafim_pso,limita_pso;ComPtr<ID3D12Resource> distz,parcial,curva;std::uint64_t parcial_bytes{};
  ComPtr<ID3D12PipelineState> curvasoma_pso,bordas_pso;ComPtr<ID3D12Resource> bordas;
  ComPtr<ID3D12Resource> buf_velho;
  std::uint64_t quadros{},sem{},falhas{};const char* motivo_ult{};bool logado{};
  void soltar(){root.Reset();post_root.Reset();celulas.Reset();distancia.Reset();preenche.Reset();aplica.Reset();post.Reset();heap.Reset();post_heap.Reset();
   cel.Reset();aux.Reset();dist.Reset();fill.Reset();cw=ch=0;distz_pso.Reset();curva_pso.Reset();curvafim_pso.Reset();limita_pso.Reset();distz.Reset();parcial.Reset();curva.Reset();curvasoma_pso.Reset();bordas_pso.Reset();if(nrestext::bordas_quadro.tabela==bordas.Get())nrestext::bordas_quadro={};bordas.Reset();}} masc;
 bool alocar_mascara(const D3D12_UNORDERED_ACCESS_VIEW_DESC& flags_view);
 bool mascara_pronta();
 void mascara_halo(ID3D12GraphicsCommandList* c,bool personagem=true);
 bool mascara_post(ID3D12GraphicsCommandList* c,ID3D12Resource* dst,DXGI_FORMAT fmt,const void* k);
 void masc_srv_bruto(ID3D12DescriptorHeap* h,unsigned i,ID3D12Resource* r,std::uint64_t bytes);
 void masc_uav_bruto(ID3D12DescriptorHeap* h,unsigned i,ID3D12Resource* r,std::uint64_t bytes);

 unsigned trace_restantes=[]{double v=0;return ler_ini(L"NeuralCenaTrace",v)&&v>=1.&&v<=200000.?unsigned(v):0u;}();
 float trace_expo{};long long trace_qpc{};
 void trace(int job,int age,const char* reason){
(void)job;(void)age;(void)reason;
}

 void perder_post(const char* motivo){if(!post.ativo)return;post.ativo=false;++post_lost;if(post_lost<=3||(post_lost&255)==0)log_post(post.job,motivo,nullptr);}
 bool modo_post()const{return !reduzido&&!post_recusado&&post_pso&&post_heap&&nrreproject::value(nrreproject::controls,nrreproject::ComposeAfter)>.5f;}

 static constexpr unsigned anel_k=3;
 static constexpr GUID guid_sombra{0x6e7a1b52,0x9c31,0x4d0e,{0x8b,0x55,0x2a,0x41,0x7f,0x10,0xd3,0x22}};
 bool criar_heap(unsigned n,ComPtr<ID3D12DescriptorHeap>& h){
  h.Reset();if(!device||!n)return false;
  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,n*anel_k,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};ComPtr<ID3D12DescriptorHeap> s;
  if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&h))))return false;
  hd.NumDescriptors=n;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&s)))||FAILED(h->SetPrivateDataInterface(guid_sombra,s.Get()))){h.Reset();return false;}
  nrd3d::nomear(h.Get(),L"nr_cena_heap");nrd3d::nomear(s.Get(),L"nr_cena_heap_sombra");return true;}

 static ID3D12DescriptorHeap* sombra(ID3D12DescriptorHeap* h) noexcept {
  if(!h)return nullptr;IUnknown* u{};UINT tam=sizeof(u);if(FAILED(h->GetPrivateData(guid_sombra,&tam,&u))||!u)return nullptr;
  ID3D12DescriptorHeap* s{};const bool ok=SUCCEEDED(u->QueryInterface(IID_PPV_ARGS(&s)));u->Release();if(!ok)return nullptr;s->Release();return s;}
 unsigned copia_anel()const{return unsigned(begins%anel_k);}
 D3D12_CPU_DESCRIPTOR_HANDLE cpu(ID3D12DescriptorHeap* h,unsigned i){if(auto* s=sombra(h))h=s;auto v=h->GetCPUDescriptorHandleForHeapStart();v.ptr+=SIZE_T(i)*stride;return v;}

 void publicar(ID3D12DescriptorHeap* h,unsigned i){auto* s=sombra(h);if(!s)return;const unsigned n=s->GetDesc().NumDescriptors;
  auto d=h->GetCPUDescriptorHandleForHeapStart();d.ptr+=SIZE_T(copia_anel()*n+i)*stride;auto o=s->GetCPUDescriptorHandleForHeapStart();o.ptr+=SIZE_T(i)*stride;
  device->CopyDescriptorsSimple(1,d,o,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);}
 D3D12_GPU_DESCRIPTOR_HANDLE gpu(ID3D12DescriptorHeap* h,unsigned i){unsigned base=0;
  if(auto* s=sombra(h)){const unsigned n=s->GetDesc().NumDescriptors;base=copia_anel()*n;auto d=h->GetCPUDescriptorHandleForHeapStart();d.ptr+=SIZE_T(base)*stride;
   device->CopyDescriptorsSimple(n,d,s->GetCPUDescriptorHandleForHeapStart(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);}
  auto v=h->GetGPUDescriptorHandleForHeapStart();v.ptr+=UINT64(base+i)*stride;return v;}
 void srv(ID3D12DescriptorHeap* h,unsigned i,ID3D12Resource* r,DXGI_FORMAT f,bool array=false){D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=f;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;if(array){v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;v.Texture2DArray.MipLevels=1;v.Texture2DArray.ArraySize=Policy::history_count;}else{v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Texture2D.MipLevels=1;}device->CreateShaderResourceView(r,&v,cpu(h,i));publicar(h,i);}
 void uav(ID3D12DescriptorHeap* h,unsigned i,ID3D12Resource* r,DXGI_FORMAT f,bool array=false){D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=f;if(array){v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2DARRAY;v.Texture2DArray.ArraySize=Policy::history_count;}else v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(r,nullptr,&v,cpu(h,i));publicar(h,i);}

 struct Aposentadas { std::vector<ComPtr<IUnknown>> objetos;unsigned long long vencimento{}; };
 std::vector<Aposentadas> retired;unsigned long long begins{};

 Despejo despejo{ler_despejo()};

 struct DespejoPendente {
  ComPtr<ID3D12Resource> leitura;unsigned long long vencimento{};
  unsigned largura{},altura{},indice{},passo{};std::size_t bytes{};float ganho{};
  bool movimento{},fonte{},confianca{};
  bool profundidade{};unsigned fatia{},idade{};unsigned long long quadro{};
  float depth_relative{},depth_absolute{};
  ComPtr<ID3D12Resource> antes;bool raw{},antes_fill{},pontual{},fill_ativo{},par_solicitado{};float fill_limiar{};
 };
 std::vector<DespejoPendente> despejos;

 std::array<float,Policy::history_count*2> jitter_fatia{};std::array<bool,Policy::history_count> jitter_ok{};

 void retire(unsigned to_w,unsigned to_h,bool contar=true){
  policy.invalidate();
  if(contar)log_realocacao(to_w,to_h);

  Aposentadas lote;lote.vencimento=begins+3;
  auto drop=[&](auto& com){if(com){lote.objetos.emplace_back(com.Get());com.Reset();}};
  for(auto& h:history)drop(h);drop(warped);drop(flags_buffer);
  drop(diag_warp);drop(diag_cena);drop(diag_rostos);drop(diag_heap);diag_falhou=false;
  nrreproject::diag_warp_recurso.store(nullptr);nrreproject::diag_cena_recurso.store(nullptr);nrreproject::diag_realce_recurso.store(nullptr);nrreproject::diag_rostos_recurso.store(nullptr);
  drop(temporal_heap);drop(compose_heap);drop(temporal_root);drop(compose_root);drop(post_root);
  drop(ganho[0]);drop(ganho[1]);ganho_ok=ganho_valido=false;ganho_cur=0;
  drop(luz_root);drop(luz_mom_pso);drop(luz_coef_pso);drop(luz_temp_pso);drop(luz_heap);
  drop(luz_mom);drop(luz_base);drop(luz_ab);drop(luz_coef);drop(luz_hist[0]);drop(luz_hist[1]);luz_ok=luz_valido=false;luz_cur=0;
  drop(luz_prox_pso);drop(luz_halo_pso);drop(luz_prox);drop(luz_halo);
  drop(capture_pso);drop(warp_pso);drop(compose_pso);drop(post_pso);drop(post_heap);perder_post("post_lost_realocacao");
  drop(cnpc.root);drop(cnpc.mede);drop(cnpc.compor);drop(cnpc.heap);drop(cnpc.estado);drop(cnpc.leitura);drop(cnpc.zeros);cnpc.zerado=false;
  drop(masc.root);drop(masc.post_root);drop(masc.celulas);drop(masc.distancia);drop(masc.preenche);drop(masc.aplica);drop(masc.post);drop(masc.heap);drop(masc.post_heap);
  drop(masc.cel);drop(masc.aux);drop(masc.dist);drop(masc.fill);masc.cw=masc.ch=0;
  drop(masc.distz_pso);drop(masc.curva_pso);drop(masc.curvafim_pso);drop(masc.limita_pso);drop(masc.distz);drop(masc.parcial);drop(masc.curva);drop(masc.curvasoma_pso);drop(masc.bordas_pso);drop(masc.bordas);
  drop(fill_heap);drop(fill_root);drop(fill_pso);drop(fill_copia);drop(fill_rapido_pso);
  drop(res_reduzido);drop(reduzir_pso);drop(reduzir_heap);res_reduzido_read=false;reduzido=false;comp_w=comp_h=0;
  drop(est_heap);drop(est_root);drop(est_pso);drop(est[0]);drop(est[1]);est_valido=false;est_falhou=false;
  drop(cro_heap);drop(cro_root);drop(cro_pso);drop(cro[0]);drop(cro[1]);drop(cro_out);cro_valido=false;
  drop(lg_heap);drop(lg_root);drop(lg_pso);drop(lg[0]);drop(lg[1]);drop(lg_out);lg_valido=false;
  drop(base_tom_root);drop(base_tom_pso);drop(base_tom_heap);drop(base_tom);
  drop(tl_heap);drop(tl_root);for(auto& x:tl_pso)drop(x);for(auto& x:tl_tex)drop(x);for(auto& x:tl_rb)drop(x);tl_rb_n=0;tl_w=tl_h=0;tl_dst=nullptr;
  drop(su_heap);drop(su_root);drop(su_pso);drop(su[0]);drop(su[1]);drop(su_out);su_valido=false;

  histories_read=false;warped_read=false;
  width=height=0;ready=false;

  if(!lote.objetos.empty())retired.push_back(std::move(lote));
 }

 static constexpr std::uint64_t orcamento_bytes=2560ull*1024*1024;

 static constexpr std::uint64_t por_pixel=std::uint64_t(Policy::history_count)*16+8;

 static constexpr std::uint64_t orcamento_piso=1024ull*1024*1024,orcamento_reserva=1024ull*1024*1024;
 std::uint64_t orcamento_atual{orcamento_bytes};Extensao orcamento_para{};

 ComPtr<IDXGIAdapter3> adaptador;bool adaptador_tentado{};unsigned long long vram_tick{};int uma{-1};
 bool vram(std::uint64_t& uso,std::uint64_t& orc){
  if(!adaptador&&!adaptador_tentado&&device){adaptador_tentado=true;ComPtr<IDXGIFactory4> f;
   if(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&f))))(void)f->EnumAdapterByLuid(device->GetAdapterLuid(),IID_PPV_ARGS(&adaptador));}
  DXGI_QUERY_VIDEO_MEMORY_INFO i{};
  if(!adaptador||FAILED(adaptador->QueryVideoMemoryInfo(0,DXGI_MEMORY_SEGMENT_GROUP_LOCAL,&i)))return false;
  uso=i.CurrentUsage;orc=i.Budget;return true;
 }
 struct Recusa { unsigned w{},h{},alocada_w{},alocada_h{};std::uint64_t bytes{}; };
 Recusa recusa{};std::uint64_t recusas_por_limite{};

 unsigned segurados_seguidos{};std::uint64_t segurados_total{};
 bool pode_segurar(void* command)const{
  return ready&&est_pso&&est_valido&&segurados_seguidos<segura_max&&policy.captured&&!composed_frame&&command==captured_command&&
         (!reduzido||(post_pso&&post_heap));
 }

 struct PausaAloc{unsigned w{},h{},cw{},ch{};std::uint64_t orc{};unsigned long long ate{};unsigned quadros{};bool ativa{};} pausa_aloc;
 std::uint64_t alocacoes_tentadas{},alocacoes_adiadas{};
 static constexpr unsigned pausa_primeira=60u,pausa_maxima=960u;
 bool allocation(unsigned w,unsigned h,unsigned cw,unsigned ch){
  if(ready&&width==w&&height==h&&comp_w==cw&&comp_h==ch)return true;
  if(pausa_aloc.ativa){
   const bool mesma=pausa_aloc.w==w&&pausa_aloc.h==h&&pausa_aloc.cw==cw&&pausa_aloc.ch==ch&&pausa_aloc.orc==orcamento_atual;
   if(mesma&&begins<pausa_aloc.ate){++alocacoes_adiadas;return false;}
   if(!mesma)pausa_aloc.quadros=0;
  }
  nrorc_vram::registrar(nrorc::Api::RecriaCena,std::uint64_t(w)*h*por_pixel,"backend_allocation");

  const std::uint64_t custo=std::uint64_t(w)*h*por_pixel;

  recusa={w,h,width,height,custo};

  if(custo>orcamento_atual){++recusas_por_limite;return false;}
  if(width||height)retire(w,h);
  ++alocacoes_tentadas;
  if(alocar_geometria(w,h,cw,ch)){pausa_aloc={};return true;}
  const unsigned q=pausa_aloc.quadros?(std::min)(pausa_aloc.quadros*2u,pausa_maxima):pausa_primeira;
  pausa_aloc={w,h,cw,ch,orcamento_atual,begins+q,q,true};
  nrlogfila::linha(L"nr_reproject.log","pid=%lu frame=%llu ALOCACAO_FALHOU %ux%u saida=%ux%u nova_tentativa_em=%u capturas tentadas=%llu adiadas=%llu\n",
   GetCurrentProcessId(),policy.frame,w,h,cw,ch,q,static_cast<unsigned long long>(alocacoes_tentadas),static_cast<unsigned long long>(alocacoes_adiadas));
  retire(w,h,false);
  return false;
 }
 bool alocar_geometria(unsigned w,unsigned h,unsigned cw,unsigned ch){
  width=w;height=h;comp_w=cw;comp_h=ch;reduzido=(w!=cw||h!=ch);stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  if(!root(device.Get(),32,8,4,temporal_root)||!root(device.Get(),16,6,3,compose_root)||!root(device.Get(),12,2,2,post_root)||
     !pipeline(device.Get(),temporal_root.Get(),g_nr_reproject_capture,capture_pso)||!pipeline(device.Get(),temporal_root.Get(),g_nr_reproject_warp,warp_pso)||!pipeline(device.Get(),compose_root.Get(),g_nr_reproject_compose,compose_pso))return false;
  if(!criar_heap(12,temporal_heap)||!criar_heap(2*compose_variante,compose_heap))return false;
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=w;td.Height=h;td.DepthOrArraySize=Policy::history_count;td.MipLevels=1;td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  const DXGI_FORMAT formats[]{DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R16G16_FLOAT,DXGI_FORMAT_R32_FLOAT};
  for(unsigned i=0;i<3;++i){td.Format=formats[i];if(!format_support(device.Get(),td.Format,true)||FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,write_state,nullptr,IID_PPV_ARGS(&history[i]))))return false;nrd3d::nomear(history[i].Get(),i==0?L"nr_hist_cor":i==1?L"nr_hist_mv":L"nr_hist_prof");srv(temporal_heap.Get(),3+i,history[i].Get(),td.Format,true);uav(temporal_heap.Get(),8+i,history[i].Get(),td.Format,true);}

  {unsigned n=teste_falhar_alocacao.load();while(n&&!teste_falhar_alocacao.compare_exchange_weak(n,n-1u)){}if(n)return false;}
  td.DepthOrArraySize=1;td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,write_state,nullptr,IID_PPV_ARGS(&warped))))return false;nrd3d::nomear(warped.Get(),L"nr_warped");
  uav(temporal_heap.Get(),11,warped.Get(),td.Format);for(unsigned v=0;v<2;++v){srv(compose_heap.Get(),compose_variante*v,warped.Get(),td.Format);srv(compose_heap.Get(),compose_variante*v+1,warped.Get(),td.Format);}
  D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=4;bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;bd.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,write_state,nullptr,IID_PPV_ARGS(&flags_buffer))))return false;nrd3d::nomear(flags_buffer.Get(),L"nr_flags");
  D3D12_UNORDERED_ACCESS_VIEW_DESC bv{};bv.Format=DXGI_FORMAT_R32_TYPELESS;bv.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;bv.Buffer.NumElements=1;bv.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;for(unsigned v=0;v<2;++v)device->CreateUnorderedAccessView(flags_buffer.Get(),nullptr,&bv,cpu(compose_heap.Get(),compose_variante*v+compose_uavs+1));

  ganho_ok=alocar_ganho(!reduzido);ganho_valido=false;ganho_cur=0;

  luz_ok=!reduzido&&alocar_luz(bv);luz_valido=false;luz_cur=0;
  if(!luz_ok){luz_root.Reset();luz_mom_pso.Reset();luz_coef_pso.Reset();luz_temp_pso.Reset();luz_heap.Reset();luz_mom.Reset();luz_base.Reset();luz_ab.Reset();luz_coef.Reset();luz_hist[0].Reset();luz_hist[1].Reset();
   luz_prox_pso.Reset();luz_halo_pso.Reset();luz_prox.Reset();luz_halo.Reset();
   for(unsigned v=0;v<2;++v){srv(compose_heap.Get(),compose_variante*v+5,nullptr,DXGI_FORMAT_R32G32B32A32_FLOAT);srv(compose_heap.Get(),compose_variante*v+1,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);}}

  if(!pipeline(device.Get(),post_root.Get(),g_nr_reproject_composepost,post_pso)||!criar_heap(4,post_heap)){post_pso.Reset();post_heap.Reset();}
  else{srv(post_heap.Get(),0,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);srv(post_heap.Get(),1,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);device->CreateUnorderedAccessView(flags_buffer.Get(),nullptr,&bv,cpu(post_heap.Get(),3));}
  if(cnpc_cfg.modo&&reduzido&&post_pso&&post_heap&&!alocar_cena_npc(bv))cnpc.soltar();

  if(nrestext::aloca_mascara(masc_bits,masc_prof)&&reduzido&&post_pso&&post_heap&&!alocar_mascara(bv))masc.soltar();
  srv(temporal_heap.Get(),6,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);srv(temporal_heap.Get(),7,nullptr,DXGI_FORMAT_R32_FLOAT);

  if(preenchimento>0&&!alocar_preenchimento()){preenchimento=0.f;fill_heap.Reset();fill_root.Reset();fill_pso.Reset();fill_copia.Reset();fill_rapido_pso.Reset();}

  est_cur=0;est_valido=false;est_falhou=false;

  const bool base_ok=base_tom_lig&&(cro_ligado()||lg_ligado())&&alocar_base_tom();
  if(!base_ok){base_tom_root.Reset();base_tom_pso.Reset();base_tom_heap.Reset();base_tom.Reset();}

  if(cro_ligado()&&!alocar_croma(base_ok)){cro_pso.Reset();cro_heap.Reset();cro_root.Reset();cro[0].Reset();cro[1].Reset();cro_out.Reset();}
  if(lg_ligado()&&!alocar_luz_grossa(base_ok)){lg_pso.Reset();lg_heap.Reset();lg_root.Reset();lg[0].Reset();lg[1].Reset();lg_out.Reset();}

  if(su_cfg.ligado()&&!alocar_suave()){su_pso.Reset();su_heap.Reset();su_root.Reset();su[0].Reset();su[1].Reset();su_out.Reset();}

  if(reduzido&&!alocar_reduzir())return false;
  ready=true;return true;
 }

 bool est_falhou{};
 bool garantir_est(){
  if(est_pso)return true;
  if(est_falhou||!ready||!warped)return false;
  if(!alocar_estabilizador()){est_pso.Reset();est_heap.Reset();est_root.Reset();est[0].Reset();est[1].Reset();est_falhou=true;return false;}
  for(ID3D12DescriptorHeap* h:{cro_heap.Get(),lg_heap.Get(),su_heap.Get()})if(h)
   for(unsigned i=0;i<2;++i)for(unsigned f=1;f<=2;++f)for(unsigned d=0;d<2;++d)srv(h,filtro_variante(i,f,d),est[f-1].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
  return true;
 }
 bool alocar_reduzir(){
  if(!post_pso||!post_heap)return false;
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=width;td.Height=height;td.DepthOrArraySize=1;td.MipLevels=1;td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,write_state,nullptr,IID_PPV_ARGS(&res_reduzido))))return false;nrd3d::nomear(res_reduzido.Get(),L"nr_residuo_reduzido");
  res_reduzido_read=false;
  if(!pipeline(device.Get(),temporal_root.Get(),g_nr_reproject_reduzir,reduzir_pso))return false;
  if(!criar_heap(12,reduzir_heap))return false;

  const DXGI_FORMAT fs[]{DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R16G16_FLOAT,DXGI_FORMAT_R32_FLOAT};
  srv(reduzir_heap.Get(),0,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);srv(reduzir_heap.Get(),1,nullptr,DXGI_FORMAT_R16G16_FLOAT);srv(reduzir_heap.Get(),2,nullptr,DXGI_FORMAT_R32_FLOAT);
  for(unsigned i=0;i<3;++i){srv(reduzir_heap.Get(),3+i,nullptr,fs[i],true);uav(reduzir_heap.Get(),8+i,nullptr,fs[i],true);}
  srv(reduzir_heap.Get(),6,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);srv(reduzir_heap.Get(),7,nullptr,DXGI_FORMAT_R32_FLOAT);
  uav(reduzir_heap.Get(),11,res_reduzido.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
  return true;
 }
 bool alocar_ganho(bool criar=true){
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=width;td.Height=height;td.DepthOrArraySize=1;td.MipLevels=1;td.Format=DXGI_FORMAT_R16_FLOAT;td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  bool ok=criar;
  for(unsigned i=0;criar&&i<2;++i){if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read_state,nullptr,IID_PPV_ARGS(&ganho[i])))){ok=false;ganho[0].Reset();ganho[1].Reset();break;}nrd3d::nomear(ganho[i].Get(),L"nr_ganho");ganho_estado[i]=read_state;}
  for(unsigned v=0;v<2;++v){
   const unsigned o=compose_variante*v;
   srv(compose_heap.Get(),o+2,ok?ganho[1-v].Get():nullptr,DXGI_FORMAT_R16_FLOAT);
   srv(compose_heap.Get(),o+3,history[1].Get(),DXGI_FORMAT_R16G16_FLOAT,true);srv(compose_heap.Get(),o+4,history[2].Get(),DXGI_FORMAT_R32_FLOAT,true);
   uav(compose_heap.Get(),o+compose_uavs+2,ok?ganho[v].Get():nullptr,DXGI_FORMAT_R16_FLOAT);
  }
  return ok;
 }
 void ganho_para(ID3D12GraphicsCommandList* c,unsigned i,D3D12_RESOURCE_STATES e){transition(c,ganho[i].Get(),ganho_estado[i],e);ganho_estado[i]=e;}

 bool alocar_luz(const D3D12_UNORDERED_ACCESS_VIEW_DESC& bv){
  if(!root(device.Get(),20,10,9,luz_root)||!pipeline(device.Get(),luz_root.Get(),g_nr_reproject_luzmomentos,luz_mom_pso)||
     !pipeline(device.Get(),luz_root.Get(),g_nr_reproject_luzcoeficientes,luz_coef_pso)||!pipeline(device.Get(),luz_root.Get(),g_nr_reproject_luztemporal,luz_temp_pso)||
     !pipeline(device.Get(),luz_root.Get(),g_nr_reproject_luzprox,luz_prox_pso)||!pipeline(device.Get(),luz_root.Get(),g_nr_reproject_luzhalo,luz_halo_pso))return false;
  luz_qw=(width+3)/4;luz_qh=(height+3)/4;
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=luz_qw;td.Height=luz_qh;td.DepthOrArraySize=1;td.MipLevels=1;td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  auto criar=[&](DXGI_FORMAT f,ComPtr<ID3D12Resource>& r){td.Format=f;const bool ok=SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read_state,nullptr,IID_PPV_ARGS(&r)));if(ok)nrd3d::nomear(r.Get(),L"nr_luz");return ok;};
  if(!criar(DXGI_FORMAT_R32G32B32A32_FLOAT,luz_mom)||!criar(DXGI_FORMAT_R32_FLOAT,luz_base)||!criar(DXGI_FORMAT_R32G32_FLOAT,luz_ab)||
     !criar(DXGI_FORMAT_R32G32B32A32_FLOAT,luz_coef)||!criar(DXGI_FORMAT_R32G32_FLOAT,luz_hist[0])||!criar(DXGI_FORMAT_R32G32_FLOAT,luz_hist[1])||
     !criar(DXGI_FORMAT_R16_FLOAT,luz_prox)||!criar(DXGI_FORMAT_R32G32B32A32_FLOAT,luz_halo))return false;
  if(!criar_heap(2*luz_variante,luz_heap))return false;
  for(unsigned v=0;v<2;++v){
   const unsigned o=luz_variante*v,u=o+luz_uavs;
   srv(luz_heap.Get(),o,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);srv(luz_heap.Get(),o+1,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
   srv(luz_heap.Get(),o+2,luz_prox.Get(),DXGI_FORMAT_R16_FLOAT);
   srv(luz_heap.Get(),o+3,history[1].Get(),DXGI_FORMAT_R16G16_FLOAT,true);srv(luz_heap.Get(),o+4,history[2].Get(),DXGI_FORMAT_R32_FLOAT,true);
   srv(luz_heap.Get(),o+5,luz_coef.Get(),DXGI_FORMAT_R32G32B32A32_FLOAT);
   srv(luz_heap.Get(),o+6,luz_mom.Get(),DXGI_FORMAT_R32G32B32A32_FLOAT);srv(luz_heap.Get(),o+7,luz_base.Get(),DXGI_FORMAT_R32_FLOAT);
   srv(luz_heap.Get(),o+8,luz_ab.Get(),DXGI_FORMAT_R32G32_FLOAT);srv(luz_heap.Get(),o+9,luz_hist[1-v].Get(),DXGI_FORMAT_R32G32_FLOAT);
   uav(luz_heap.Get(),u,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);
   device->CreateUnorderedAccessView(flags_buffer.Get(),nullptr,&bv,cpu(luz_heap.Get(),u+1));
   uav(luz_heap.Get(),u+2,luz_prox.Get(),DXGI_FORMAT_R16_FLOAT);
   uav(luz_heap.Get(),u+3,luz_mom.Get(),DXGI_FORMAT_R32G32B32A32_FLOAT);uav(luz_heap.Get(),u+4,luz_base.Get(),DXGI_FORMAT_R32_FLOAT);
   uav(luz_heap.Get(),u+5,luz_ab.Get(),DXGI_FORMAT_R32G32_FLOAT);uav(luz_heap.Get(),u+6,luz_coef.Get(),DXGI_FORMAT_R32G32B32A32_FLOAT);
   uav(luz_heap.Get(),u+7,luz_hist[v].Get(),DXGI_FORMAT_R32G32_FLOAT);
   uav(luz_heap.Get(),u+8,luz_halo.Get(),DXGI_FORMAT_R32G32B32A32_FLOAT);
   srv(compose_heap.Get(),compose_variante*v+5,luz_coef.Get(),DXGI_FORMAT_R32G32B32A32_FLOAT);

   srv(compose_heap.Get(),compose_variante*v+1,luz_halo.Get(),DXGI_FORMAT_R32G32B32A32_FLOAT);
  }
  return true;
 }

 void iluminar(ID3D12GraphicsCommandList* c,ID3D12Resource* dst,DXGI_FORMAT formato,const ComposeConstants& k,bool prof=false,unsigned haloR=0,unsigned haloForca64=0,bool corte=false,bool teto=false){
  const float alfa0=nrreproject::value(nrreproject::controls,nrreproject::LightSmooth);
  const bool temporal=alfa0>0.f;
  const unsigned n=Policy::history_count,atual=constants.slice%n,anterior=(atual+n-1)%n,v=luz_cur;
  const bool valido=temporal&&luz_valido&&luz_quadro+1==policy.frame&&((policy.mask()>>anterior)&1u);

  LuzConstants l{k,alfa0,luz_delta,luz_eps,(temporal?1u:0u)|(prof?2u:0u)|(luz_raio<<8)};
  l.k.p1=(valido?2u:0u)|(pontual?4u:0u)|(atual<<8)|(anterior<<16);
  l.k.prof_rel=constants.depth_relative;l.k.prof_abs=constants.depth_absolute;
  uav(luz_heap.Get(),luz_variante*v+luz_uavs,dst,formato);
  {ID3D12DescriptorHeap* heaps[]{luz_heap.Get()};c->SetDescriptorHeaps(1,heaps);c->SetComputeRootSignature(luz_root.Get());
   c->SetComputeRoot32BitConstants(0,20,&l,0);c->SetComputeRootDescriptorTable(1,gpu(luz_heap.Get(),luz_variante*v));c->SetComputeRootDescriptorTable(2,gpu(luz_heap.Get(),luz_variante*v+luz_uavs));}
  const unsigned gx=(luz_qw+7)/8,gy=(luz_qh+7)/8;
  transition(c,luz_mom.Get(),read_state,write_state);transition(c,luz_base.Get(),read_state,write_state);
  c->SetPipelineState(luz_mom_pso.Get());c->Dispatch(gx,gy,1);
  transition(c,luz_mom.Get(),write_state,read_state);transition(c,luz_base.Get(),write_state,read_state);order_uav(c,dst);
  transition(c,luz_ab.Get(),read_state,write_state);
  c->SetPipelineState(luz_coef_pso.Get());c->Dispatch(gx,gy,1);
  transition(c,luz_ab.Get(),write_state,read_state);
  transition(c,luz_coef.Get(),read_state,write_state);transition(c,luz_hist[v].Get(),read_state,write_state);
  c->SetPipelineState(luz_temp_pso.Get());c->Dispatch(gx,gy,1);
  transition(c,luz_coef.Get(),write_state,read_state);transition(c,luz_hist[v].Get(),write_state,read_state);

  if(haloR>0u&&haloForca64>0u&&luz_prox_pso&&luz_halo_pso){
   l.bits|=((haloR&63u)<<16)|((haloForca64&255u)<<24)|(corte?4u:0u)|(teto?8u:0u);
   c->SetComputeRoot32BitConstants(0,20,&l,0);
   transition(c,luz_prox.Get(),read_state,write_state);
   c->SetPipelineState(luz_prox_pso.Get());c->Dispatch(gx,gy,1);
   transition(c,luz_prox.Get(),write_state,read_state);transition(c,luz_halo.Get(),read_state,write_state);
   c->SetPipelineState(luz_halo_pso.Get());c->Dispatch(gx,gy,1);
   transition(c,luz_halo.Get(),write_state,read_state);
  }
  ++luz_quadros;if(valido)++luz_filtrados;if(prof)++luz_prof_quadros;
  if(temporal){luz_valido=true;luz_quadro=policy.frame;luz_cur=1u-v;}else luz_valido=false;
 }

 void atualizar_ao_vivo(){
  (void)thresholds();
  if(!nrreproject::controls.menu.load(std::memory_order_acquire))return;
  using namespace nrreproject;
  pontual=value(controls,PointMotion)>.5f;
 }

 static float ajuste(nrreproject::Control c,float inicial){
  return nrreproject::controls.menu.load(std::memory_order_acquire)?nrreproject::value(nrreproject::controls,c):inicial;
 }
 ComposeConstants constantes_compose(unsigned tone,unsigned w,unsigned h,float exposure){

  if(!nrreproject::grade_semeado.load()){static const bool lido=[]{double v{};
   if(ler_ini(L"NeuralStyle",v)&&v>=0&&v<=2)nrreproject::grade_estilo.store(unsigned(v));
   double cfg{},lt{};const bool temCfg=ler_ini(L"NeuralToneConfig",cfg)&&cfg>=0&&cfg<=9;if(!ler_ini(L"LocalTone",lt))lt=0;
   nrreproject::grade_tom.store(nrreproject::tom_de_config(temCfg?unsigned(cfg):0u,float(lt)));return true;}();(void)lido;}
  ComposeConstants k{tone,w,h,exposure,nrreproject::grade_estilo.load(),nrreproject::grade_tom.load(),
   nrreproject::value(nrreproject::controls,nrreproject::GradeExposure),nrreproject::value(nrreproject::controls,nrreproject::GradeContrast),
   nrreproject::value(nrreproject::controls,nrreproject::GradeSaturation),
   nrreproject::value(nrreproject::controls,nrreproject::SoftGuard)>.5f?1u:0u,0,0,0.f,0.f,0,0};

  k.p0|=(nrreproject::index_of(nrreproject::controls,nrreproject::RatioFloor)&3u)<<3;
  if(k.estilo>2)k.estilo=0;if(!std::isfinite(k.tom))k.tom=1.f;
  nrreproject::somar_grade_do_estilo(k.estilo,k.tom,k.ev,k.contraste,k.saturacao);
  return k;
 }
 bool alocar_estabilizador(){
  if(!root(device.Get(),35,5,1,est_root)||!(q_chave(L"NeuralQEstabCR")?pipeline(device.Get(),est_root.Get(),g_nr_reproject_estabilizarcr,est_pso):pipeline(device.Get(),est_root.Get(),g_nr_reproject_estabilizar,est_pso)))return false;
  const auto wd=warped->GetDesc();D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  for(unsigned i=0;i<2;++i){if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&wd,write_state,nullptr,IID_PPV_ARGS(&est[i]))))return false;nrd3d::nomear(est[i].Get(),L"nr_estabilizador");est_estado[i]=write_state;}
  if(!criar_heap(24,est_heap))return false;

  for(unsigned j=0;j<4;++j){const unsigned i=j&1u;srv(est_heap.Get(),6*j,j<2u?warped.Get():fill_copia.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);srv(est_heap.Get(),6*j+1,est[1-i].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
   srv(est_heap.Get(),6*j+2,history[1].Get(),DXGI_FORMAT_R16G16_FLOAT,true);srv(est_heap.Get(),6*j+3,history[2].Get(),DXGI_FORMAT_R32_FLOAT,true);
   srv(est_heap.Get(),6*j+4,history[0].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,true);
   uav(est_heap.Get(),6*j+5,est[i].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);}
  est_cur=0;est_valido=false;return true;
 }

 void estabilizar(ID3D12GraphicsCommandList* c,float alfa,bool sem_limite_parado=false,bool segurar=false,bool movimento=false,bool seguinte=false){
  const unsigned cur=est_cur,prev=1u-cur;
  transition(c,est[cur].Get(),est_estado[cur],write_state);est_estado[cur]=write_state;
  transition(c,est[prev].Get(),est_estado[prev],read_state);est_estado[prev]=read_state;

  const unsigned historico_n=Policy::history_count,fatia_atual=constants.slice%historico_n,fatia_anterior=(fatia_atual+historico_n-1u)%historico_n;
  struct K{unsigned w,h,fatia;float alfa;unsigned valido;float ref;unsigned b,d;float profRel,profAbs;unsigned fatiaAnterior;float alfaMov,alfaLuz,limite,alfaLuzMov;
   float alfaRosto,aspecto;unsigned nRostos;float rostos[12];unsigned detRampa;float detParado;float expoTom,deltaClaroMax;unsigned rostoForma;}
   k{width,height,fatia_atual,alfa,est_valido?1u:0u,nrreproject::value(nrreproject::controls,nrreproject::StabilizeStill),
     segurar?2u:(sem_limite_parado&&movimento)?3u:sem_limite_parado?1u:0u,0u,
     constants.depth_relative,constants.depth_absolute,fatia_anterior,post_alfa_mov,
     std::clamp(nrreproject::value(nrreproject::controls,nrreproject::LightSmooth),0.f,1.f),
     std::max(nrreproject::value(nrreproject::controls,nrreproject::GainLimit),0.f),est_luz_mov};
  if(sem_limite_parado&&!segurar)std::memcpy(&k.d,&post_alargar,sizeof k.d);
  if(est_cross>0.f&&!segurar){k.alfa=k.alfaMov=k.alfaLuz=k.alfaLuzMov=est_cross;}

  k.alfaRosto=est_rosto;k.aspecto=comp_h?float(comp_w)/float(comp_h):1.f;k.nRostos=0u;
  k.detRampa=(post_det_rampa&&est_rosto>0.f&&!segurar)?1u:0u;k.detParado=post_det_parado;
  if(post_estab_buraco&&!segurar)k.detRampa|=2u;
  k.expoTom=constants.current_exposure;k.deltaClaroMax=segurar?0.f:post_delta_claro;
  {nrestab::Fonte fl=nrestab::Menu;k.alfaLuz=nrestab::luz_parada(k.alfaLuz,post_estab_luz,sem_limite_parado,segurar,&fl);

   const unsigned fontes=unsigned(est_fonte_base)|(unsigned(fl)<<4)|(unsigned(est_fonte_mov)<<8)|(est_cross>0.f&&!segurar?unsigned(nrestab::Crossfade)<<12:0u);
   if(fontes!=est_fontes_log&&est_fontes_trocas<64u){est_fontes_log=fontes;++est_fontes_trocas;
    nrlogfila::linha(L"nr_reproject.log","pid=%lu ESTAB_FONTES alfa=%s(%.3f) luz_parada=%s(%.3f) luz_andando=%s(%.3f)%s quadro=%llu\n",GetCurrentProcessId(),
     nrestab::nome(est_fonte_base),double(k.alfa),nrestab::nome(fl),double(k.alfaLuz),nrestab::nome(est_fonte_mov),double(k.alfaLuzMov),
     est_cross>0.f&&!segurar?" crossfade=todas":"",static_cast<unsigned long long>(policy.frame));}}
  k.rostoForma=0u;
  if(post_estab_rosto&&est_rosto>0.f&&!segurar&&comp_w&&comp_h){

   nrrj::AreaRosto a[4];const unsigned n=nrrj::areas_ligadas(nrrj::UsoArea::Estabilizador,a);
   for(unsigned i=0;i<n;++i){float* r=k.rostos+3u*k.nRostos++;r[0]=a[i].x/float(comp_w);r[1]=a[i].y/float(comp_h);r[2]=a[i].rx/float(comp_h);if(a[i].elipse)k.rostoForma=1u;}
  }
  ID3D12DescriptorHeap* heaps[]{est_heap.Get()};c->SetDescriptorHeaps(1,heaps);
  c->SetComputeRootSignature(est_root.Get());c->SetPipelineState(est_pso.Get());
  static_assert(sizeof(K)==35*4,"estabilizar: 35 constantes (dedup item 2: + rostoForma)");
  c->SetComputeRoot32BitConstants(0,35,&k,0);

  const unsigned v=6u*(cur+(cadeia_fonte==5u?2u:0u));
  c->SetComputeRootDescriptorTable(1,gpu(est_heap.Get(),v));c->SetComputeRootDescriptorTable(2,gpu(est_heap.Get(),v+5));
  c->Dispatch((width+7)/8,(height+7)/8,1);
  if(seguinte){
   transition(c,est[cur].Get(),write_state,read_state);est_estado[cur]=read_state;cadeia_fonte=1u+cur;
  }else{
  transition(c,est[cur].Get(),write_state,D3D12_RESOURCE_STATE_COPY_SOURCE);est_estado[cur]=D3D12_RESOURCE_STATE_COPY_SOURCE;
  transition(c,warped.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);c->CopyResource(warped.Get(),est[cur].Get());
  transition(c,warped.Get(),D3D12_RESOURCE_STATE_COPY_DEST,read_state);cadeia_fonte=0u;
  }
  est_valido=true;est_cur=prev;
 }

 unsigned cadeia_fonte{};
 static constexpr unsigned filtro_fontes=6,filtro_descr=7;
 static constexpr unsigned filtro_heap_n=2*filtro_fontes*2*filtro_descr;
 static constexpr unsigned filtro_variante(unsigned i,unsigned f,unsigned d){return ((i*filtro_fontes+f)*2u+d)*filtro_descr;}
 ID3D12Resource* fonte_recurso(unsigned f)const{return f==0u?warped.Get():f<=2u?est[f-1u].Get():f==3u?cro_out.Get():f==4u?lg_out.Get():f==5u?fill_copia.Get():nullptr;}
 bool roda_cro(float e)const{return cro_pso&&cro_ligado()&&std::isfinite(e)&&e>0.f;}
 bool roda_lg(float e)const{return lg_pso&&lg_ligado()&&std::isfinite(e)&&e>0.f;}
 bool roda_su(float e)const{return su_pso&&su_cfg.ligado()&&std::isfinite(e)&&e>0.f;}
 bool cadeia_segue(float e)const{return cadeia_direta.load(std::memory_order_relaxed)&&(roda_cro(e)||roda_lg(e)||roda_su(e));}

 bool ultimo_cro(float e)const{return !cadeia_direta.load(std::memory_order_relaxed)||(!roda_lg(e)&&!roda_su(e));}
 bool ultimo_lg(float e)const{return !cadeia_direta.load(std::memory_order_relaxed)||!roda_su(e);}

 void filtro_descritores(ID3D12DescriptorHeap* h,const ComPtr<ID3D12Resource>* x,ID3D12Resource* saida,bool base=false){
  for(unsigned i=0;i<2;++i)for(unsigned f=0;f<filtro_fontes;++f)for(unsigned d=0;d<2;++d){const unsigned o=filtro_variante(i,f,d);
   srv(h,o,fonte_recurso(f),DXGI_FORMAT_R16G16B16A16_FLOAT);srv(h,o+1,x[1-i].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
   srv(h,o+2,history[1].Get(),DXGI_FORMAT_R16G16_FLOAT,true);srv(h,o+3,history[2].Get(),DXGI_FORMAT_R32_FLOAT,true);
   if(base)srv(h,o+4,base_tom.Get(),DXGI_FORMAT_R32G32B32A32_FLOAT);else srv(h,o+4,history[0].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,true);
   uav(h,o+5,x[i].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);uav(h,o+6,d?warped.Get():saida,DXGI_FORMAT_R16G16B16A16_FLOAT);}
 }

 unsigned filtro_inicio(ID3D12GraphicsCommandList* c,unsigned cur,bool ultimo,ID3D12Resource* out,D3D12_RESOURCE_STATES& out_estado,bool& direto){
  direto=ultimo&&cadeia_fonte!=0u;
  if(direto)transition(c,warped.Get(),read_state,write_state);
  else{transition(c,out,out_estado,write_state);out_estado=write_state;}
  return filtro_variante(cur,cadeia_fonte,direto?1u:0u);
 }

 void filtro_fim(ID3D12GraphicsCommandList* c,bool ultimo,bool direto,ID3D12Resource* out,D3D12_RESOURCE_STATES& out_estado,unsigned fonte_out){
  if(direto){transition(c,warped.Get(),write_state,read_state);cadeia_fonte=0u;return;}
  if(!ultimo){transition(c,out,write_state,read_state);out_estado=read_state;cadeia_fonte=fonte_out;return;}
  transition(c,out,write_state,D3D12_RESOURCE_STATE_COPY_SOURCE);out_estado=D3D12_RESOURCE_STATE_COPY_SOURCE;
  transition(c,warped.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);c->CopyResource(warped.Get(),out);
  transition(c,warped.Get(),D3D12_RESOURCE_STATE_COPY_DEST,read_state);cadeia_fonte=0u;
 }

 void cadeia_fechar(ID3D12GraphicsCommandList* c){
  if(!cadeia_fonte)return;
  D3D12_RESOURCE_STATES* st=cadeia_fonte<=2u?&est_estado[cadeia_fonte-1u]:cadeia_fonte==3u?&cro_out_estado:cadeia_fonte==4u?&lg_out_estado:&fill_copia_estado;
  ID3D12Resource* f=fonte_recurso(cadeia_fonte);cadeia_fonte=0u;if(!f)return;
  transition(c,f,*st,D3D12_RESOURCE_STATE_COPY_SOURCE);*st=D3D12_RESOURCE_STATE_COPY_SOURCE;
  transition(c,warped.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);c->CopyResource(warped.Get(),f);
  transition(c,warped.Get(),D3D12_RESOURCE_STATE_COPY_DEST,read_state);
 }

 bool alocar_filtro(ComPtr<ID3D12Resource> (&x)[2],D3D12_RESOURCE_STATES (&x_estado)[2],ComPtr<ID3D12Resource>& out,D3D12_RESOURCE_STATES& out_estado,
                    ComPtr<ID3D12DescriptorHeap>& heap,bool base){
  const auto wd=warped->GetDesc();D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  for(unsigned i=0;i<2;++i){if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&wd,write_state,nullptr,IID_PPV_ARGS(&x[i]))))return false;nrd3d::nomear(x[i].Get(),L"nr_filtro_cena");x_estado[i]=write_state;}
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&wd,write_state,nullptr,IID_PPV_ARGS(&out))))return false;nrd3d::nomear(out.Get(),L"nr_filtro_cena_saida");out_estado=write_state;
  if(!criar_heap(filtro_heap_n,heap))return false;

  filtro_descritores(heap.Get(),x,out.Get(),base);
  return true;
 }

 void passe_filtro(ID3D12GraphicsCommandList* c,unsigned cur,ComPtr<ID3D12Resource> (&x)[2],D3D12_RESOURCE_STATES (&x_estado)[2],
                   ID3D12Resource* out,D3D12_RESOURCE_STATES& out_estado,ID3D12DescriptorHeap* heap,ID3D12RootSignature* raiz,
                   ID3D12PipelineState* pso,const void* k,bool ultimo,unsigned fonte_out){
  const unsigned prev=1u-cur;
  transition(c,x[cur].Get(),x_estado[cur],write_state);x_estado[cur]=write_state;
  transition(c,x[prev].Get(),x_estado[prev],read_state);x_estado[prev]=read_state;
  bool direto{};const unsigned o=filtro_inicio(c,cur,ultimo,out,out_estado,direto);
  ID3D12DescriptorHeap* heaps[]{heap};c->SetDescriptorHeaps(1,heaps);
  c->SetComputeRootSignature(raiz);c->SetPipelineState(pso);
  c->SetComputeRoot32BitConstants(0,20,k,0);
  c->SetComputeRootDescriptorTable(1,gpu(heap,o));c->SetComputeRootDescriptorTable(2,gpu(heap,o+5));
  c->Dispatch((width+7)/8,(height+7)/8,1);
  transition(c,x[cur].Get(),write_state,read_state);x_estado[cur]=read_state;
  filtro_fim(c,ultimo,direto,out,out_estado,fonte_out);
 }

 bool alocar_croma(bool base=false){

  {const bool lds=cfg_croma_lds;const bool usa=lds&&cro_raio*cro_passo<=4u;
   const bool ok=root(device.Get(),20,5,2,cro_root)&&(base?(usa?pipeline(device.Get(),cro_root.Get(),g_nr_reproject_cromaldsbase,cro_pso):pipeline(device.Get(),cro_root.Get(),g_nr_reproject_cromabase,cro_pso)):
                                                          (usa?pipeline(device.Get(),cro_root.Get(),g_nr_reproject_cromalds,cro_pso):pipeline(device.Get(),cro_root.Get(),g_nr_reproject_croma,cro_pso)));
   if(!ok)return false;
   if(usa)nrlogfila::linha(L"nr_reproject.log","pid=%lu CUSTO_CROMA_LDS ligado (raio=%u passo=%u)\n",GetCurrentProcessId(),cro_raio,cro_passo);}
  if(!alocar_filtro(cro,cro_estado,cro_out,cro_out_estado,cro_heap,base))return false;
  cro_cur=0;cro_valido=false;
  {{nrlogfila::linha(L"nr_reproject.log","pid=%lu CROMA_ESTAVEL ligado alfa=%.3f mov=%.3f ref=%.2f delta=%.3f raio=%u passo=%u sigma=%.3f limite=%.3f %ux%u\n",GetCurrentProcessId(),
   double(cro_alfa),double(cro_mov),double(cro_ref),double(cro_delta),cro_raio,cro_passo,double(cro_sigma),double(cro_limite),width,height);}}
  return true;
 }

 void croma(ID3D12GraphicsCommandList* c,float expo,unsigned tom,bool ultimo=true){
  if(!roda_cro(expo)){cro_valido=false;return;}
  const unsigned cur=cro_cur,prev=1u-cur;
  const bool valido=cro_valido&&cro_quadro+1u==policy.frame;
  const unsigned historico_n=Policy::history_count,fatia_atual=constants.slice%historico_n,fatia_anterior=(fatia_atual+historico_n-1u)%historico_n;
  struct K{unsigned w,h,fatia,fatiaAnterior,valido,raio,tom,passo;float alfa,alfaMov,delta,expo,profRel,profAbs,refMov,limite,sigma,r0,r1,r2;}
   k{width,height,fatia_atual,fatia_anterior,valido?1u:0u,cro_raio,tom&1u,cro_passo,cro_alfa,cro_mov,cro_delta,expo,
     constants.depth_relative,constants.depth_absolute,cro_ref,cro_limite,cro_sigma,0.f,0.f,0.f};
  static_assert(sizeof(K)==80);
  passe_filtro(c,cur,cro,cro_estado,cro_out.Get(),cro_out_estado,cro_heap.Get(),cro_root.Get(),cro_pso.Get(),&k,ultimo,3u);
  cro_valido=true;cro_quadro=policy.frame;cro_cur=prev;++cro_quadros;
 }
 bool alocar_luz_grossa(bool base=false){
  if(!root(device.Get(),20,5,2,lg_root)||!(base?pipeline(device.Get(),lg_root.Get(),g_nr_reproject_luzgrossabase,lg_pso):pipeline(device.Get(),lg_root.Get(),g_nr_reproject_luzgrossa,lg_pso)))return false;
  if(!alocar_filtro(lg,lg_estado,lg_out,lg_out_estado,lg_heap,base))return false;
  lg_cur=0;lg_valido=false;
  {{nrlogfila::linha(L"nr_reproject.log","pid=%lu LUZ_GROSSA ligada alfa=%.3f mov=%.3f ref=%.2f delta=%.3f raio=%u passo=%u limite=%.3f segura_max=%u segura_fade=%u %ux%u\n",GetCurrentProcessId(),
   double(lg_alfa),double(lg_mov),double(lg_ref),double(lg_delta),lg_raio,lg_passo,double(lg_limite),segura_max,segura_fade,width,height);}}
  return true;
 }

 bool alocar_base_tom(){
  if(!root(device.Get(),20,5,2,base_tom_root)||!pipeline(device.Get(),base_tom_root.Get(),g_nr_reproject_basetom,base_tom_pso))return false;
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=width;td.Height=height;td.DepthOrArraySize=1;td.MipLevels=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  if(!format_support(device.Get(),td.Format,true)||FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read_state,nullptr,IID_PPV_ARGS(&base_tom))))return false;nrd3d::nomear(base_tom.Get(),L"nr_base_tom");
  base_tom_estado=read_state;
  if(!criar_heap(7,base_tom_heap))return false;
  ID3D12DescriptorHeap* h=base_tom_heap.Get();
  srv(h,0,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);srv(h,1,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);
  srv(h,2,nullptr,DXGI_FORMAT_R16G16_FLOAT,true);srv(h,3,nullptr,DXGI_FORMAT_R32_FLOAT,true);
  srv(h,4,history[0].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,true);
  uav(h,5,base_tom.Get(),DXGI_FORMAT_R32G32B32A32_FLOAT);uav(h,6,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);
  nrlogfila::linha(L"nr_reproject.log","pid=%lu CUSTO_BASE_TOM ligado (NeuralCustoBaseTom=1) %ux%u\n",GetCurrentProcessId(),width,height);
  return true;
 }

 void calcular_base_tom(ID3D12GraphicsCommandList* c,float expo,unsigned tom){
  if(!base_tom_pso||!(roda_cro(expo)||roda_lg(expo)))return;
  const unsigned historico_n=Policy::history_count,fatia_atual=constants.slice%historico_n,fatia_anterior=(fatia_atual+historico_n-1u)%historico_n;
  struct K{unsigned w,h,fatia,fatiaAnterior,valido,raio,tom,passo;float alfa,alfaMov,delta,expo,r[8];}
   k{width,height,fatia_atual,fatia_anterior,0u,0u,tom&1u,0u,0.f,0.f,0.f,expo,{}};
  static_assert(sizeof(K)==80);
  transition(c,base_tom.Get(),base_tom_estado,write_state);base_tom_estado=write_state;
  bind(c,base_tom_heap.Get(),base_tom_root.Get(),base_tom_pso.Get(),20,&k,5);
  c->Dispatch((width+7)/8,(height+7)/8,1);
  transition(c,base_tom.Get(),write_state,read_state);base_tom_estado=read_state;++base_tom_quadros;
 }
 void luz_grossa(ID3D12GraphicsCommandList* c,float expo,unsigned tom,bool ultimo=true){
  if(!roda_lg(expo)){lg_valido=false;return;}
  const unsigned cur=lg_cur,prev=1u-cur;
  const bool valido=lg_valido&&lg_quadro+1u==policy.frame;
  const unsigned historico_n=Policy::history_count,fatia_atual=constants.slice%historico_n,fatia_anterior=(fatia_atual+historico_n-1u)%historico_n;
  struct K{unsigned w,h,fatia,fatiaAnterior,valido,raio,tom,passo;float alfa,alfaMov,delta,expo,profRel,profAbs,refMov,limite,r0,r1,r2,r3;}
   k{width,height,fatia_atual,fatia_anterior,valido?1u:0u,lg_raio,tom&1u,lg_passo,lg_alfa,lg_mov,lg_delta,expo,
     constants.depth_relative,constants.depth_absolute,lg_ref,lg_limite,lg_teto,lg_piso,lg_cor,0.f};
  static_assert(sizeof(K)==80);
  passe_filtro(c,cur,lg,lg_estado,lg_out.Get(),lg_out_estado,lg_heap.Get(),lg_root.Get(),lg_pso.Get(),&k,ultimo,4u);
  lg_valido=true;lg_quadro=policy.frame;lg_cur=prev;++lg_quadros;
 }
 bool alocar_suave(){
  if(!root(device.Get(),20,5,2,su_root)||!pipeline(device.Get(),su_root.Get(),g_nr_reproject_suave,su_pso))return false;
  if(!alocar_filtro(su,su_estado,su_out,su_out_estado,su_heap,false))return false;
  su_cur=0;su_valido=false;
  {{nrlogfila::linha(L"nr_reproject.log","pid=%lu SUAVE ligado forca=%.3f entrada=%.2f residuo=%.2f mov=%.2f escuro=%d banda=%u %ux%u\n",GetCurrentProcessId(),
   double(su_cfg.forca),double(su_cfg.entrada),double(su_cfg.residuo),double(su_cfg.mov),su_cfg.escuro?1:0,su_cfg.banda,width,height);}}
  return true;
 }

 void suave(ID3D12GraphicsCommandList* c,float expo,unsigned tom){
  if(!roda_su(expo)){su_valido=false;return;}
  const unsigned cur=su_cur,prev=1u-cur;
  const bool valido=su_valido&&su_quadro+1u==policy.frame;
  const unsigned historico_n=Policy::history_count,fatia_atual=constants.slice%historico_n,fatia_anterior=(fatia_atual+historico_n-1u)%historico_n;
  const nrsuave::Constantes k=nrsuave::constantes(su_cfg,width,height,fatia_atual,fatia_anterior,valido,tom,expo,constants.depth_relative,constants.depth_absolute);
  passe_filtro(c,cur,su,su_estado,su_out.Get(),su_out_estado,su_heap.Get(),su_root.Get(),su_pso.Get(),&k,true,0u);
  su_valido=true;su_quadro=policy.frame;su_cur=prev;++su_quadros;
 }

 bool alocar_preenchimento(){
  if(!root(device.Get(),8,2,1,fill_root)||!pipeline(device.Get(),fill_root.Get(),g_nr_reproject_fill,fill_pso))return false;
  const auto wd=warped->GetDesc();D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&wd,read_state,nullptr,IID_PPV_ARGS(&fill_copia))))return false;nrd3d::nomear(fill_copia.Get(),L"nr_fill_copia");
  fill_copia_estado=read_state;

  if(!criar_heap(6,fill_heap))return false;
  srv(fill_heap.Get(),0,history[2].Get(),DXGI_FORMAT_R32_FLOAT,true);srv(fill_heap.Get(),1,fill_copia.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
  uav(fill_heap.Get(),2,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
  srv(fill_heap.Get(),3,history[2].Get(),DXGI_FORMAT_R32_FLOAT,true);srv(fill_heap.Get(),4,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
  uav(fill_heap.Get(),5,fill_copia.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);

  if(!pipeline(device.Get(),fill_root.Get(),g_nr_reproject_fillrapido,fill_rapido_pso))fill_rapido_pso.Reset();
  return true;
 }

 void preencher(ID3D12GraphicsCommandList* c){

  struct K{unsigned w,h,fatia;float limiar,relativa,absoluta;unsigned raio,pad;}
   k{width,height,constants.slice,preenchimento,constants.depth_relative,constants.depth_absolute,256u,modo_preenchimento()};

  if(fill_direto.load(std::memory_order_relaxed)&&fill_rapido_pso){
   transition(c,fill_copia.Get(),fill_copia_estado,write_state);fill_copia_estado=write_state;
   ID3D12DescriptorHeap* heaps[]{fill_heap.Get()};c->SetDescriptorHeaps(1,heaps);
   c->SetComputeRootSignature(fill_root.Get());c->SetPipelineState(fill_rapido_pso.Get());
   c->SetComputeRoot32BitConstants(0,8,&k,0);
   c->SetComputeRootDescriptorTable(1,gpu(fill_heap.Get(),3));c->SetComputeRootDescriptorTable(2,gpu(fill_heap.Get(),5));
   c->Dispatch((width+7)/8,(height+7)/8,1);
   transition(c,fill_copia.Get(),write_state,read_state);fill_copia_estado=read_state;cadeia_fonte=5u;
   return;
  }
  {
  transition(c,fill_copia.Get(),fill_copia_estado,read_state);fill_copia_estado=read_state;
  transition(c,fill_copia.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);
  transition(c,warped.Get(),read_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
  c->CopyResource(fill_copia.Get(),warped.Get());
  transition(c,fill_copia.Get(),D3D12_RESOURCE_STATE_COPY_DEST,read_state);
  transition(c,warped.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,write_state);}
  ID3D12DescriptorHeap* heaps[]{fill_heap.Get()};c->SetDescriptorHeaps(1,heaps);
  c->SetComputeRootSignature(fill_root.Get());c->SetPipelineState(fill_pso.Get());
  c->SetComputeRoot32BitConstants(0,8,&k,0);
  c->SetComputeRootDescriptorTable(1,gpu(fill_heap.Get(),0));c->SetComputeRootDescriptorTable(2,gpu(fill_heap.Get(),2));
  c->Dispatch((width+7)/8,(height+7)/8,1);
  transition(c,warped.Get(),write_state,read_state);
 }
 void bind(ID3D12GraphicsCommandList* c,ID3D12DescriptorHeap* h,ID3D12RootSignature* r,ID3D12PipelineState* p,unsigned n,const void* values,unsigned uav_offset){ID3D12DescriptorHeap* heaps[]{h};c->SetDescriptorHeaps(1,heaps);c->SetComputeRootSignature(r);c->SetPipelineState(p);c->SetComputeRoot32BitConstants(0,n,values,0);c->SetComputeRootDescriptorTable(1,gpu(h,0));c->SetComputeRootDescriptorTable(2,gpu(h,uav_offset));}

 unsigned long long captures{};
 std::uint64_t realocacoes{};

 void log_realocacao(unsigned para_w,unsigned para_h){
(void)para_w;(void)para_h;
}
 void log_constants(const Constants& k){
(void)k;
}

 DespejoPendente reservar_despejo(ID3D12GraphicsCommandList* c){
 (void)c;return {};
}
 void gravar_despejo(ID3D12GraphicsCommandList* c,ID3D12Resource* fonte,const DespejoPendente& reserva){
  if(!reserva.leitura)return;
  const auto wd=warped->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 total{};
  device->GetCopyableFootprints(&wd,0,1,0,&fp,nullptr,nullptr,&total);
  const auto& leitura=reserva.leitura;const unsigned indice=reserva.indice;
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;
  D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=total;bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  transition(c,warped.Get(),read_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
  D3D12_TEXTURE_COPY_LOCATION destino{},origem{};
  destino.pResource=leitura.Get();destino.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;destino.PlacedFootprint=fp;
  origem.pResource=warped.Get();origem.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;origem.SubresourceIndex=0;
  c->CopyTextureRegion(&destino,0,0,0,&origem,nullptr);
  transition(c,warped.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,read_state);

  despejos.push_back(reserva);

  despejos.push_back(DespejoPendente{leitura,begins+3,width,height,indice,
                                     fp.Footprint.RowPitch,std::size_t(total),despejo.ganho,
                                     false,false,true});
  if(fonte) {
   const auto sd=fonte->GetDesc();
   D3D12_PLACED_SUBRESOURCE_FOOTPRINT sf{};UINT64 stotal{};
   device->GetCopyableFootprints(&sd,0,1,0,&sf,nullptr,nullptr,&stotal);
   ComPtr<ID3D12Resource> leitura_fonte;
   D3D12_RESOURCE_DESC sbd=bd;sbd.Width=stotal;
   if(stotal&&SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&sbd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&leitura_fonte)))) {nrd3d::nomear(leitura_fonte.Get(),L"nr_f9_leitura_fonte");
    D3D12_TEXTURE_COPY_LOCATION sdst{},ssrc{};
    sdst.pResource=leitura_fonte.Get();sdst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;sdst.PlacedFootprint=sf;
    ssrc.pResource=fonte;ssrc.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;ssrc.SubresourceIndex=0;
    transition(c,fonte,read_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
    c->CopyTextureRegion(&sdst,0,0,0,&ssrc,nullptr);
    transition(c,fonte,D3D12_RESOURCE_STATE_COPY_SOURCE,read_state);
    despejos.push_back(DespejoPendente{leitura_fonte,begins+3,width,height,indice,
                                       sf.Footprint.RowPitch,std::size_t(stotal),despejo.ganho,false,true});
   }
  }

  if(history[1]) {
   const auto md=history[1]->GetDesc();
   const unsigned fatia=constants.slice<md.DepthOrArraySize?constants.slice:0u;
   D3D12_PLACED_SUBRESOURCE_FOOTPRINT mf{};UINT64 mtotal{};
   device->GetCopyableFootprints(&md,fatia,1,0,&mf,nullptr,nullptr,&mtotal);
   ComPtr<ID3D12Resource> leitura_mv;
   D3D12_RESOURCE_DESC mbd=bd;mbd.Width=mtotal;
   if(mtotal&&SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&mbd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&leitura_mv)))) {nrd3d::nomear(leitura_mv.Get(),L"nr_f9_leitura_mv");
    transition(c,history[1].Get(),read_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION md_destino{},md_origem{};
    md_destino.pResource=leitura_mv.Get();md_destino.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;md_destino.PlacedFootprint=mf;
    md_origem.pResource=history[1].Get();md_origem.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;md_origem.SubresourceIndex=fatia;
    c->CopyTextureRegion(&md_destino,0,0,0,&md_origem,nullptr);
    transition(c,history[1].Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,read_state);
    despejos.push_back(DespejoPendente{leitura_mv,begins+3,width,height,indice,
                                       mf.Footprint.RowPitch,std::size_t(mtotal),despejo.ganho,true});
   }
  }

  if(despejo.profundidade&&history[2]) {
   const auto dd=history[2]->GetDesc();
   const unsigned fatia=constants.slice;
   D3D12_PLACED_SUBRESOURCE_FOOTPRINT df{};UINT64 dtotal{};
   device->GetCopyableFootprints(&dd,fatia,1,0,&df,nullptr,nullptr,&dtotal);
   ComPtr<ID3D12Resource> leitura_depth;
   D3D12_RESOURCE_DESC dbd=bd;dbd.Width=dtotal;
   if(dtotal&&SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&dbd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&leitura_depth)))) {nrd3d::nomear(leitura_depth.Get(),L"nr_f9_leitura_prof");
    transition(c,history[2].Get(),read_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION dst{},src{};
    dst.pResource=leitura_depth.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=df;
    src.pResource=history[2].Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.SubresourceIndex=fatia;
    c->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
    transition(c,history[2].Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,read_state);
    DespejoPendente d{leitura_depth,begins+3,width,height,indice,df.Footprint.RowPitch,std::size_t(dtotal),despejo.ganho};
    d.profundidade=true;d.fatia=fatia;d.idade=constants.age;d.quadro=policy.frame;
    d.depth_relative=constants.depth_relative;d.depth_absolute=constants.depth_absolute;
    despejos.push_back(std::move(d));
   }
  }
 }

 void escrever_despejos_vencidos(){

}

 bool escrever_bmp(const DespejoPendente& d){
(void)d;return false;
}
 void escrever_par(const DespejoPendente& d){
(void)d;
}

 void log_post(int job,const char* reason,const D3D12_RESOURCE_DESC* od){{
  if(od)nrlogfila::linha(L"nr_reproject.log","pid=%lu frame=%llu job=%d post_composed=%llu post_refused=%llu post_lost=%llu formato=%u saida=%llux%u entrada=%ux%u reason=%s\n",GetCurrentProcessId(),policy.frame,job,post_composed,post_refused,post_lost,unsigned(od->Format),static_cast<unsigned long long>(od->Width),od->Height,width,height,reason);
  else nrlogfila::linha(L"nr_reproject.log","pid=%lu frame=%llu job=%d post_composed=%llu post_refused=%llu post_lost=%llu reason=%s\n",GetCurrentProcessId(),policy.frame,job,post_composed,post_refused,post_lost,reason);
  }}
 void log(int job,int age,const char* reason){trace(job,age,reason);const auto events=composed+skipped+unsupported;if(events>3&&(events&255)!=0)return;{nrlogfila::linha(L"nr_reproject.log","pid=%lu frame=%llu job=%d age=%d composed=%llu skipped=%llu unsupported=%llu reason=%s\n",GetCurrentProcessId(),policy.frame,job,age,composed,skipped,unsupported,reason);}}
};

namespace {
constexpr std::uint64_t impl_velho_margem=3;
struct ImplsVelhos{std::mutex m;std::vector<std::pair<std::shared_ptr<void>,std::uint64_t>> v;std::uint64_t capturas{};};
ImplsVelhos& impls_velhos(){static auto* a=new ImplsVelhos;return *a;}
void impls_velhos_passo() noexcept {
 std::vector<std::shared_ptr<void>> soltar;
 try{auto& V=impls_velhos();std::lock_guard<std::mutex> l(V.m);++V.capturas;
  for(auto it=V.v.begin();it!=V.v.end();){if(it->second<=V.capturas){soltar.push_back(std::move(it->first));it=V.v.erase(it);}else ++it;}}catch(...){}
}
unsigned impls_velhos_vivos() noexcept {try{auto& V=impls_velhos();std::lock_guard<std::mutex> l(V.m);return unsigned(V.v.size());}catch(...){return 0u;}}
}

Backend::Backend():p_([]{nrinijogo::Lote lote;return std::make_unique<Impl>();}()){}Backend::~Backend()=default;

void Backend::invalidate() noexcept {if(p_){p_->policy.invalidate();p_->est_valido=false;p_->cro_valido=false;p_->lg_valido=false;p_->su_valido=false;p_->ganho_valido=false;p_->luz_valido=false;p_->perder_post("post_lost_invalidate");
 for(auto& a:p_->late.armed)a=0;}}

void Backend::invalidate_suave() noexcept {if(p_){p_->policy.invalidate();p_->perder_post("post_lost_invalidate_suave");for(auto& a:p_->late.armed)a=0;}}

void Backend::reset() noexcept {nrinijogo::Lote lote;
 if(p_){try{auto& V=impls_velhos();std::lock_guard<std::mutex> l(V.m);
   V.v.emplace_back(std::shared_ptr<void>(p_.get(),[](void* x){delete static_cast<Impl*>(x);}),V.capturas+impl_velho_margem);(void)p_.release();}
  catch(...){}}
 p_.reset(new(std::nothrow) Impl);nrrj::epoca_backend.fetch_add(1,std::memory_order_acq_rel);}
void Backend::accepted(int job) noexcept {if(p_){p_->policy.accepted(job);rj_aceito(job);
 if(nrrj::npc2.load(std::memory_order_relaxed))for(int kn=1;kn<=nrrj::npc_n();++kn){nrrj::Inst i1(kn);rj_aceito(job);}
}}
bool Backend::post_pending() const noexcept {return p_&&p_->post.ativo;}

void Backend::refuse_post(const char* reason) noexcept {
 if(!p_)return;auto& p=*p_;const int job=p.post.job;p.post.ativo=false;p.post_recusado=true;++p.post_refused;
 p.log_post(job,reason?reason:"post_unsupported_list_state_fallback_pre",nullptr);
}
Backend::Counters Backend::counters() const noexcept {
 if(!p_)return {};const auto& p=*p_;
 return Counters{p.composed,p.skipped,p.unsupported,p.realocacoes,p.recusas_por_limite,
                 unsigned(p.retired.size()),p.post_composed,p.post_refused,p.post_lost,
                 p.alocacoes_tentadas,p.alocacoes_adiadas,p.est_pso!=nullptr,p.base_tom_quadros,impls_velhos_vivos()};
}
CachePsoInfo cache_pso_info() noexcept{auto& c=cache_pso();std::lock_guard<std::mutex> trava(c.m);return {c.criados,c.reusados};}
bool Backend::begin(const NrCaptureRecord& r){
 impls_velhos_passo();
 if(!p_)return false;auto& p=*p_;
 p.atualizar_ao_vivo();

 ++p.begins;
 p.retired.erase(std::remove_if(p.retired.begin(),p.retired.end(),
                                [&](const Impl::Aposentadas& v){return v.vencimento<=p.begins;}),
                 p.retired.end());

 p.escrever_despejos_vencidos();
 p.policy.captured=false;p.captured_command=nullptr;p.composed_frame=false;p.perder_post("post_lost_sem_compose_post");
 auto* c=static_cast<ID3D12GraphicsCommandList*>(r.command_list);auto* color=static_cast<ID3D12Resource*>(r.color);auto* motion=static_cast<ID3D12Resource*>(r.motion);auto* depth=static_cast<ID3D12Resource*>(r.depth);auto* exposure=static_cast<ID3D12Resource*>(r.exposure);

 auto reject=[&](const char* porque){p.policy.invalidate();++p.unsupported;p.log(0,-1,porque);return false;};

 const DXGI_FORMAT cor_vista=color?vista_cor(color->GetDesc().Format):DXGI_FORMAT_UNKNOWN;
 const DXGI_FORMAT prof_vista=depth?vista_profundidade(depth->GetDesc().Format):DXGI_FORMAT_UNKNOWN;
 const DXGI_FORMAT mov_vista=motion?vista_movimento(motion->GetDesc().Format):DXGI_FORMAT_UNKNOWN;
 if(!c||!entrada(color,cor_vista)||!entrada(motion,mov_vista)||!entrada(depth,prof_vista)||!std::isfinite(r.motion_scale_x)||!std::isfinite(r.motion_scale_y)){
  char linha[128];
  NRLOGF(linha,sizeof linha,"capture_formato_de_entrada cor=%u movimento=%u profundidade=%u",
                color?unsigned(color->GetDesc().Format):0u,motion?unsigned(motion->GetDesc().Format):0u,depth?unsigned(depth->GetDesc().Format):0u);
  if(p.nova(nrcompat::Origem::Captura,p.captura_relatada,"capture_formato_de_entrada")){
   D3D12_RESOURCE_DESC dc{},dm{},dp{};if(color)dc=color->GetDesc();if(motion)dm=motion->GetDesc();if(depth)dp=depth->GetDesc();
   char campo[48]{},detalhe[420];
   if(!c)NRLOGF(campo,sizeof campo,"lista nula");
   else if(!nrformato::motivo_papel("cor",color?&dc:nullptr,cor_vista,campo,sizeof campo)&&
           !nrformato::motivo_papel("mv",motion?&dm:nullptr,mov_vista,campo,sizeof campo)&&
           !nrformato::motivo_papel("prof",depth?&dp:nullptr,prof_vista,campo,sizeof campo))
    NRLOGF(campo,sizeof campo,"mvScale %.4g,%.4g",static_cast<double>(r.motion_scale_x),static_cast<double>(r.motion_scale_y));
   NRLOGF(detalhe,sizeof detalhe,"campo=\"%s\" cor=fmt%u_%llux%u_flags0x%x mv=fmt%u_%llux%u_flags0x%x prof=fmt%u_%llux%u_flags0x%x mvScale=%.6g,%.6g limite=%u",campo,
                 unsigned(dc.Format),static_cast<unsigned long long>(dc.Width),dc.Height,unsigned(dc.Flags),unsigned(dm.Format),static_cast<unsigned long long>(dm.Width),dm.Height,unsigned(dm.Flags),
                 unsigned(dp.Format),static_cast<unsigned long long>(dp.Width),dp.Height,unsigned(dp.Flags),static_cast<double>(r.motion_scale_x),static_cast<double>(r.motion_scale_y),nrformato::lado_maximo);
   p.relatar(nrcompat::Origem::Captura,p.captura_relatada,"capture_formato_de_entrada",detalhe,campo);}
  return reject(linha);
 }
 if(exposure&&(!texture(exposure,DXGI_FORMAT_R32_FLOAT)||exposure->GetDesc().Width!=1||exposure->GetDesc().Height!=1)){
  if(!p.nova(nrcompat::Origem::Captura,p.captura_relatada,"capture_exposicao"))return reject("capture_exposicao");
  const auto de=exposure->GetDesc();char detalhe[120],campo[48];
  NRLOGF(detalhe,sizeof detalhe,"exp=fmt%u_%llux%u_flags0x%x (aceita: R32_FLOAT 1x1)",unsigned(de.Format),static_cast<unsigned long long>(de.Width),de.Height,unsigned(de.Flags));
  NRLOGF(campo,sizeof campo,"exp fmt %u %llux%u",unsigned(de.Format),static_cast<unsigned long long>(de.Width),de.Height);
  p.relatar(nrcompat::Origem::Captura,p.captura_relatada,"capture_exposicao",detalhe,campo);
  return reject("capture_exposicao");
 }

 const bool lista_compute=c&&c->GetType()==D3D12_COMMAND_LIST_TYPE_COMPUTE;
 const unsigned states[]{r.color_state,r.motion_state,r.depth_state,r.exposure_state};for(unsigned i=0;i<(exposure?4u:3u);++i)if(state(states[i])==D3D12_RESOURCE_STATES(~0u)||
   (lista_compute&&!nrffx_estado::valido_em_compute(unsigned(state(states[i]))))){
  if(!p.nova(nrcompat::Origem::Captura,p.captura_relatada,"capture_estado_de_recurso"))return reject("capture_estado_de_recurso");
  const char* papel[]{"cor","mv","prof","exp"};char detalhe[120],campo[48];
  NRLOGF(detalhe,sizeof detalhe,"papel=%s estado_ffx=0x%x",papel[i],states[i]);NRLOGF(campo,sizeof campo,"%s estado 0x%x",papel[i],states[i]);
  p.relatar(nrcompat::Origem::Captura,p.captura_relatada,"capture_estado_de_recurso",detalhe,campo);
  return reject("capture_estado_de_recurso");
 }
 if(!p.device&&FAILED(color->GetDevice(IID_PPV_ARGS(&p.device)))){if(p.nova(nrcompat::Origem::Captura,p.captura_relatada,"capture_sem_device"))p.relatar(nrcompat::Origem::Captura,p.captura_relatada,"capture_sem_device","GetDevice da cor falhou",nullptr);return reject("capture_sem_device");}
 if(!device_matches(p.device.Get(),c)||!device_matches(p.device.Get(),color)||!device_matches(p.device.Get(),motion)||!device_matches(p.device.Get(),depth)||(exposure&&!device_matches(p.device.Get(),exposure))){
  if(p.nova(nrcompat::Origem::Captura,p.captura_relatada,"capture_device_diferente"))p.relatar(nrcompat::Origem::Captura,p.captura_relatada,"capture_device_diferente","lista, cor, movimento, profundidade ou exposicao em outro dispositivo",nullptr);
  return reject("capture_device_diferente");
 }
 auto cd=color->GetDesc(),md=motion->GetDesc();

 const bool post=nrprescale::rede_na_saida(unsigned(cd.Width),cd.Height,nrprescale::quadro.load(std::memory_order_acquire));

 const double teto=nrprescale::post_teto();const unsigned chave=nrprescale::post_chave.load(std::memory_order_relaxed);

 {const auto agora=GetTickCount64();
  if(agora-p.vram_tick>=500){p.vram_tick=agora;std::uint64_t uso{},orc{};
   if(p.vram(uso,orc)){nrposteto::uso.store(uso);nrposteto::orcamento.store(orc);

    auto& G=nrorc::estado().guarda;
    const std::uint64_t b_cena=G.bytes[unsigned(nrorc::Origem::Cena)].load(),b_rosto=G.bytes_rosto(),b_f9=G.bytes[unsigned(nrorc::Origem::F9)].load();
    if(p.uma<0){D3D12_FEATURE_DATA_ARCHITECTURE a{};p.uma=p.device&&SUCCEEDED(p.device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE,&a,sizeof a))&&a.UMA?1:0;}
    const std::uint64_t uso_cena=nrposteto::uso_da_cena(uso,b_rosto,p.uma==1?b_f9:0);
    const bool gravando=agora<nrorc::estado().calib_pausa_ate.load(std::memory_order_relaxed);
    if(!post||!nrposteto::folga_pre.load()){
     const std::uint64_t folga=orc>uso?orc-uso:1;const auto q=nrprescale::unpack_quadro(nrprescale::quadro.load(std::memory_order_acquire));
     nrposteto::folga_pre.store(folga);

     if(post)nrposteto::medida_vram.uso_pre=uso_cena;
     nrposteto::base_partes={b_cena,b_rosto,b_f9,uso};
     const unsigned vram=unsigned(nrposteto::teto_vram(folga,std::uint64_t(q.saida_w)*q.saida_h)*100.+.5);
     nrposteto::teto_vram_centi.store(vram);

     if(nrposteto::automatico()&&vram<nrprescale::post_teto_centi.load(std::memory_order_relaxed))nrprescale::post_teto_centi.store(vram,std::memory_order_relaxed);}

    {const auto q=nrprescale::unpack_quadro(nrprescale::quadro.load(std::memory_order_acquire));
     const std::uint64_t saida_px=std::uint64_t(q.saida_w)*q.saida_h;
     const double medido=nrposteto::amostra_vram(nrposteto::medida_vram,post,uso_cena,nrposteto::folga_pre.load(),saida_px,
                                                 post?nrprescale::efetivo.load(std::memory_order_relaxed):100.,agora,gravando);
     if(!post)nrposteto::medida_vram_relatada=false;
     const double atual=double(nrposteto::teto_vram_centi.load())/100.;
     const double aplicado=nrposteto::aplicar_medido(nrposteto::medida_vram,atual,medido,uso_cena);
     if(medido>0){const unsigned centi=unsigned(aplicado*100.+.5);

      if(nrposteto::teto_vram_centi.exchange(centi)!=centi||!nrposteto::medida_vram_relatada){nrposteto::medida_vram_relatada=true;char d[320];
       const auto& B=nrposteto::base_partes;const auto mb=[](std::int64_t v){return static_cast<long long>(v/(1<<20));};
       const std::int64_t d_uso=std::int64_t(uso)-std::int64_t(B.uso),d_cena=std::int64_t(b_cena)-std::int64_t(B.cena),d_rosto=std::int64_t(b_rosto)-std::int64_t(B.rosto);
       NRLOGF(d,sizeof d,"teto_vram_medido=%.1f%% aplicado=%.1f%% custo=%llu MB rede=%.1f%% folga_pre=%llu MB cena_mb=%lld rosto_mb=%lld f9_mb=%lld jogo_mb=%lld uso_mb=%lld uma=%d",medido,aplicado,
                     static_cast<unsigned long long>((uso_cena>nrposteto::medida_vram.uso_pre?uso_cena-nrposteto::medida_vram.uso_pre:0)>>20),
                     nrprescale::efetivo.load(std::memory_order_relaxed),static_cast<unsigned long long>(nrposteto::folga_pre.load()>>20),
                     mb(d_cena),mb(d_rosto),mb(std::int64_t(b_f9)),mb(d_uso-d_rosto-d_cena-(p.uma==1?std::int64_t(b_f9)-std::int64_t(B.f9):0)),mb(d_uso),p.uma);
       nrcompat::report("post_teto_vram_medido",d);}}}}}}

 {const auto pedida=historico_post(unsigned(cd.Width),cd.Height,post,chave,0,~0ull,teto);
  if(!(pedida==p.orcamento_para)){
   p.orcamento_para=pedida;std::uint64_t uso{},orc{};const bool medido=p.vram(uso,orc);
   const std::uint64_t alocado=p.ready?std::uint64_t(p.width)*p.height*Impl::por_pixel:0;
   p.orcamento_atual=orcamento_dinamico(Impl::orcamento_bytes,Impl::orcamento_piso,Impl::orcamento_reserva,alocado,medido,orc,uso);
   const std::uint64_t custo=std::uint64_t(pedida.w)*pedida.h*Impl::por_pixel;
   if(p.orcamento_atual<Impl::orcamento_bytes&&custo>p.orcamento_atual){char d[240];
    NRLOGF(d,sizeof d,"pedido=%ux%u custo=%llu orcamento=%llu teto=%llu budget=%llu uso=%llu alocado=%llu post=%u",pedida.w,pedida.h,
                  static_cast<unsigned long long>(custo),static_cast<unsigned long long>(p.orcamento_atual),static_cast<unsigned long long>(Impl::orcamento_bytes),
                  static_cast<unsigned long long>(orc),static_cast<unsigned long long>(uso),static_cast<unsigned long long>(alocado),post?1u:0u);
    nrcompat::report("orcamento_historico_dinamico",d);}}}
 const auto hist=historico_post(unsigned(cd.Width),cd.Height,post,chave,Impl::por_pixel,p.orcamento_atual,teto);
 p.post_atual=post;
 if(!p.allocation(hist.w,hist.h,unsigned(cd.Width),cd.Height)){
  char linha[160];
  NRLOGF(linha,sizeof linha,"capture_alocacao pedida=%ux%u alocada=%ux%u bytes=%llu limite=%llu post=%u saida=%llux%u",
                p.recusa.w,p.recusa.h,p.recusa.alocada_w,p.recusa.alocada_h,
                static_cast<unsigned long long>(p.recusa.bytes),
                static_cast<unsigned long long>(p.orcamento_atual),post?1u:0u,static_cast<unsigned long long>(cd.Width),cd.Height);
  if(p.nova(nrcompat::Origem::Captura,p.captura_relatada,"capture_alocacao")){
   char campo[48];NRLOGF(campo,sizeof campo,"%ux%u > %llu MiB",p.recusa.w,p.recusa.h,static_cast<unsigned long long>(p.orcamento_atual>>20));
   p.relatar(nrcompat::Origem::Captura,p.captura_relatada,"capture_alocacao",linha,campo);}
  return reject(linha);
 }
 p.recuperar(nrcompat::Origem::Captura,p.captura_relatada);

 if(post){const auto rede=historico_post(unsigned(cd.Width),cd.Height,true,chave,0,~0ull,teto);
  if(!(rede==hist)&&!(p.post_relatado==hist)){p.post_relatado=hist;char d[160];
   NRLOGF(d,sizeof d,"saida=%llux%u rede=%ux%u historico=%ux%u limite=%llu",static_cast<unsigned long long>(cd.Width),cd.Height,rede.w,rede.h,hist.w,hist.h,static_cast<unsigned long long>(p.orcamento_atual));
   nrcompat::report("post_historico_reduzido",d);}}
 p.srv(p.temporal_heap.Get(),0,color,cor_vista);p.srv(p.temporal_heap.Get(),1,motion,mov_vista);p.srv(p.temporal_heap.Get(),2,depth,prof_vista);p.srv(p.temporal_heap.Get(),7,exposure,DXGI_FORMAT_R32_FLOAT);
 p.late.fonte={color,cor_vista,motion,mov_vista,depth,exposure,prof_vista};

 if(nrexpofresca::fresca.load(std::memory_order_relaxed))expoanel::gravar(c,p.device.Get(),exposure,exposure?state(states[3]):D3D12_RESOURCE_STATE_COMMON);
 ID3D12Resource* inputs[]{color,motion,depth,exposure};for(unsigned i=0;i<(exposure?4u:3u);++i)transition(c,inputs[i],state(states[i]),read_state);
 if(p.histories_read)for(auto& h:p.history)transition(c,h.Get(),read_state,write_state);p.histories_read=false;
 auto& k=p.constants;k={};k.width=p.width;k.height=p.height;k.slice=p.policy.begin();k.mask=p.policy.mask();k.count=Policy::history_count;k.flags=1u|bits_cor_efetivo(cor_vista)|(p.pontual?512u:0u)|(exposure&&p.portao_exposicao?2u:0u);

 if(prof_normal())k.flags|=0x1000000u;
 k.inverse_x=1.f/p.width;k.inverse_y=1.f/p.height;k.depth_absolute=Impl::ajuste(nrreproject::DepthAbsolute,thresholds().depth_absolute);k.current_exposure=k.source_exposure=1;{k.residual_exposure=nrreproject::multiplicador_escala();}
 k.motion_reference=Impl::ajuste(nrreproject::MotionReference,thresholds().motion_reference);

 const float motion_factor=nrreproject::value(nrreproject::controls,nrreproject::MotionFactor);
 k.scale_x=r.motion_scale_x*motion_factor/float(md.Width);k.scale_y=r.motion_scale_y*motion_factor/float(md.Height);

 k.depth_relative=nrreproject::value(nrreproject::controls,nrreproject::Depth);
 k.color_threshold=nrreproject::value(nrreproject::controls,nrreproject::Color);
 k.clamp_strength=nrreproject::value(nrreproject::controls,nrreproject::Clamp);
 k.motion_divergence=nrreproject::value(nrreproject::controls,nrreproject::Divergence);

 k.detail_motion=nrreproject::value(nrreproject::controls,nrreproject::DetailMotion);
 {

  {const float rd=nrreproject::value(nrreproject::controls,nrreproject::RawDivergence);k.pad|=(rd<.5f?0u:rd>15.f?1u:rd>6.f?2u:3u)<<16;}

  k.pad|=(nrreproject::index_of(nrreproject::controls,nrreproject::MotionStrength)&7u)<<20;
  k.pad|=(nrreproject::index_of(nrreproject::controls,nrreproject::MotionStrengthRef)&3u)<<24;

 }
 if(k.slice<Policy::history_count){p.jitter_fatia[k.slice*2]=nrreproject::jitter_atual_x.load();p.jitter_fatia[k.slice*2+1]=nrreproject::jitter_atual_y.load();p.jitter_ok[k.slice]=nrreproject::jitter_valido.load();}
 p.log_constants(k);

 {const bool nn=p.cfg_near_norm;static float near_ref=0.f;static unsigned long long trocas=0;static float near_ult=0.f;
  const float nq=nrreproject::near_atual.load(std::memory_order_relaxed);
  if(nn&&nq>0.f){if(!(near_ref>0.f))near_ref=nq;const float f=near_ref/nq;if(f!=1.f&&std::isfinite(f)&&f>.01f&&f<100.f)std::memcpy(&k.age,&f,4);
   if(nq!=near_ult){++trocas;if(trocas<=8||(trocas&255u)==0)nrlogfila::linha(L"nr_reproject.log","pid=%lu NEAR_NORM near=%.5f ref=%.5f fator=%.4f trocas=%llu\n",GetCurrentProcessId(),double(nq),double(near_ref),double(f),trocas);near_ult=nq;}}}
 {nrtgpu::Escopo tg(c,nrtgpu::Captura);p.bind(c,p.temporal_heap.Get(),p.temporal_root.Get(),p.capture_pso.Get(),32,&k,8);c->Dispatch((p.width+7)/8,(p.height+7)/8,1);}
 k.age=0;
 for(auto& h:p.history)transition(c,h.Get(),write_state,read_state);p.histories_read=true;

 {nrtgpu::Escopo tg(c,nrtgpu::RostoRecorte);
 rj_recorte(r.command_list,color,unsigned(cor_vista),post);
 if(nrrj::npc2.load(std::memory_order_relaxed))for(int kn=1;kn<=nrrj::npc_n();++kn){nrrj::Inst i1(kn);rj_recorte(r.command_list,color,unsigned(cor_vista),post);}
 }
 for(unsigned i=0;i<(exposure?4u:3u);++i)transition(c,inputs[i],read_state,state(states[i]));p.captured_command=r.command_list;return true;
}
Result Backend::compose(void* command,void* destination,unsigned destination_state,void* residual,int job,float exposure,unsigned tone){
 if(!p_)return Result::Failed;auto& p=*p_;p.atualizar_ao_vivo();int age=p.policy.age(job);p.trace_expo=exposure;

 bool segurar=false;
 if(!p.ready||age<0){
  if(p.pode_segurar(command))segurar=true;
  else{++p.skipped;p.log(job,age,"old_or_missing_source");return Result::Skipped;}
 }

 if(p.composed_frame||command!=p.captured_command){++p.skipped;p.log(job,age,"duplicate_or_different_command");return Result::Skipped;}
 auto* c=static_cast<ID3D12GraphicsCommandList*>(command);auto* dst=static_cast<ID3D12Resource*>(destination);auto* res=static_cast<ID3D12Resource*>(residual);

 nrtgpu::quadro();nrtgpu::Escopo tg_total(c,nrtgpu::ComposeTotal);unsigned tg=0;
 auto tg_troca=[&](unsigned prox){nrtgpu::fim(c,tg);tg=prox<nrtgpu::Passes?nrtgpu::inicio(c,prox):0u;};
 struct TgFecha{ID3D12GraphicsCommandList* c;unsigned& t;~TgFecha(){nrtgpu::fim(c,t);}} tg_fecha{c,tg};
 auto unsupported=[&]{++p.unsupported;p.log(job,age,"compose_unsupported");
  if(!p.nova(nrcompat::Origem::Composicao,p.composicao_relatada,"compose_unsupported"))return Result::Unsupported;

  D3D12_RESOURCE_DESC dd{};if(dst)dd=dst->GetDesc();char detalhe[200],campo[48];
  NRLOGF(detalhe,sizeof detalhe,"destino=fmt%u_%llux%u_dim%u_mips%u_amostras%u_flags0x%x captura=%ux%u exposicao=%.6g",unsigned(dd.Format),static_cast<unsigned long long>(dd.Width),dd.Height,
                unsigned(dd.Dimension),unsigned(dd.MipLevels),unsigned(dd.SampleDesc.Count),unsigned(dd.Flags),p.width,p.height,static_cast<double>(exposure));
  NRLOGF(campo,sizeof campo,"destino fmt %u %llux%u",unsigned(dd.Format),static_cast<unsigned long long>(dd.Width),dd.Height);
  p.relatar(nrcompat::Origem::Composicao,p.composicao_relatada,"compose_unsupported",detalhe,campo);
  return Result::Unsupported;};
 if(!c||!dst||(!segurar&&(!texture(res,DXGI_FORMAT_R16G16B16A16_FLOAT)||!device_matches(p.device.Get(),res)))||!device_matches(p.device.Get(),c)||!device_matches(p.device.Get(),dst)||!std::isfinite(exposure)||exposure<=0)return unsupported();
 const auto dd=dst->GetDesc();const auto rd=segurar?D3D12_RESOURCE_DESC{}:res->GetDesc();if(dd.Width!=p.comp_w||dd.Height!=p.comp_h||(!segurar&&(rd.Width!=p.comp_w||rd.Height!=p.comp_h))||dd.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||dd.DepthOrArraySize!=1||dd.MipLevels!=1||dd.SampleDesc.Count!=1||!(dd.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)||!format_support(p.device.Get(),dd.Format,true))return unsupported();
 p.recuperar(nrcompat::Origem::Composicao,p.composicao_relatada);

 const unsigned variante=p.ganho_cur;

 ID3D12DescriptorHeap* const warp_heap=p.warp_heap_override?p.warp_heap_override:p.temporal_heap.Get();

 ID3D12Resource* res_warp=res;
 tg_troca(nrtgpu::Reduzir);
 if(p.reduzido){
  if(!p.reduzir_pso||!p.reduzir_heap||!p.res_reduzido||!p.post_pso||!p.post_heap){
   if(p.nova(nrcompat::Origem::Composicao,p.composicao_relatada,"compose_post_reduzido"))
    p.relatar(nrcompat::Origem::Composicao,p.composicao_relatada,"compose_post_reduzido","passe Reduzir ou ComposePost sem PSO/heap",nullptr);
   ++p.unsupported;p.log(job,age,"compose_post_reduzido");return Result::Unsupported;}
 }
 if(p.reduzido&&!segurar){
  const bool gravando_tardia=p.suprimir_despejo||p.reduzir_heap_override!=nullptr;
  if(p.red_cache_lig&&!p.reduzir_heap_override&&p.red_cache_valido&&p.res_reduzido_read&&p.red_cache_res==res&&p.red_cache_job==job&&p.red_cache_dst==p.res_reduzido.Get()){
   ++p.red_cache_hits;res_warp=p.res_reduzido.Get();
  }else{
  ID3D12DescriptorHeap* const rh=p.reduzir_heap_override?p.reduzir_heap_override:p.reduzir_heap.Get();
  p.srv(rh,6,res,rd.Format);
  if(p.res_reduzido_read)transition(c,p.res_reduzido.Get(),read_state,write_state);
  p.bind(c,rh,p.temporal_root.Get(),p.reduzir_pso.Get(),32,&p.constants,8);c->Dispatch((p.width+7)/8,(p.height+7)/8,1);
  transition(c,p.res_reduzido.Get(),write_state,read_state);p.res_reduzido_read=true;res_warp=p.res_reduzido.Get();
  ++p.red_cache_misses;
  p.red_cache_valido=!gravando_tardia;p.red_cache_res=res;p.red_cache_job=job;p.red_cache_dst=p.res_reduzido.Get();
  if(p.red_cache_lig&&(p.red_cache_misses==1||(p.red_cache_misses&4095u)==0)){
   nrlogfila::linha(L"nr_reproject.log","pid=%lu CUSTO_REDUZIR_CACHE hits=%llu misses=%llu\n",GetCurrentProcessId(),
    static_cast<unsigned long long>(p.red_cache_hits),static_cast<unsigned long long>(p.red_cache_misses));}
  }
 }
 nrcusto::marca(c,nrcusto::ReduzirFim);tg_troca(segurar?nrtgpu::Cadeia:nrtgpu::Warp);
 if(!segurar)p.srv(warp_heap,6,res_warp,res_warp==res?rd.Format:DXGI_FORMAT_R16G16B16A16_FLOAT);
 p.uav(p.compose_heap.Get(),compose_variante*variante+compose_uavs,dst,dd.Format);
 if(segurar){++p.segurados_seguidos;++p.segurados_total;nrposteto::segurados_total.fetch_add(1,std::memory_order_relaxed);

  if(!p.warped_read){transition(c,p.warped.Get(),write_state,read_state);p.warped_read=true;}
  {float f=0.f;

   if(p.segura_fade>0&&p.segurados_seguidos>p.segura_fade&&p.segura_max>p.segura_fade){
    auto rampa=[&](unsigned n){return n<=p.segura_fade?1.f:(std::max)(1e-3f,1.f-float(n-p.segura_fade)/float(p.segura_max-p.segura_fade));};
    f=rampa(p.segurados_seguidos)/rampa(p.segurados_seguidos-1u);}
   p.cadeia_fonte=0u;p.estabilizar(c,f,false,true,false,p.cadeia_segue(exposure));}

  p.calcular_base_tom(c,exposure,tone);
  if(p.cro_ligado())p.croma(c,exposure,tone,p.ultimo_cro(exposure));
  if(p.lg_ligado())p.luz_grossa(c,exposure,tone,p.ultimo_lg(exposure));
  if(p.su_cfg.ligado())p.suave(c,exposure,tone);
  p.cadeia_fechar(c);
 }else{
 p.segurados_seguidos=0;
 if(p.warped_read)transition(c,p.warped.Get(),read_state,write_state);p.warped_read=false;p.constants.age=unsigned(age);
 {

  const float sinal=nrreproject::value(nrreproject::controls,nrreproject::JitterComp);
  const unsigned n=Policy::history_count,atual=p.constants.slice%n,fonte=(p.constants.slice+n-unsigned(age)%n)%n;
  p.constants.jitter_x=p.constants.jitter_y=0.f;

  if(std::isfinite(exposure)&&exposure>0.f)p.constants.current_exposure=p.constants.source_exposure=exposure;
  if(sinal!=0.f&&p.jitter_ok[atual]&&p.jitter_ok[fonte]&&p.width&&p.height){
   p.constants.jitter_x=sinal*(p.jitter_fatia[fonte*2]-p.jitter_fatia[atual*2])/float(p.width);
   p.constants.jitter_y=sinal*(p.jitter_fatia[fonte*2+1]-p.jitter_fatia[atual*2+1])/float(p.height);}
 }
 {

  if(age>=0)p.idade_media=p.idade_media>0.f?p.idade_media+.1f*(float(age)-p.idade_media):float(age);
 }
 p.bind(c,warp_heap,p.temporal_root.Get(),p.warp_pso.Get(),32,&p.constants,8);c->Dispatch((p.width+7)/8,(p.height+7)/8,1);transition(c,p.warped.Get(),write_state,read_state);p.warped_read=true;
 p.diag_rodar_warp(c,res_warp,res_warp==res?rd.Format:DXGI_FORMAT_R16G16B16A16_FLOAT);
 nrcusto::marca(c,nrcusto::WarpFim);tg_troca(nrtgpu::Fill);

 const auto despejo=p.reservar_despejo(c);

 if(despejo.leitura){{nrlogfila::linha(L"nr_reproject.log","pid=%lu frame=%llu DUMPINFO indice=%u job=%d age=%d\n",GetCurrentProcessId(),p.policy.frame,despejo.indice,job,age);}}
 p.cadeia_fonte=0u;
 if(p.preenchimento>0&&p.fill_pso)p.preencher(c);
 nrcusto::marca(c,nrcusto::FillFim);tg_troca(nrtgpu::Estab);

 {const auto eb=nrestab::base(nrreproject::value(nrreproject::controls,nrreproject::Stabilize),p.post_atual,p.post_estab);
  float alfa=eb.alfa;bool sem_limite=eb.sem_limite;p.est_fonte_base=eb.fonte;
  {const bool troca=job!=p.est_ultimo_job;p.est_ultimo_job=job;
   const bool fg=nrreproject::fg_ativo();
   p.est_luz_mov=nrestab::luz_andando(troca,fg,p.fg_troca_luz,p.fg_troca_sempre,p.post_luz_mov,&p.est_fonte_mov);

   if(p.fg_log_estado!=int(fg)){p.fg_log_estado=int(fg);{
    nrlogfila::linha(L"nr_reproject.log","pid=%lu FG_TROCA fg_ativo=%d NeuralFgTrocaLuz=%.3f NeuralFgTrocaLuzSempre=%d NeuralPostLuzMov=%.3f NeuralFgCrossfade=%d estabilizador_post=%.3f mov=%.3f quadro=%llu\n",
     GetCurrentProcessId(),int(fg),p.fg_troca_luz,int(p.fg_troca_sempre),p.post_luz_mov,int(p.fg_cross),p.post_estab,p.post_alfa_mov,static_cast<unsigned long long>(p.policy.frame));}}}

  p.est_cross=0.f;p.est_rosto=0.f;
  const bool cross_on=p.fg_cross&&(p.fg_cross_sempre||nrreproject::fg_ativo());
  if(cross_on||((p.post_estab_rosto||p.post_det_rampa)&&alfa>0.f&&alfa<1.f&&sem_limite)){
   if(job!=p.cross_job){if(p.cross_job>0&&p.cross_j>0)p.cross_P=p.cross_P>0.f?p.cross_P+.125f*(float(p.cross_j)-p.cross_P):float(p.cross_j);p.cross_job=job;p.cross_j=0;}
   const float P=std::clamp(p.cross_P>0.f?p.cross_P:2.f,1.f,8.f);
   const float a=1.f/(std::max)(1.f,P-float(p.cross_j));++p.cross_j;
   if(cross_on){p.est_cross=std::clamp(a,.05f,.999f);alfa=p.est_cross;sem_limite=true;}
   else p.est_rosto=std::clamp(a,.05f,.999f);
  }
  if(alfa>0.f&&alfa<1.f&&p.garantir_est())p.estabilizar(c,alfa,sem_limite,false,p.est_cross>0.f||(sem_limite&&p.post_filtro_mov),p.cadeia_segue(exposure));else p.est_valido=false;}
 nrcusto::marca(c,nrcusto::EstabFim);tg_troca(nrtgpu::Cadeia);

 p.calcular_base_tom(c,exposure,tone);
 if(p.cro_ligado())p.croma(c,exposure,tone,p.ultimo_cro(exposure));
 if(p.lg_ligado())p.luz_grossa(c,exposure,tone,p.ultimo_lg(exposure));

 if(p.su_cfg.ligado())p.suave(c,exposure,tone);
 p.cadeia_fechar(c);
 p.gravar_despejo(c,res_warp,despejo);
 }
 nrcusto::marca(c,nrcusto::CadeiaFim);tg_troca(nrtgpu::Passes);
 if(p.reduzido){

  p.ganho_valido=false;p.luz_valido=false;p.halo_exc_raio=0u;p.corte_raio_comp=0u;

  const bool masc_ok=p.masc_bits&&p.mascara_pronta();

  const auto modo_masc=nrestext::modo_halo(p.masc_bits,p.masc_prof,masc_ok);
  if(modo_masc==nrestext::ModoHalo::Personagem){nrtgpu::Escopo tm(c,nrtgpu::MascaraHalo);p.mascara_halo(c);}
  else if(modo_masc==nrestext::ModoHalo::SoProfundidade){nrtgpu::Escopo tm(c,nrtgpu::MascaraHalo);p.mascara_halo(c,false);}
  ID3D12DescriptorHeap* const ph=p.post_heap.Get();
  p.uav(ph,2,dst,dd.Format);
  {nrtgpu::Escopo tl(c,nrtgpu::RostoLimpa);rj_limpa_preparar(c,dst,destination_state,p.policy.frame^(std::uint64_t(nrrj::epoca_backend.load(std::memory_order_relaxed))<<44));}
  tg_troca(nrtgpu::ComposePost);
  transition(c,dst,D3D12_RESOURCE_STATES(destination_state),write_state);
  p.tl_medir_jogo(c,dst,dd.Format,p.comp_w,p.comp_h);
  ComposeConstants k=p.constantes_compose(teto_unico(tone)&~6u,p.comp_w,p.comp_h,exposure);k.p2=protege_realce_codigo();

  const bool npc_cena=ph==p.post_heap.Get()&&p.cnpc_cfg.modo&&p.cena_npc(c,dst,dd.Format,&k);
  if(!npc_cena&&!(masc_ok&&p.mascara_post(c,dst,dd.Format,&k)))p.bind(c,ph,p.post_root.Get(),p.post_pso.Get(),12,&k,2);
  c->Dispatch((p.comp_w+7)/8,(p.comp_h+7)/8,1);order_uav(c,dst);tg_troca(nrtgpu::Passes);
  if(p.diag_gravando()){p.diag_copiar(c,dst,p.diag_cena,nrreproject::diag_cena_recurso,L"nr_diag_cena");if(p.warped_read)nrreproject::diag_realce_recurso.store(p.warped.Get());}

  if(!segurar){nrtgpu::Escopo tr(c,nrtgpu::Rosto);rj_compor(c,dst,unsigned(dd.Format),job,age,exposure,tone);}
  if(!segurar&&nrrj::npc2.load(std::memory_order_relaxed)){nrtgpu::Escopo tn(c,nrtgpu::RostoNpc);for(int kn=1;kn<=nrrj::npc_n();++kn){nrrj::Inst i1(kn);rj_compor(c,dst,unsigned(dd.Format),job,age,exposure,tone);}}
  {nrtgpu::Escopo tc(c,nrtgpu::Camada);rj_camada(c,dst,exposure,tone);}
  if(p.diag_gravando())p.diag_copiar(c,dst,p.diag_rostos,nrreproject::diag_rostos_recurso,L"nr_diag_rostos");
  transition(c,dst,write_state,D3D12_RESOURCE_STATES(destination_state));
  nrcusto::marca(c,nrcusto::ComposeFim);
  p.composed_frame=true;++p.composed;nrreproject::idade_composta.store(age);
  nrreproject::base_fatia.store(p.constants.slice);nrreproject::base_recurso.store(p.history[0].Get());
  p.log(job,age,segurar?"ok_segurado_post_reduzido":"ok_post_reduzido");return Result::Composed;
 }
 if(p.modo_post()){

  p.ganho_valido=false;p.luz_valido=false;
  p.perder_post("post_lost_sobrescrito");p.post={true,command,job,exposure,tone};p.composed_frame=true;++p.composed;nrreproject::idade_composta.store(age);
  nrreproject::base_fatia.store(p.constants.slice);nrreproject::base_recurso.store(p.history[0].Get());
  p.log(job,age,segurar?"ok_segurado_adiado_pos_fsr":"ok_adiado_pos_fsr");return Result::Composed;
 }
 transition(c,dst,D3D12_RESOURCE_STATES(destination_state),write_state);
 ComposeConstants k=p.constantes_compose(tone&~6u,p.width,p.height,exposure);

 const float protecaoLuz=nrreproject::value(nrreproject::controls,nrreproject::LightGuard);
 const bool dividir=p.luz_ok&&protecaoLuz>1.f;

 {const float det=nrreproject::value(nrreproject::controls,nrreproject::DetailGuard);const unsigned antes=k.tone;
  k.tone=dividir?nrrealce::tom_dividida(k.tone,protecaoLuz,det):nrrealce::tom(k.tone,protecaoLuz,det);
  registrar_realce(dividir?"pre_dividida":"pre_teto_unico",antes,k.tone,protecaoLuz,det);}

 const bool luzProf=dividir&&nrreproject::value(nrreproject::controls,nrreproject::LightGuideDepth)>.5f;
 if(dividir){

  const float gb=nrreproject::ln_protecao(protecaoLuz),ga=nrreproject::ln_protecao(nrreproject::value(nrreproject::controls,nrreproject::DetailGuard));
  k.p0|=2u;std::memcpy(&k.p3,&gb,sizeof gb);std::memcpy(&k.p4,&ga,sizeof ga);

  const float forcaExc=nrreproject::value(nrreproject::controls,nrreproject::HaloExcess);
  if(forcaExc>0.f&&p.luz_prox_pso&&p.luz_halo_pso)
   p.halo_exc_raio=2u*((nrreproject::halo_excesso_raio(p.halo_exc_r100,p.pct_raio())+1u)/2u);
  else p.halo_exc_raio=0u;
  if(p.halo_exc_raio>24u)p.halo_exc_raio=24u;
  unsigned forca64=p.halo_exc_raio?unsigned(std::lround(std::clamp(forcaExc,0.f,3.9f)*64.f)):0u;

  {const unsigned nivel=unsigned(nrreproject::value(nrreproject::controls,nrreproject::HaloCutMoving)+.5f);
   p.corte_raio_comp=(p.luz_prox_pso&&p.luz_halo_pso)?nrreproject::corte_raio_compose(nivel,p.pct_raio()):0u;
   if(p.corte_raio_comp){p.halo_exc_raio=std::max(p.halo_exc_raio,p.corte_raio_comp);forca64=255u;}}

  {const float forcaTeto=p.cfg_halo_teto_forca;
   const double raioTeto=p.cfg_halo_teto_raio;
   p.halo_teto_raio=0u;
   if(forcaTeto>0.f&&p.luz_prox_pso&&p.luz_halo_pso){
    const double pct=p.pct_raio();const double r=(pct>0.&&pct<100.)?raioTeto*100./pct:raioTeto;
    const unsigned u=unsigned(std::min(30.,r)+.5);p.halo_teto_raio=std::min(30u,2u*((u+1u)/2u));
    p.halo_exc_raio=0u;p.corte_raio_comp=0u;forca64=64u;
    static bool registrado=false;
    if(!registrado){registrado=true;{nrlogfila::linha(L"nr_reproject.log","pid=%lu HALO_TETO forca=%.3f raio100=%.1f raio_comp=%u\n",GetCurrentProcessId(),double(forcaTeto),raioTeto,p.halo_teto_raio);}}
   }
   if(p.halo_teto_raio)p.iluminar(c,dst,dd.Format,k,luzProf,p.halo_teto_raio,forca64,false,true);
   else p.iluminar(c,dst,dd.Format,k,luzProf,p.halo_exc_raio,forca64,p.corte_raio_comp!=0u);}
 }else{p.luz_valido=false;p.halo_exc_raio=0u;p.corte_raio_comp=0u;p.halo_teto_raio=0u;}
 const float limite=nrreproject::value(nrreproject::controls,nrreproject::GainLimit);
 const bool limitar=p.ganho_ok&&limite>0.f;
 if(limitar){
  const unsigned n=Policy::history_count,atual=p.constants.slice%n,anterior=(atual+n-1)%n;
  const bool valido=p.ganho_valido&&p.ganho_dividido==dividir&&p.ganho_quadro+1==p.policy.frame&&((p.policy.mask()>>anterior)&1u);
  k.p1=1u|(valido?2u:0u)|(p.pontual?4u:0u)|(atual<<8)|(anterior<<16);
  const float passo=std::log1p(limite);std::memcpy(&k.p2,&passo,sizeof passo);
  k.prof_rel=p.constants.depth_relative;k.prof_abs=p.constants.depth_absolute;
  p.ganho_para(c,variante,write_state);p.ganho_para(c,1u-variante,read_state);
  if(valido)++p.ganho_limitados;
 }

 p.halo_rede_pct=nrreproject::value(nrreproject::controls,nrreproject::HaloNetScale)>.5f?p.pct_raio():100.;
 if(const unsigned hb=halo_bits(unsigned(nrreproject::value(nrreproject::controls,nrreproject::HaloReduce)+.5f),p.halo_ini,p.halo_rede_pct)){
  k.p0|=hb;if(!limitar)k.p1|=(p.constants.slice%Policy::history_count)<<8;
  ++p.halo_quadros;p.halo_ultimo=hb;
 }

 if(luzProf){k.p0|=32u;if(!limitar)k.p1|=(p.constants.slice%Policy::history_count)<<8;}

 if(p.halo_exc_raio){k.p0|=128u|(((p.halo_exc_raio/2u)&15u)<<28);if(!limitar)k.p1|=(p.constants.slice%Policy::history_count)<<8;++p.halo_exc_quadros;}

 if(p.corte_raio_comp&&p.halo_exc_raio){
  const unsigned nivel=unsigned(nrreproject::value(nrreproject::controls,nrreproject::HaloCutMoving)+.5f)&3u;
  const unsigned idade=unsigned(std::lround(std::clamp(p.idade_media,0.f,15.f)));
  k.p1|=(nivel<<3)|(idade<<24);++p.corte_quadros;
 }

 if(p.halo_teto_raio){
  const unsigned forca15=p.cfg_halo_teto_forca15;
  if(forca15){k.p0|=((p.halo_teto_raio/2u)&15u)<<28;k.p1|=32u|(forca15<<28);if(!limitar)k.p1|=(p.constants.slice%Policy::history_count)<<8;++p.halo_teto_quadros;}
 }
 {ID3D12DescriptorHeap* heaps[]{p.compose_heap.Get()};c->SetDescriptorHeaps(1,heaps);c->SetComputeRootSignature(p.compose_root.Get());c->SetPipelineState(p.compose_pso.Get());
  c->SetComputeRoot32BitConstants(0,16,&k,0);c->SetComputeRootDescriptorTable(1,p.gpu(p.compose_heap.Get(),compose_variante*variante));c->SetComputeRootDescriptorTable(2,p.gpu(p.compose_heap.Get(),compose_variante*variante+compose_uavs));}
 c->Dispatch((p.width+7)/8,(p.height+7)/8,1);order_uav(c,dst);
 transition(c,dst,write_state,D3D12_RESOURCE_STATES(destination_state));
 nrcusto::marca(c,nrcusto::ComposeFim);
 if(limitar){p.ganho_valido=true;p.ganho_dividido=dividir;p.ganho_quadro=p.policy.frame;p.ganho_cur=1u-variante;}else p.ganho_valido=false;
 p.composed_frame=true;++p.composed;nrreproject::idade_composta.store(age);
 nrreproject::base_fatia.store(p.constants.slice);nrreproject::base_recurso.store(p.history[0].Get());
 p.log(job,age,segurar?"ok_segurado":"ok");return Result::Composed;
}

Result Backend::compose_post(void* command,void* output,std::uint32_t ffx_state){
 if(!p_)return Result::Failed;auto& p=*p_;
 if(!p.post.ativo||command!=p.post.command||command!=p.captured_command)return Result::Skipped;
 p.post.ativo=false;
 auto* c=static_cast<ID3D12GraphicsCommandList*>(command);auto* out=static_cast<ID3D12Resource*>(output);
 nrtgpu::Escopo tg_post(c,nrtgpu::PostTardio);
 auto recusar=[&](const char* motivo,const D3D12_RESOURCE_DESC* od){p.post_recusado=true;++p.post_refused;p.log_post(p.post.job,motivo,od);return Result::Unsupported;};

 const auto estado=state(ffx_state);
 if(estado==D3D12_RESOURCE_STATES(~0u)||(c&&c->GetType()==D3D12_COMMAND_LIST_TYPE_COMPUTE&&!nrffx_estado::valido_em_compute(unsigned(estado))))
  return recusar("post_unsupported_state_fallback_pre",nullptr);
 if(!c||!out||!device_matches(p.device.Get(),c)||!device_matches(p.device.Get(),out))return recusar("post_unsupported_device_fallback_pre",nullptr);
 const auto od=out->GetDesc();

 if(od.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||od.DepthOrArraySize!=1||od.MipLevels!=1||od.SampleDesc.Count!=1||!(od.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)||
    od.Width<p.width||od.Height<p.height||od.Width>16384||od.Height>16384||!format_support(p.device.Get(),od.Format,true))return recusar("post_unsupported_format_fallback_pre",&od);
 if(!p.warped_read){transition(c,p.warped.Get(),write_state,read_state);p.warped_read=true;}
 ID3D12DescriptorHeap* const ph=p.post_heap.Get();
 p.uav(ph,2,out,od.Format);

 if(estado==write_state)order_uav(c,out);else transition(c,out,estado,write_state);
 ComposeConstants k=p.constantes_compose(teto_unico(p.post.tone)&~6u,unsigned(od.Width),od.Height,p.post.exposure);k.p2=protege_realce_codigo();
 p.bind(c,ph,p.post_root.Get(),p.post_pso.Get(),12,&k,2);
 c->Dispatch((unsigned(od.Width)+7)/8,(od.Height+7)/8,1);order_uav(c,out);
 transition(c,out,write_state,estado);
 ++p.post_composed;
 if(!p.post_formato_logado){p.post_formato_logado=true;p.log_post(p.post.job,"ok_pos_fsr",&od);}
 return Result::Composed;
}
}
namespace nrreproject {
namespace {

constexpr unsigned lados_final=10;

std::atomic<unsigned long long> f9_liberados_n{0},f9_liberados_bytes{0};
void f9_conta_libera(UINT64 n) noexcept {nrorc::estado().guarda.liberar(nrorc::Origem::F9,n);f9_liberados_n.fetch_add(1,std::memory_order_relaxed);f9_liberados_bytes.fetch_add(n,std::memory_order_relaxed);}

void tela_conta_f9(bool criar,std::uint64_t n) noexcept {
 if(!criar){f9_conta_libera(n);return;}
 nrorc_vram::pausar_calib();nrorc::EscopoOrigem eo(nrorc::Origem::F9);nrorc_vram::registrar(nrorc::Api::Recurso,n,"f9_tela");
}
struct Buffers {
 ComPtr<ID3D12Resource> leitura[lados_final];UINT64 tamanho[lados_final]{};
 Buffers()=default;Buffers(const Buffers&)=delete;Buffers& operator=(const Buffers&)=delete;
 Buffers(Buffers&& o) noexcept {for(unsigned k=0;k<lados_final;++k){leitura[k]=std::move(o.leitura[k]);tamanho[k]=o.tamanho[k];o.tamanho[k]=0;}}
 Buffers& operator=(Buffers&& o) noexcept {if(this!=&o){soltar();for(unsigned k=0;k<lados_final;++k){leitura[k]=std::move(o.leitura[k]);tamanho[k]=o.tamanho[k];o.tamanho[k]=0;}}return *this;}
 ~Buffers(){soltar();}
 void soltar_lado(unsigned k) noexcept {if(leitura[k]){leitura[k].Reset();if(tamanho[k])f9_conta_libera(tamanho[k]);}tamanho[k]=0;}
 void soltar() noexcept {for(unsigned k=0;k<lados_final;++k)soltar_lado(k);}
};
struct Final {
 Buffers b;D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp[lados_final]{};UINT altura[lados_final]{};unsigned formato[lados_final]{};
 unsigned modo{},indice{},espera{4};int idade{-1};std::uint64_t quadro{};
 bool exato{};FinalExtras extras{};double qpc_ms{};

 bool seguro{};const volatile std::uint32_t* marca{};std::uint32_t valor{};
};

std::mutex final_mutex;std::vector<Final>& finais=*new std::vector<Final>;std::vector<Buffers>& reserva=*new std::vector<Buffers>;unsigned final_indice{};

std::mutex final_cemiterio_mutex;std::vector<Buffers>& final_cemiterio=*new std::vector<Buffers>;

struct FinalMarcas {ComPtr<ID3D12Device> dev;ComPtr<ID3D12Resource> buf;volatile std::uint32_t* m{};};
FinalMarcas* final_marcas(ID3D12Device* d){
 static FinalMarcas* atual{};if(atual&&atual->dev.Get()==d)return atual;
 auto* f=new(std::nothrow) FinalMarcas;if(!f)return nullptr;
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=4u*final_teto_max;bd.Height=1;
 bd.DepthOrArraySize=1;bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;void* m{};
 if(FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&f->buf)))||FAILED(f->buf->Map(0,nullptr,&m))||!m){delete f;return nullptr;}nrd3d::nomear(f->buf.Get(),L"nr_f9_final");
 std::memset(m,0,4u*final_teto_max);f->m=static_cast<volatile std::uint32_t*>(m);f->dev=d;atual=f;return f;
}
bool final_seguro_ini(){static const bool v=[]{double x=1.;return !(ler_ini(L"NeuralDumpFinalSeguro",x)&&x==0.);}();return v;}
bool final_movimento{};

unsigned final_maximo_(){const unsigned t=final_teto_sessao.load(std::memory_order_relaxed);return t<final_teto_max?t:final_teto_max;}

std::atomic<unsigned long long> escrita_bytes{0};std::atomic<long long> escrita_t0{0};std::atomic<int> escritores{0},escritores_max{0};
void escrever_final_(Final f,unsigned long long& bytes);
void escrever_final(Final f){
(void)f;
}
void escrever_final_(Final f,unsigned long long& bytes){
(void)f;(void)bytes;
}
}

namespace {
unsigned final_reserva_modo(){if(NR_SEM_GRAVACAO_K)return 0u;static const unsigned v=[]{double x=0.;return ler_ini(L"NeuralGravarF9Reserva",x)?(x==1.?1u:x==2.?2u:0u):0u;}();return v;}
bool final_movimento_ini(){static const bool v=[]{double x=0.;return ler_ini(L"NeuralDumpFinalMovimento",x)&&x==1.;}();return v;}
std::atomic<bool> reserva_voo{false};
std::atomic<unsigned> reserva_falhas{0};
bool reserva_gravando{};
std::atomic<unsigned long long> reserva_aloc_chamadora{0},reserva_aloc_fundo{0},reserva_lancamentos{0},reserva_prontas{0},reserva_descartadas{0};
unsigned reserva_modo_efetivo(){return reserva_falhas.load(std::memory_order_relaxed)>=3u?0u:final_reserva_modo();}
void log_reserva(const char* t){
(void)t;
}

Buffers alocar_lote(ID3D12Device* dev,const UINT64* total,const bool* lado,DWORD chamadora,UINT64& bytes){
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;Buffers b;
 for(unsigned k=0;k<lados_final;++k){if(!lado[k])continue;
  D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=total[k];bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  (GetCurrentThreadId()==chamadora?reserva_aloc_chamadora:reserva_aloc_fundo).fetch_add(1);
  if(SUCCEEDED(dev->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&b.leitura[k])))){nrd3d::nomear(b.leitura[k].Get(),L"nr_f9_reserva");b.tamanho[k]=total[k];bytes+=total[k];{nrorc::EscopoOrigem eo(nrorc::Origem::F9);nrorc_vram::registrar(nrorc::Api::Recurso,total[k],"f9_readback");}}}
 return b;
}

bool reserva_serve(const std::vector<Buffers>& v,ID3D12Resource* const* rs,const UINT64* total){
 for(const auto& b:v)for(unsigned k=0;k<lados_final;++k)if(rs[k]&&b.tamanho[k]<total[k])return false;
 return true;
}

void soltar_no_fundo(std::vector<Buffers>& v){
 if(v.empty())return;nrorc_vram::pausar_calib();std::vector<Buffers> w;w.swap(v);
 try{std::thread([x=std::move(w)]()mutable{x.clear();}).detach();}catch(...){}
}

void lancar_reserva(ID3D12Device* dev,const UINT64* total,ID3D12Resource* const* rs,unsigned n){
 if(!n||reserva_voo.exchange(true))return;
 nrorc_vram::pausar_calib();
 std::array<UINT64,lados_final> t{};std::array<bool,lados_final> l{};for(unsigned k=0;k<lados_final;++k){t[k]=total[k];l[k]=rs[k]!=nullptr;}
 ComPtr<ID3D12Device> d(dev);const unsigned modo=final_reserva_modo();reserva_lancamentos.fetch_add(1);
 {char linha[96];NRLOGF(linha,sizeof linha,"pedido modo=%u lotes=%u",modo,n);log_reserva(linha);}
 try{std::thread([d,t,l,n,modo]{
   LARGE_INTEGER t0{},t1{},q{};QueryPerformanceCounter(&t0);
   std::vector<Buffers> novos;novos.reserve(n);UINT64 bytes=0;unsigned falhos=0;
   for(unsigned j=0;j<n;++j){Buffers b=alocar_lote(d.Get(),t.data(),l.data(),0,bytes);bool completo=true;
    for(unsigned k=0;k<lados_final;++k)if(l[k]&&!b.tamanho[k])completo=false;
    if(completo)novos.push_back(std::move(b));else ++falhos;}
   QueryPerformanceCounter(&t1);QueryPerformanceFrequency(&q);const double ms=q.QuadPart?double(t1.QuadPart-t0.QuadPart)*1000.0/double(q.QuadPart):0.;
   std::vector<Buffers> soltar;bool usado=false;size_t na_reserva=0;
   {std::lock_guard<std::mutex> trava(final_mutex);
    if(despejo_final.load()>0||modo==2){for(auto& b:novos)reserva.push_back(std::move(b));usado=true;}else soltar.swap(novos);
    na_reserva=reserva.size();reserva_voo.store(false);}
   if(falhos)reserva_falhas.fetch_add(1);
   (usado?reserva_prontas:reserva_descartadas).fetch_add(1);
   char linha[200];NRLOGF(linha,sizeof linha,"%s modo=%u lotes=%u falhos=%u reserva=%zu MB=%.1f ms=%.1f thread=%lu",usado?"pronta":"descartada",modo,n,falhos,na_reserva,
    double(bytes)/(1024.*1024.),ms,GetCurrentThreadId());log_reserva(linha);
   soltar.clear();
  }).detach();}catch(...){reserva_voo.store(false);reserva_falhas.fetch_add(1);}
}
}
FinalReservaInfo final_reserva_info() noexcept{
 FinalReservaInfo i{};i.modo=final_reserva_modo();
 {std::lock_guard<std::mutex> trava(final_mutex);i.lotes=unsigned(reserva.size());}
 i.voo=reserva_voo.load()?1u:0u;i.falhas=reserva_falhas.load();
 i.alocacoes_chamadora=reserva_aloc_chamadora.load();i.alocacoes_fundo=reserva_aloc_fundo.load();
 i.lancamentos=reserva_lancamentos.load();i.prontas=reserva_prontas.load();i.descartadas=reserva_descartadas.load();
 return i;
}
void preparar_final(void* command,void* input,void* output,unsigned modo) noexcept{
 (void)command;(void)input;(void)output;(void)modo;
}
bool despejar_final(void* command,void* input,std::uint32_t estado_entrada,void* output,std::uint32_t estado_saida,unsigned modo,std::uint64_t quadro) noexcept{
 nrorc_vram::pausar_calib();
 (void)command;(void)input;(void)estado_entrada;(void)output;(void)estado_saida;(void)modo;(void)quadro;return false;
}
void despejo_final_tick() noexcept{

}
}
#include "late_compose.inl"
#include "rosto_jogador_d3d.inl"
#include "cena_npc.inl"
#include "mascara.inl"
