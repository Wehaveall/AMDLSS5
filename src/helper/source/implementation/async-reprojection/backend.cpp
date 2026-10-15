#include "../../../runtime/nr_publico.h"
#include "../../../runtime/custo_perfil.h"
#include "backend.h"
#include "../../../runtime/rosto_jogador_state.h"
#include "policy.h"
#include "late_compose.h"
#include "formatos.h"
#include "reproject_controls.h"
#include "../compose-controls/realce.h"
#include "../compat-diag/compat.h"
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
#include "nr_reproject_composepostcr.h"
#include "nr_reproject_composepostg.h"
#include "nr_reproject_composepostcrg.h"
#include "nr_reproject_estabilizarcr.h"
#include "nr_reproject_croma.h"
#include "nr_reproject_cromalds.h"
#include "nr_reproject_suave.h"
#include "nr_reproject_luzgrossa.h"
#include "suave_config.h"
#include "nr_reproject_fill.h"
#include "nr_reproject_bilaterallumafonte.h"
#include "nr_reproject_bilaterallumaatual.h"
#include "nr_reproject_bilateralprep.h"
#include "nr_reproject_bilateralfit.h"
#include "nr_reproject_bilateralapply.h"
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

struct Thresholds { float depth_relative, depth_absolute, color, clamp_strength, motion_reference, confidence_rate, motion_divergence, motion_factor, deform_tolerance, gate_smooth, deform_blur, detail_motion, gate_coherent, occlusion_tolerance, residual_floor, confidence_floor, dual_motion, dual_threshold; };
const Thresholds& thresholds() {
    static const Thresholds value = []{

        auto read=[](const wchar_t* key,float fallback,bool negativo_vale=false){
            double parsed{};
            if(!nrinijogo::ler_double(key,parsed)||(!negativo_vale&&parsed<0))return fallback;
            return float(parsed);
        };
        return Thresholds{read(L"NeuralReprojectDepthRelative",.03f),
                          read(L"NeuralReprojectDepthAbsolute",.0001f),
                          read(L"NeuralReprojectColorThreshold",.15f),

                          read(L"NeuralReprojectClampStrength",0.f),

                          read(L"NeuralReprojectMotionReference",.01f),

                          read(L"NeuralReprojectConfidenceRate",0.f),

                          read(L"NeuralReprojectMotionDivergence",0.f),

                          read(L"NeuralReprojectMotionFactor",1.f,true),

                          read(L"NeuralReprojectDeformTolerance",0.f),

                          read(L"NeuralReprojectGateSmooth",0.f),
                          read(L"NeuralReprojectDeformBlur",0.f),
                          read(L"NeuralReprojectDetailMotion",0.f),
                          read(L"NeuralReprojectGateCoherent",0.f),

                          read(L"NeuralReprojectOcclusionTolerance",0.f),

                          read(L"NeuralReprojectResidualFloor",0.f),

                          read(L"NeuralReprojectConfidenceFloor",0.f),

                          read(L"NeuralReprojectDualMotion",0.f),

                          read(L"NeuralReprojectDualMotionThreshold",0.f)};
    }();

    static const bool seeded=[&]{
        auto ini=[](const wchar_t* key,float fallback){double v{};return nrinijogo::ler_double(key,v)?float(v):fallback;};
        const float initial[nrreproject::ControlCount]={value.clamp_strength,value.motion_divergence,
                                                        value.depth_relative,value.color,
                                                        value.confidence_rate,value.motion_factor,
                                                        value.deform_tolerance,value.detail_motion,
                                                        value.occlusion_tolerance,
                                                        ini(L"NeuralReprojectBilateral",0.f),ini(L"NeuralReprojectBilateralDetail",.01f),
                                                        ini(L"NeuralReprojectBilateralGuide",0.f),ini(L"NeuralReprojectBilateralHold",0.f),
                                                        ini(L"NeuralReprojectBilateralRidge",.001f),ini(L"NeuralReprojectPointMotion",0.f),
                                                        value.dual_motion,value.dual_threshold,value.residual_floor,
                                                        value.confidence_floor,value.gate_smooth,value.deform_blur,
                                                        value.motion_reference,value.depth_absolute,
                                                        ini(L"NeuralAsyncMaxAge",0.f),
                                                        ini(L"NeuralReprojectComposeAfterUpscale",0.f),
                                                        ini(L"NeuralReprojectFillMode",0.f),
                                                        ini(L"NeuralReprojectDepthSnap",0.f),
                                                        ini(L"NeuralReprojectMotionRepair",0.f),
                                                        ini(L"NeuralReprojectRawDivergence",0.f),
                                                        ini(L"NeuralReprojectStabilize",0.f),
                                                        ini(L"NeuralGradeExposure",0.f),ini(L"NeuralGradeContrast",0.f),ini(L"NeuralGradeSaturation",0.f),
                                                        ini(L"NeuralReprojectMotionStrength",.5f),ini(L"NeuralReprojectMotionStrengthRef",4.f),
                                                        ini(L"NeuralReprojectJitterComp",0.f),ini(L"NeuralReprojectStabilizeStill",2.f),ini(L"NeuralReprojectNoiseCancel",0.f),ini(L"NeuralReprojectRatioTransfer",0.f),
                                                        ini(L"NeuralGainLimit",0.f),ini(L"NeuralHighlightGuardSoft",0.f),
                                                        [&]{const float v=ini(L"NeuralHighlightGuardLight",-1.f);return v<0.f?0.f:nrrealce::luz_legada(v)?nrreproject::guarda_sem_limite:v;}(),ini(L"NeuralHighlightGuardDetail",1.2f),ini(L"NeuralLightSmooth",0.f),
                                                        ini(L"NeuralHaloReduce",0.f),ini(L"NeuralReprojectDepthAwareRead",0.f),ini(L"NeuralRatioFloor",1.f/512.f),
                                                        ini(L"NeuralLateCompose",0.f),ini(L"NeuralHaloNetScale",0.f),ini(L"NeuralLightGuideDepth",0.f),
                                                        ini(L"NeuralHaloExcess",0.f),ini(L"NeuralReprojectEdgeCarry",0.f),
                                                        ini(L"NeuralHaloCutMoving",0.f)};
        nrreproject::seed(nrreproject::controls,initial);return true;
    }();
    (void)seeded;
    return value;
}

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
D3D12_RESOURCE_STATES state(unsigned s){switch(s){
 case 1:return D3D12_RESOURCE_STATE_COMMON;case 2:return write_state;case 4:return read_state;
 case 8:return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;case 12:return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|read_state;
 case 16:return D3D12_RESOURCE_STATE_COPY_SOURCE;case 32:return D3D12_RESOURCE_STATE_COPY_DEST;
 case 20:return D3D12_RESOURCE_STATE_COPY_SOURCE|read_state;default:return D3D12_RESOURCE_STATES(~0u);}}
void transition(ID3D12GraphicsCommandList* c,ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){if(a==b)return;D3D12_RESOURCE_BARRIER v{};v.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;v.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,a,b};c->ResourceBarrier(1,&v);}
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
 if(FAILED(d->CreateCommittedResource(&hl,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&l)))){falhou=true;return false;}
 bd.Width=UINT64(nrexpofresca::slots)*256u;
 if(FAILED(d->CreateCommittedResource(&he,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&u)))){falhou=true;return false;}
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
bool texture(ID3D12Resource* r,DXGI_FORMAT f){if(!r)return false;auto d=r->GetDesc();return d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Format==f&&d.Width&&d.Height&&d.Width<=4096&&d.Height<=4096&&d.DepthOrArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1;}

using nrformato::vista_cor;using nrformato::bits_cor;using nrformato::vista_profundidade;using nrformato::vista_movimento;using nrformato::entrada;

bool format_support(ID3D12Device* d,DXGI_FORMAT f,bool uav){
 struct Resposta{ID3D12Device* d;DXGI_FORMAT f;bool uav,ok;};static Resposta cache[8]{};static unsigned n{},proximo{};static std::mutex m;
 {std::lock_guard<std::mutex> l(m);for(unsigned i=0;i<n;++i)if(cache[i].d==d&&cache[i].f==f&&cache[i].uav==uav)return cache[i].ok;}
 D3D12_FEATURE_DATA_FORMAT_SUPPORT q{f};if(FAILED(d->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&q,sizeof(q))))return false;
 const bool ok=(q.Support1&D3D12_FORMAT_SUPPORT1_SHADER_LOAD)&&(!uav||((q.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD)&&(q.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE)));
 std::lock_guard<std::mutex> l(m);cache[proximo]={d,f,uav,ok};proximo=(proximo+1)%8u;if(n<8)++n;return ok;}
bool root(ID3D12Device* d,unsigned constants,unsigned srvs,unsigned uavs,ComPtr<ID3D12RootSignature>& result){
 D3D12_DESCRIPTOR_RANGE ranges[2]{};ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,srvs,0,0,0};ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,uavs,0,0,0};
 D3D12_ROOT_PARAMETER p[3]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[0].Constants={0,0,constants};
 for(unsigned i=0;i<2;++i){p[i+1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[i+1].DescriptorTable={1,&ranges[i]};}
 D3D12_STATIC_SAMPLER_DESC s{};s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxLOD=D3D12_FLOAT32_MAX;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
 D3D12_ROOT_SIGNATURE_DESC rd{3,p,1,&s,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob>b,e;
 const bool ok=SUCCEEDED(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&e))&&SUCCEEDED(d->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&result)));
 if(ok)nrorc_vram::registrar(nrorc::Api::Raiz,0,"raiz");
 return ok;
}
template<class T,std::size_t N>bool pipeline(ID3D12Device* d,ID3D12RootSignature* r,const T(&bytes)[N],ComPtr<ID3D12PipelineState>& p){D3D12_COMPUTE_PIPELINE_STATE_DESC q{};q.pRootSignature=r;q.CS={bytes,sizeof(bytes)};const bool ok=SUCCEEDED(d->CreateComputePipelineState(&q,IID_PPV_ARGS(&p)));if(ok)nrorc_vram::registrar(nrorc::Api::Pso,0,"pso");return ok;}

void registrar_realce(const char* modo,unsigned antes,unsigned depois,float luz,float detalhe){
(void)modo;(void)antes;(void)depois;(void)luz;(void)detalhe;
}

unsigned teto_unico(unsigned tone){
    const float luz=nrreproject::value(nrreproject::controls,nrreproject::LightGuard),det=nrreproject::value(nrreproject::controls,nrreproject::DetailGuard);
    const unsigned t=nrrealce::tom(tone,luz,det);registrar_realce("post_teto_unico",tone,t,luz,det);return t;
}

bool ler_ini(const wchar_t* chave,double& valor){return nrinijogo::ler_double(chave,valor);}

bool q_chave(const wchar_t* k){double v=0.;return ler_ini(k,v)&&v>.5;}

float q_ganho(){double v=1.;if(!ler_ini(L"NeuralQGanho",v)||!(v>=.25&&v<=4.))v=1.;return float(v);}

constexpr unsigned despejo_maximo=120;
unsigned despejos_reservados{};
struct Despejo { unsigned restantes{},espera{},intervalo{},contador{};float ganho{16.f};bool profundidade{},etapas_fill{}; };
Despejo ler_despejo(){
    Despejo d;double v{};

    if(ler_ini(L"NeuralDumpFrames",v)&&v>=1&&v<=double(despejo_maximo))d.restantes=unsigned(v);

    if(ler_ini(L"NeuralDumpGain",v)&&v>=.1&&v<=1000.)d.ganho=float(v);

    if(ler_ini(L"NeuralDumpAfterFrames",v)&&v>=0&&v<=1e6)d.espera=unsigned(v);

    if(ler_ini(L"NeuralDumpEveryFrames",v)&&v>=0&&v<=1e6)d.intervalo=unsigned(v);
    d.profundidade=ler_ini(L"NeuralDumpDepth",v)&&v==1.;
    d.etapas_fill=ler_ini(L"NeuralDumpFillStages",v)&&v==1.;
    return d;
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

struct Bilateral { float forca{},tau{.01f},ridge{1e-3f};unsigned celula{16},retencao{},guia{}; };
Bilateral ler_bilateral(){
    Bilateral b;double v{};
    if(ler_ini(L"NeuralReprojectBilateral",v)&&v>0&&v<=1.)b.forca=float(v);
    if(ler_ini(L"NeuralReprojectBilateralDetail",v)&&v>=0&&v<=1.)b.tau=float(v);
    if(ler_ini(L"NeuralReprojectBilateralRidge",v)&&v>0&&v<=1.)b.ridge=float(v);

    if(ler_ini(L"NeuralReprojectBilateralCell",v)&&v>=4&&v<=16)b.celula=unsigned(v);

    if(ler_ini(L"NeuralReprojectBilateralHold",v)&&v>=0&&v<=240)b.retencao=unsigned(v);

    if(ler_ini(L"NeuralReprojectBilateralGuide",v)&&(v==0||v==1))b.guia=unsigned(v);
    return b;
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

 ComPtr<ID3D12Resource> fill_copia;

 ComPtr<ID3D12RootSignature> est_root;ComPtr<ID3D12PipelineState> est_pso;ComPtr<ID3D12DescriptorHeap> est_heap;
 ComPtr<ID3D12Resource> est[2];D3D12_RESOURCE_STATES est_estado[2]{};unsigned est_cur{};bool est_valido{};

 ComPtr<ID3D12Resource> ganho[2];D3D12_RESOURCE_STATES ganho_estado[2]{};unsigned ganho_cur{};bool ganho_ok{},ganho_valido{};std::uint64_t ganho_quadro{},ganho_limitados{};

 bool ganho_dividido{};

 ComPtr<ID3D12RootSignature> luz_root;ComPtr<ID3D12PipelineState> luz_mom_pso,luz_coef_pso,luz_temp_pso;ComPtr<ID3D12DescriptorHeap> luz_heap;

 ComPtr<ID3D12PipelineState> luz_prox_pso,luz_halo_pso;ComPtr<ID3D12Resource> luz_prox,luz_halo;
 unsigned halo_teto_raio{},halo_teto_quadros{};
 unsigned halo_exc_r100{ler_raio_ini(L"NeuralHaloExcessRadius",12u,4u,24u)},borda_r100{ler_raio_ini(L"NeuralReprojectEdgeCarryRadius",3u,2u,16u)};
 unsigned halo_exc_raio{},borda_raio{};std::uint64_t halo_exc_quadros{},borda_quadros{};

 unsigned corte_raio_warp{},corte_raio_comp{};std::uint64_t corte_quadros{};float idade_media{};
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

 Bilateral bilateral{ler_bilateral()};
 ComPtr<ID3D12RootSignature> bil_root;ComPtr<ID3D12PipelineState> bil_luma_fonte,bil_luma_atual,bil_prep,bil_fit,bil_apply;ComPtr<ID3D12DescriptorHeap> bil_heap;
 ComPtr<ID3D12Resource> bil_features,bil_luma;std::array<ComPtr<ID3D12Resource>,5> bil_coef;DXGI_FORMAT bil_coef_format{DXGI_FORMAT_R32G32B32A32_FLOAT};

 ComPtr<ID3D12DescriptorHeap> bil_heap_tardia;ID3D12DescriptorHeap* bil_heap_override{};int bil_duplo_prev{-1},bil_duplo_late{-1};std::uint64_t bil_duplo_quadro{};
 std::uint64_t retidos{},preservacoes{};bool bil_falhou{};int bil_preservado_job{-1};bool bil_preservado_read{};Microsoft::WRL::ComPtr<ID3D12Resource> bil_preservado,bil_preservado_prof;Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> bil_heap_preservado;unsigned bil_nx{},bil_ny{};int bil_job{-1};bool bil_features_read{},bil_luma_read{},bil_coef_read{};std::uint64_t bil_fits{};
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
 bool lg_ligado()const{return lg_alfa<1.f;}
 ComPtr<ID3D12RootSignature> lg_root;ComPtr<ID3D12PipelineState> lg_pso;ComPtr<ID3D12DescriptorHeap> lg_heap;
 ComPtr<ID3D12Resource> lg[2],lg_out;D3D12_RESOURCE_STATES lg_estado[2]{},lg_out_estado{};unsigned lg_cur{};bool lg_valido{};std::uint64_t lg_quadro{},lg_quadros{};

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
 bool fg_cross=[]{double v=0.;return ler_ini(L"NeuralFgCrossfade",v)&&v>.5;}();
 bool fg_cross_sempre=[]{double v=0.;return ler_ini(L"NeuralFgCrossfadeSempre",v)&&v>.5;}();
 float est_cross{},cross_P{};int cross_job{-1};unsigned cross_j{};
 float post_alfa_mov=[]{double v=.5;if(!ler_ini(L"NeuralPostStabilizeMotion",v)||!(v>0.&&v<=1.))v=.5;return float(v);}();
 Constants constants{};std::uint64_t composed{},skipped{},unsupported{};

 LateState late;ID3D12DescriptorHeap* warp_heap_override{};bool suprimir_despejo{};

 ID3D12DescriptorHeap* reduzir_heap_override{};bool late_post=[]{double v=0.;return ler_ini(L"NeuralLateComposePost",v)&&v>.5;}();

 bool red_cache_lig=[]{double v=0.;return ler_ini(L"NeuralCustoReduzirCache",v)&&v>.5;}();
 bool red_cache_valido{};const void* red_cache_res{};const void* red_cache_dst{};int red_cache_job{-1};std::uint64_t red_cache_hits{},red_cache_misses{};

 ComPtr<ID3D12PipelineState> post_pso;ComPtr<ID3D12DescriptorHeap> post_heap;

 struct QMapaCfg{bool ligado{};unsigned bits{};float gHi{1},gHiNeg{1},gLo{1},gLoNeg{1},movA{0},movB{0},gradA{0},gradB{0},profR{0},profA{.05f},profB{.15f},confP{1},edgeA{.05f},edgeB{.25f},movR{0},stopsMax{0};};
 QMapaCfg q_mapa=[]{QMapaCfg m;m.ligado=q_chave(L"NeuralQMapa");if(!m.ligado)return m;
  auto f=[](const wchar_t* k,float& x,double lo,double hi){double v=x;if(ler_ini(k,v)&&v>=lo&&v<=hi)x=float(v);};
  float b=0;f(L"NeuralQMapaBits",b,0.,4095.);m.bits=unsigned(b+.5f);
  {float fr=0;f(L"NeuralQMapaForaRosto",fr,0.,1.);if(fr>.5f)m.bits|=4096u;}

  {f(L"NeuralQMapaMovViz",m.movR,0.,32.);if(m.movR>0.f)m.bits|=8192u;float ns=0;f(L"NeuralQMapaNegStops",ns,0.,1.);if(ns>.5f)m.bits|=16384u;f(L"NeuralQMapaNegStopsMax",m.stopsMax,0.,4.);}
  f(L"NeuralQMapaHi",m.gHi,.25,4.);m.gHiNeg=m.gHi;f(L"NeuralQMapaHiNeg",m.gHiNeg,.0,4.);f(L"NeuralQMapaLo",m.gLo,.25,4.);m.gLoNeg=m.gLo;f(L"NeuralQMapaLoNeg",m.gLoNeg,.0,4.);
  f(L"NeuralQMapaMovA",m.movA,0.,100.);f(L"NeuralQMapaMovB",m.movB,0.,100.);f(L"NeuralQMapaGradA",m.gradA,0.,100.);f(L"NeuralQMapaGradB",m.gradB,0.,100.);
  f(L"NeuralQMapaProfR",m.profR,0.,32.);f(L"NeuralQMapaProfA",m.profA,0.,10.);f(L"NeuralQMapaProfB",m.profB,0.,10.);f(L"NeuralQMapaConfP",m.confP,0.,16.);f(L"NeuralQMapaBordaA",m.edgeA,0.,10.);f(L"NeuralQMapaBordaB",m.edgeB,0.,10.);if(!(m.edgeB>m.edgeA))m.edgeB=m.edgeA+1e-3f;
  if(!(m.movB>m.movA))m.movB=m.movA+1e-3f;if(!(m.gradB>m.gradA))m.gradB=m.gradA+1e-3f;if(!(m.profB>m.profA))m.profB=m.profA+1e-3f;
  return m;}();
 float q_post_ganho=q_mapa.ligado?1.f:q_ganho();
 struct QPost{ComPtr<ID3D12RootSignature> root;ComPtr<ID3D12PipelineState> pso;ComPtr<ID3D12DescriptorHeap> heap,post_heap;ComPtr<ID3D12Resource> saida;
  std::uint64_t quadros{},pulados_passos{};bool logado{},fora_passos{};void soltar(){root.Reset();pso.Reset();heap.Reset();post_heap.Reset();saida.Reset();}} qpost;
 bool alocar_q_post(const D3D12_UNORDERED_ACCESS_VIEW_DESC& flags_view);
 bool q_post(ID3D12GraphicsCommandList* c,float exposure,unsigned tone);

 float q_rosto_ganho=[]{double v=1.;if(!ler_ini(L"NeuralQGanhoRosto",v)||!(v>=.25&&v<=4.))v=1.;return float(v);}();
 struct Pendente{bool ativo{};void* command{};int job{};float exposure{1};unsigned tone{};} post;
 bool post_recusado{},post_formato_logado{};std::uint64_t post_composed{},post_refused{},post_lost{};

 struct HaloPost{ComPtr<ID3D12RootSignature> root;ComPtr<ID3D12PipelineState> mom_pso,momp_pso,ref_pso,apl_pso;ComPtr<ID3D12DescriptorHeap> heap,post_heap;
  ComPtr<ID3D12Resource> mom,ref,saida;unsigned qw{},qh{};std::uint64_t quadros{};bool logado{};
  void soltar(){root.Reset();mom_pso.Reset();momp_pso.Reset();ref_pso.Reset();apl_pso.Reset();heap.Reset();post_heap.Reset();mom.Reset();ref.Reset();saida.Reset();qw=qh=0;}} hpost;
 float halo_post_forca=[]{double v=0;return ler_ini(L"NeuralHaloTetoPost",v)&&v>0.?float(std::min(v,1.)):0.f;}();
 double halo_post_raio=[]{double v=16;return ler_ini(L"NeuralHaloTetoPostRaio",v)&&v>=2.&&v<=30.?v:16.;}();

 float halo_post_salto=[]{double v=0;return ler_ini(L"NeuralHaloTetoPostSalto",v)&&v>0.&&v<=.5?float(v):0.f;}();
 bool alocar_halo_post(const D3D12_UNORDERED_ACCESS_VIEW_DESC& flags_view);
 bool halo_post(ID3D12GraphicsCommandList* c,float exposure,unsigned tone);

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

 unsigned trace_restantes=[]{double v=0;return ler_ini(L"NeuralCenaTrace",v)&&v>=1.&&v<=200000.?unsigned(v):0u;}();
 float trace_expo{};long long trace_qpc{};
 void trace(int job,int age,const char* reason){
(void)job;(void)age;(void)reason;
}

 void perder_post(const char* motivo){if(!post.ativo)return;post.ativo=false;++post_lost;if(post_lost<=3||(post_lost&255)==0)log_post(post.job,motivo,nullptr);}
 bool modo_post()const{return !reduzido&&!post_recusado&&post_pso&&post_heap&&nrreproject::value(nrreproject::controls,nrreproject::ComposeAfter)>.5f;}
 D3D12_CPU_DESCRIPTOR_HANDLE cpu(ID3D12DescriptorHeap* h,unsigned i){auto v=h->GetCPUDescriptorHandleForHeapStart();v.ptr+=SIZE_T(i)*stride;return v;}
 D3D12_GPU_DESCRIPTOR_HANDLE gpu(ID3D12DescriptorHeap* h,unsigned i){auto v=h->GetGPUDescriptorHandleForHeapStart();v.ptr+=UINT64(i)*stride;return v;}
 void srv(ID3D12DescriptorHeap* h,unsigned i,ID3D12Resource* r,DXGI_FORMAT f,bool array=false){D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=f;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;if(array){v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;v.Texture2DArray.MipLevels=1;v.Texture2DArray.ArraySize=Policy::history_count;}else{v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Texture2D.MipLevels=1;}device->CreateShaderResourceView(r,&v,cpu(h,i));}
 void uav(ID3D12DescriptorHeap* h,unsigned i,ID3D12Resource* r,DXGI_FORMAT f,bool array=false){D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=f;if(array){v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2DARRAY;v.Texture2DArray.ArraySize=Policy::history_count;}else v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(r,nullptr,&v,cpu(h,i));}

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

 void retire(unsigned to_w,unsigned to_h){
  policy.invalidate();
  log_realocacao(to_w,to_h);

  Aposentadas lote;lote.vencimento=begins+3;
  auto drop=[&](auto& com){if(com){lote.objetos.emplace_back(com.Get());com.Reset();}};
  for(auto& h:history)drop(h);drop(warped);drop(flags_buffer);
  drop(temporal_heap);drop(compose_heap);drop(temporal_root);drop(compose_root);drop(post_root);
  drop(ganho[0]);drop(ganho[1]);ganho_ok=ganho_valido=false;ganho_cur=0;
  drop(luz_root);drop(luz_mom_pso);drop(luz_coef_pso);drop(luz_temp_pso);drop(luz_heap);
  drop(luz_mom);drop(luz_base);drop(luz_ab);drop(luz_coef);drop(luz_hist[0]);drop(luz_hist[1]);luz_ok=luz_valido=false;luz_cur=0;
  drop(luz_prox_pso);drop(luz_halo_pso);drop(luz_prox);drop(luz_halo);
  drop(capture_pso);drop(warp_pso);drop(compose_pso);drop(post_pso);drop(post_heap);perder_post("post_lost_realocacao");
  drop(hpost.root);drop(hpost.mom_pso);drop(hpost.momp_pso);drop(hpost.ref_pso);drop(hpost.apl_pso);drop(hpost.heap);drop(hpost.post_heap);drop(hpost.mom);drop(hpost.ref);drop(hpost.saida);hpost.qw=hpost.qh=0;
  drop(qpost.root);drop(qpost.pso);drop(qpost.heap);drop(qpost.post_heap);drop(qpost.saida);
  drop(cnpc.root);drop(cnpc.mede);drop(cnpc.compor);drop(cnpc.heap);drop(cnpc.estado);drop(cnpc.leitura);drop(cnpc.zeros);cnpc.zerado=false;
  drop(fill_heap);drop(fill_root);drop(fill_pso);drop(fill_copia);
  drop(res_reduzido);drop(reduzir_pso);drop(reduzir_heap);res_reduzido_read=false;reduzido=false;comp_w=comp_h=0;
  drop(est_heap);drop(est_root);drop(est_pso);drop(est[0]);drop(est[1]);est_valido=false;
  drop(cro_heap);drop(cro_root);drop(cro_pso);drop(cro[0]);drop(cro[1]);drop(cro_out);cro_valido=false;
  drop(lg_heap);drop(lg_root);drop(lg_pso);drop(lg[0]);drop(lg[1]);drop(lg_out);lg_valido=false;
  drop(su_heap);drop(su_root);drop(su_pso);drop(su[0]);drop(su[1]);drop(su_out);su_valido=false;
  drop(bil_heap);drop(bil_root);drop(bil_luma_fonte);drop(bil_luma_atual);drop(bil_prep);drop(bil_fit);drop(bil_apply);drop(bil_features);drop(bil_luma);
  for(auto& t:bil_coef)drop(t);
  drop(bil_preservado);drop(bil_preservado_prof);drop(bil_heap_preservado);drop(bil_heap_tardia);bil_duplo_late=-1;
  bil_job=-1;bil_features_read=bil_luma_read=bil_coef_read=false;bil_preservado_job=-1;bil_preservado_read=false;

  histories_read=false;warped_read=false;
  width=height=0;ready=false;

  if(!lote.objetos.empty())retired.push_back(std::move(lote));
 }

 static constexpr std::uint64_t orcamento_bytes=2560ull*1024*1024;

 static constexpr std::uint64_t por_pixel=std::uint64_t(Policy::history_count)*16+8;

 static constexpr std::uint64_t orcamento_piso=1024ull*1024*1024,orcamento_reserva=1024ull*1024*1024;
 std::uint64_t orcamento_atual{orcamento_bytes};Extensao orcamento_para{};

 ComPtr<IDXGIAdapter3> adaptador;bool adaptador_tentado{};unsigned long long vram_tick{};
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
 bool allocation(unsigned w,unsigned h,unsigned cw,unsigned ch){
  if(ready&&width==w&&height==h&&comp_w==cw&&comp_h==ch)return true;
  nrorc_vram::registrar(nrorc::Api::RecriaCena,std::uint64_t(w)*h*por_pixel,"backend_allocation");

  const std::uint64_t custo=std::uint64_t(w)*h*por_pixel;

  recusa={w,h,width,height,custo};

  if(custo>orcamento_atual){++recusas_por_limite;return false;}
  if(width||height)retire(w,h);
  width=w;height=h;comp_w=cw;comp_h=ch;reduzido=(w!=cw||h!=ch);stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  if(!root(device.Get(),32,8,4,temporal_root)||!root(device.Get(),16,6,3,compose_root)||!root(device.Get(),12,2,2,post_root)||
     !pipeline(device.Get(),temporal_root.Get(),g_nr_reproject_capture,capture_pso)||!pipeline(device.Get(),temporal_root.Get(),g_nr_reproject_warp,warp_pso)||!pipeline(device.Get(),compose_root.Get(),g_nr_reproject_compose,compose_pso))return false;
  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,12,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&temporal_heap))))return false;hd.NumDescriptors=2*compose_variante;if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&compose_heap))))return false;hd.NumDescriptors=4;
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=w;td.Height=h;td.DepthOrArraySize=Policy::history_count;td.MipLevels=1;td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  const DXGI_FORMAT formats[]{DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R16G16_FLOAT,DXGI_FORMAT_R32_FLOAT};
  for(unsigned i=0;i<3;++i){td.Format=formats[i];if(!format_support(device.Get(),td.Format,true)||FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,write_state,nullptr,IID_PPV_ARGS(&history[i]))))return false;srv(temporal_heap.Get(),3+i,history[i].Get(),td.Format,true);uav(temporal_heap.Get(),8+i,history[i].Get(),td.Format,true);}
  td.DepthOrArraySize=1;td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,write_state,nullptr,IID_PPV_ARGS(&warped))))return false;
  uav(temporal_heap.Get(),11,warped.Get(),td.Format);for(unsigned v=0;v<2;++v){srv(compose_heap.Get(),compose_variante*v,warped.Get(),td.Format);srv(compose_heap.Get(),compose_variante*v+1,warped.Get(),td.Format);}
  D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=4;bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;bd.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,write_state,nullptr,IID_PPV_ARGS(&flags_buffer))))return false;
  D3D12_UNORDERED_ACCESS_VIEW_DESC bv{};bv.Format=DXGI_FORMAT_R32_TYPELESS;bv.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;bv.Buffer.NumElements=1;bv.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;for(unsigned v=0;v<2;++v)device->CreateUnorderedAccessView(flags_buffer.Get(),nullptr,&bv,cpu(compose_heap.Get(),compose_variante*v+compose_uavs+1));

  ganho_ok=alocar_ganho(!reduzido);ganho_valido=false;ganho_cur=0;

  luz_ok=!reduzido&&alocar_luz(bv);luz_valido=false;luz_cur=0;
  if(!luz_ok){luz_root.Reset();luz_mom_pso.Reset();luz_coef_pso.Reset();luz_temp_pso.Reset();luz_heap.Reset();luz_mom.Reset();luz_base.Reset();luz_ab.Reset();luz_coef.Reset();luz_hist[0].Reset();luz_hist[1].Reset();
   luz_prox_pso.Reset();luz_halo_pso.Reset();luz_prox.Reset();luz_halo.Reset();
   for(unsigned v=0;v<2;++v){srv(compose_heap.Get(),compose_variante*v+5,nullptr,DXGI_FORMAT_R32G32B32A32_FLOAT);srv(compose_heap.Get(),compose_variante*v+1,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);}}

  if(!(q_post_ganho!=1.f?(q_chave(L"NeuralQPostCR")?pipeline(device.Get(),post_root.Get(),g_nr_reproject_composepostcrg,post_pso):pipeline(device.Get(),post_root.Get(),g_nr_reproject_composepostg,post_pso)):(q_chave(L"NeuralQPostCR")?pipeline(device.Get(),post_root.Get(),g_nr_reproject_composepostcr,post_pso):pipeline(device.Get(),post_root.Get(),g_nr_reproject_composepost,post_pso)))||FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&post_heap)))){post_pso.Reset();post_heap.Reset();}
  else{srv(post_heap.Get(),0,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);srv(post_heap.Get(),1,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);device->CreateUnorderedAccessView(flags_buffer.Get(),nullptr,&bv,cpu(post_heap.Get(),3));}

  if(halo_post_forca>0.f&&post_pso&&post_heap&&!alocar_halo_post(bv))hpost.soltar();
  if(q_mapa.ligado&&post_pso&&post_heap&&!alocar_q_post(bv))qpost.soltar();
  if(cnpc_cfg.modo&&reduzido&&post_pso&&post_heap&&!alocar_cena_npc(bv))cnpc.soltar();
  srv(temporal_heap.Get(),6,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);srv(temporal_heap.Get(),7,nullptr,DXGI_FORMAT_R32_FLOAT);

  if(preenchimento>0&&!alocar_preenchimento()){preenchimento=0.f;fill_heap.Reset();fill_root.Reset();fill_pso.Reset();fill_copia.Reset();}

  if(!alocar_estabilizador()){est_pso.Reset();est_heap.Reset();est_root.Reset();est[0].Reset();est[1].Reset();}

  if(cro_ligado()&&!alocar_croma()){cro_pso.Reset();cro_heap.Reset();cro_root.Reset();cro[0].Reset();cro[1].Reset();cro_out.Reset();}
  if(lg_ligado()&&!alocar_luz_grossa()){lg_pso.Reset();lg_heap.Reset();lg_root.Reset();lg[0].Reset();lg[1].Reset();lg_out.Reset();}

  if(su_cfg.ligado()&&!alocar_suave()){su_pso.Reset();su_heap.Reset();su_root.Reset();su[0].Reset();su[1].Reset();su_out.Reset();}

  if(reduzido&&!alocar_reduzir())return false;
  if(!reduzido&&!alocar_bilateral()){bil_falhou=true;bilateral.forca=0.f;bil_preservado.Reset();bil_preservado_prof.Reset();bil_heap_preservado.Reset();bil_heap.Reset();bil_heap_tardia.Reset();bil_root.Reset();bil_luma_fonte.Reset();bil_luma_atual.Reset();bil_prep.Reset();bil_fit.Reset();bil_apply.Reset();bil_features.Reset();bil_luma.Reset();for(auto& t:bil_coef)t.Reset();}
  ready=true;return true;
 }
 bool alocar_reduzir(){
  if(!post_pso||!post_heap)return false;
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=width;td.Height=height;td.DepthOrArraySize=1;td.MipLevels=1;td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,write_state,nullptr,IID_PPV_ARGS(&res_reduzido))))return false;
  res_reduzido_read=false;
  if(!pipeline(device.Get(),temporal_root.Get(),g_nr_reproject_reduzir,reduzir_pso))return false;
  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,12,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&reduzir_heap))))return false;

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
  for(unsigned i=0;criar&&i<2;++i){if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read_state,nullptr,IID_PPV_ARGS(&ganho[i])))){ok=false;ganho[0].Reset();ganho[1].Reset();break;}ganho_estado[i]=read_state;}
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
  auto criar=[&](DXGI_FORMAT f,ComPtr<ID3D12Resource>& r){td.Format=f;return SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read_state,nullptr,IID_PPV_ARGS(&r)));};
  if(!criar(DXGI_FORMAT_R32G32B32A32_FLOAT,luz_mom)||!criar(DXGI_FORMAT_R32_FLOAT,luz_base)||!criar(DXGI_FORMAT_R32G32_FLOAT,luz_ab)||
     !criar(DXGI_FORMAT_R32G32B32A32_FLOAT,luz_coef)||!criar(DXGI_FORMAT_R32G32_FLOAT,luz_hist[0])||!criar(DXGI_FORMAT_R32G32_FLOAT,luz_hist[1])||
     !criar(DXGI_FORMAT_R16_FLOAT,luz_prox)||!criar(DXGI_FORMAT_R32G32B32A32_FLOAT,luz_halo))return false;
  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,2*luz_variante,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&luz_heap))))return false;
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
 bool alocar_bilateral(){
  if(!root(device.Get(),15,11,8,bil_root)||
     !pipeline(device.Get(),bil_root.Get(),g_nr_reproject_bilaterallumafonte,bil_luma_fonte)||!pipeline(device.Get(),bil_root.Get(),g_nr_reproject_bilaterallumaatual,bil_luma_atual)||
     !pipeline(device.Get(),bil_root.Get(),g_nr_reproject_bilateralprep,bil_prep)||!pipeline(device.Get(),bil_root.Get(),g_nr_reproject_bilateralfit,bil_fit)||
     !pipeline(device.Get(),bil_root.Get(),g_nr_reproject_bilateralapply,bil_apply))return false;
  const unsigned cel=bilateral.celula;bil_nx=(width+cel-1)/cel+1;bil_ny=(height+cel-1)/cel+1;

  {D3D12_FEATURE_DATA_FORMAT_SUPPORT fs{DXGI_FORMAT_R32G32B32A32_FLOAT};
   const bool f32=SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&fs,sizeof(fs)))&&(fs.Support1&D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE)&&(fs.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
   bil_coef_format=f32?DXGI_FORMAT_R32G32B32A32_FLOAT:DXGI_FORMAT_R16G16B16A16_FLOAT;}
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=width;td.Height=height;td.DepthOrArraySize=1;td.MipLevels=1;td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,write_state,nullptr,IID_PPV_ARGS(&bil_features))))return false;
  td.Format=DXGI_FORMAT_R32_FLOAT;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,write_state,nullptr,IID_PPV_ARGS(&bil_luma))))return false;
  D3D12_RESOURCE_DESC cd{};cd.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE3D;cd.Width=bil_nx;cd.Height=bil_ny;cd.DepthOrArraySize=16;cd.MipLevels=1;cd.Format=bil_coef_format;cd.SampleDesc.Count=1;cd.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  for(auto& t:bil_coef)if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&cd,write_state,nullptr,IID_PPV_ARGS(&t))))return false;

  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,19,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&bil_heap)))||FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&bil_heap_tardia))))return false;

  for(ID3D12DescriptorHeap* h:{bil_heap.Get(),bil_heap_tardia.Get()}){
   srv(h,10,history[2].Get(),DXGI_FORMAT_R32_FLOAT,true);
   srv(h,0,history[0].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,true);srv(h,1,history[1].Get(),DXGI_FORMAT_R16G16_FLOAT,true);
   srv(h,2,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);srv(h,3,bil_features.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);srv(h,4,bil_luma.Get(),DXGI_FORMAT_R32_FLOAT);
   for(unsigned o=0;o<5;++o){
    D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=bil_coef_format;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE3D;v.Texture3D.MipLevels=1;device->CreateShaderResourceView(bil_coef[o].Get(),&v,cpu(h,5+o));
    D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=bil_coef_format;u.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE3D;u.Texture3D.WSize=16;device->CreateUnorderedAccessView(bil_coef[o].Get(),nullptr,&u,cpu(h,12+o));
   }
   uav(h,11,bil_features.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);uav(h,17,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);uav(h,18,bil_luma.Get(),DXGI_FORMAT_R32_FLOAT);
  }

  {
   D3D12_RESOURCE_DESC pd{};pd.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;pd.Width=width;pd.Height=height;pd.DepthOrArraySize=1;pd.MipLevels=1;pd.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;pd.SampleDesc.Count=1;
   if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&pd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&bil_preservado))))return false;
   pd.Format=DXGI_FORMAT_R32_FLOAT;
   if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&pd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&bil_preservado_prof))))return false;
   if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&bil_heap_preservado))))return false;
   ID3D12DescriptorHeap* h=bil_heap_preservado.Get();
   {D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;v.Texture2DArray.MipLevels=1;v.Texture2DArray.ArraySize=1;device->CreateShaderResourceView(bil_preservado.Get(),&v,cpu(h,0));
    v.Format=DXGI_FORMAT_R32_FLOAT;device->CreateShaderResourceView(bil_preservado_prof.Get(),&v,cpu(h,10));}
   srv(h,1,history[1].Get(),DXGI_FORMAT_R16G16_FLOAT,true);srv(h,2,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);srv(h,3,bil_features.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);srv(h,4,bil_luma.Get(),DXGI_FORMAT_R32_FLOAT);
   for(unsigned o=0;o<5;++o){
    D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=bil_coef_format;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE3D;v.Texture3D.MipLevels=1;device->CreateShaderResourceView(bil_coef[o].Get(),&v,cpu(h,5+o));
    D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=bil_coef_format;u.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE3D;u.Texture3D.WSize=16;device->CreateUnorderedAccessView(bil_coef[o].Get(),nullptr,&u,cpu(h,12+o));
   }
   uav(h,11,bil_features.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);uav(h,17,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);uav(h,18,bil_luma.Get(),DXGI_FORMAT_R32_FLOAT);
  }
  bil_preservado_job=-1;bil_preservado_read=false;bil_duplo_late=-1;
  bil_job=-1;bil_features_read=bil_luma_read=bil_coef_read=false;return true;
 }

 void atualizar_ao_vivo(){
  (void)thresholds();
  if(!nrreproject::controls.menu.load(std::memory_order_acquire))return;
  using namespace nrreproject;
  const float tau=value(controls,BilateralDetail),ridge=value(controls,BilateralRidge);
  const unsigned guia=value(controls,BilateralGuide)>.5f?1u:0u;
  if(tau!=bilateral.tau||ridge!=bilateral.ridge||guia!=bilateral.guia){bil_job=-1;bil_duplo_late=-1;}
  bilateral.forca=bil_falhou?0.f:value(controls,BilateralStrength);
  bilateral.tau=tau;bilateral.ridge=ridge;bilateral.guia=guia;
  bilateral.retencao=unsigned(value(controls,BilateralHold)+.5f);
  pontual=value(controls,PointMotion)>.5f;
 }

 static float ajuste(nrreproject::Control c,float inicial){
  return nrreproject::controls.menu.load(std::memory_order_acquire)?nrreproject::value(nrreproject::controls,c):inicial;
 }
 void preservar_fonte(ID3D12GraphicsCommandList* c){
  if(!bil_preservado||!bil_apply||bilateral.forca<=0||bilateral.retencao==0)return;
  const unsigned fatia=unsigned((policy.frame+1)%Policy::history_count);const auto antigo=policy.frames[fatia];if(!antigo)return;
  int alvo=-1;for(const auto& j:policy.jobs)if(j.id>0&&j.frame==antigo&&j.id>bil_job&&j.id>alvo)alvo=j.id;
  if(alvo<0||alvo==bil_preservado_job)return;
  const auto antes=histories_read?read_state:write_state;
  transition(c,history[0].Get(),antes,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(c,history[2].Get(),antes,D3D12_RESOURCE_STATE_COPY_SOURCE);
  if(bil_preservado_read){transition(c,bil_preservado.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);transition(c,bil_preservado_prof.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);}
  D3D12_TEXTURE_COPY_LOCATION destino{},origem{};destino.pResource=bil_preservado.Get();destino.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;destino.SubresourceIndex=0;
  origem.pResource=history[0].Get();origem.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;origem.SubresourceIndex=fatia;
  c->CopyTextureRegion(&destino,0,0,0,&origem,nullptr);
  destino.pResource=bil_preservado_prof.Get();origem.pResource=history[2].Get();
  c->CopyTextureRegion(&destino,0,0,0,&origem,nullptr);
  transition(c,history[0].Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,antes);transition(c,history[2].Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,antes);
  transition(c,bil_preservado.Get(),D3D12_RESOURCE_STATE_COPY_DEST,read_state);transition(c,bil_preservado_prof.Get(),D3D12_RESOURCE_STATE_COPY_DEST,read_state);bil_preservado_read=true;
  bil_preservado_job=alvo;++preservacoes;
 }

 void transferir(ID3D12GraphicsCommandList* c,ID3D12Resource* fonte,int job,unsigned idade,bool so_modelo=false,bool da_preservada=false){
  const unsigned n=Policy::history_count;

  const unsigned idade_grade=so_modelo?std::min(idade,n-1):idade;
  struct K{unsigned w,h,atual,fonte,idade,nx,ny,historico;float forca,tau,ridge,escala;unsigned celula,modo,guia;}
   k{width,height,constants.slice,(constants.slice+n-(idade%n))%n,idade_grade,bil_nx,bil_ny,n,bilateral.forca,bilateral.tau,bilateral.ridge,constants.residual_exposure,bilateral.celula,so_modelo?1u:0u,bilateral.guia};
  ID3D12DescriptorHeap* const hb=bil_heap_override?bil_heap_override:bil_heap.Get();
  srv(hb,2,fonte,DXGI_FORMAT_R16G16B16A16_FLOAT);
  auto ligar=[&](ID3D12DescriptorHeap* h,const K& kk){ID3D12DescriptorHeap* heaps[]{h};c->SetDescriptorHeaps(1,heaps);
   c->SetComputeRootSignature(bil_root.Get());c->SetComputeRoot32BitConstants(0,15,&kk,0);
   c->SetComputeRootDescriptorTable(1,gpu(h,0));c->SetComputeRootDescriptorTable(2,gpu(h,11));};
  const unsigned gx=(width+7)/8,gy=(height+7)/8;

  if(da_preservada){srv(bil_heap_preservado.Get(),2,fonte,DXGI_FORMAT_R16G16B16A16_FLOAT);K kp=k;kp.fonte=0;ligar(bil_heap_preservado.Get(),kp);}
  else ligar(hb,k);
  if(da_preservada||(!so_modelo&&job!=bil_job)){
   if(bil_luma_read)transition(c,bil_luma.Get(),read_state,write_state);
   c->SetPipelineState(bil_luma_fonte.Get());c->Dispatch(gx,gy,1);
   transition(c,bil_luma.Get(),write_state,read_state);bil_luma_read=true;
   if(bil_features_read)transition(c,bil_features.Get(),read_state,write_state);
   c->SetPipelineState(bil_prep.Get());c->Dispatch(gx,gy,1);
   transition(c,bil_features.Get(),write_state,read_state);bil_features_read=true;
   if(bil_coef_read)for(auto& t:bil_coef)transition(c,t.Get(),read_state,write_state);
   c->SetPipelineState(bil_fit.Get());c->Dispatch(bil_nx,bil_ny,1);
   for(auto& t:bil_coef)transition(c,t.Get(),write_state,read_state);bil_coef_read=true;
   bil_job=job;++bil_fits;
  }
  if(!bil_coef_read)return;
  if(da_preservada)ligar(hb,k);
  if(bil_luma_read)transition(c,bil_luma.Get(),read_state,write_state);
  c->SetPipelineState(bil_luma_atual.Get());c->Dispatch(gx,gy,1);
  transition(c,bil_luma.Get(),write_state,read_state);bil_luma_read=true;
  transition(c,warped.Get(),read_state,write_state);
  c->SetPipelineState(bil_apply.Get());c->Dispatch(gx,gy,1);
  transition(c,warped.Get(),write_state,read_state);
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
  if(k.estilo>2)k.estilo=0;if(!std::isfinite(k.tom))k.tom=1.f;return k;
 }
 bool alocar_estabilizador(){
  if(!root(device.Get(),15,5,1,est_root)||!(q_chave(L"NeuralQEstabCR")?pipeline(device.Get(),est_root.Get(),g_nr_reproject_estabilizarcr,est_pso):pipeline(device.Get(),est_root.Get(),g_nr_reproject_estabilizar,est_pso)))return false;
  const auto wd=warped->GetDesc();D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  for(unsigned i=0;i<2;++i){if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&wd,write_state,nullptr,IID_PPV_ARGS(&est[i]))))return false;est_estado[i]=write_state;}
  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,12,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&est_heap))))return false;

  for(unsigned i=0;i<2;++i){srv(est_heap.Get(),6*i,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);srv(est_heap.Get(),6*i+1,est[1-i].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
   srv(est_heap.Get(),6*i+2,history[1].Get(),DXGI_FORMAT_R16G16_FLOAT,true);srv(est_heap.Get(),6*i+3,history[2].Get(),DXGI_FORMAT_R32_FLOAT,true);
   srv(est_heap.Get(),6*i+4,history[0].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,true);
   uav(est_heap.Get(),6*i+5,est[i].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);}
  est_cur=0;est_valido=false;return true;
 }

 void estabilizar(ID3D12GraphicsCommandList* c,float alfa,bool sem_limite_parado=false,bool segurar=false,bool movimento=false,bool seguinte=false){
  const unsigned cur=est_cur,prev=1u-cur;
  transition(c,est[cur].Get(),est_estado[cur],write_state);est_estado[cur]=write_state;
  transition(c,est[prev].Get(),est_estado[prev],read_state);est_estado[prev]=read_state;

  const unsigned historico_n=Policy::history_count,fatia_atual=constants.slice%historico_n,fatia_anterior=(fatia_atual+historico_n-1u)%historico_n;
  struct K{unsigned w,h,fatia;float alfa;unsigned valido;float ref;unsigned b,d;float profRel,profAbs;unsigned fatiaAnterior;float alfaMov,alfaLuz,limite,alfaLuzMov;}
   k{width,height,fatia_atual,alfa,est_valido?1u:0u,nrreproject::value(nrreproject::controls,nrreproject::StabilizeStill),
     segurar?2u:(sem_limite_parado&&movimento)?3u:sem_limite_parado?1u:0u,0u,
     constants.depth_relative,constants.depth_absolute,fatia_anterior,post_alfa_mov,
     std::clamp(nrreproject::value(nrreproject::controls,nrreproject::LightSmooth),0.f,1.f),
     std::max(nrreproject::value(nrreproject::controls,nrreproject::GainLimit),0.f),est_luz_mov};
  if(sem_limite_parado&&!segurar)std::memcpy(&k.d,&post_alargar,sizeof k.d);
  if(est_cross>0.f&&!segurar){k.alfa=k.alfaMov=k.alfaLuz=k.alfaLuzMov=est_cross;}
  ID3D12DescriptorHeap* heaps[]{est_heap.Get()};c->SetDescriptorHeaps(1,heaps);
  c->SetComputeRootSignature(est_root.Get());c->SetPipelineState(est_pso.Get());
  c->SetComputeRoot32BitConstants(0,15,&k,0);
  c->SetComputeRootDescriptorTable(1,gpu(est_heap.Get(),6*cur));c->SetComputeRootDescriptorTable(2,gpu(est_heap.Get(),6*cur+5));
  c->Dispatch((width+7)/8,(height+7)/8,1);
  if(seguinte){
   transition(c,est[cur].Get(),write_state,read_state);est_estado[cur]=read_state;cadeia_fonte=1u+cur;
  }else{
  transition(c,est[cur].Get(),write_state,D3D12_RESOURCE_STATE_COPY_SOURCE);est_estado[cur]=D3D12_RESOURCE_STATE_COPY_SOURCE;
  transition(c,warped.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);c->CopyResource(warped.Get(),est[cur].Get());
  transition(c,warped.Get(),D3D12_RESOURCE_STATE_COPY_DEST,read_state);
  }
  est_valido=true;est_cur=prev;
 }

 unsigned cadeia_fonte{};
 static constexpr unsigned filtro_fontes=5,filtro_descr=7;
 static constexpr unsigned filtro_heap_n=2*filtro_fontes*2*filtro_descr;
 static constexpr unsigned filtro_variante(unsigned i,unsigned f,unsigned d){return ((i*filtro_fontes+f)*2u+d)*filtro_descr;}
 ID3D12Resource* fonte_recurso(unsigned f)const{return f==0u?warped.Get():f<=2u?est[f-1u].Get():f==3u?cro_out.Get():f==4u?lg_out.Get():nullptr;}
 bool roda_cro(float e)const{return cro_pso&&cro_ligado()&&std::isfinite(e)&&e>0.f;}
 bool roda_lg(float e)const{return lg_pso&&lg_ligado()&&std::isfinite(e)&&e>0.f;}
 bool roda_su(float e)const{return su_pso&&su_cfg.ligado()&&std::isfinite(e)&&e>0.f;}
 bool cadeia_segue(float e)const{return cadeia_direta.load(std::memory_order_relaxed)&&(roda_cro(e)||roda_lg(e)||roda_su(e));}

 bool ultimo_cro(float e)const{return !cadeia_direta.load(std::memory_order_relaxed)||(!roda_lg(e)&&!roda_su(e));}
 bool ultimo_lg(float e)const{return !cadeia_direta.load(std::memory_order_relaxed)||!roda_su(e);}

 void filtro_descritores(ID3D12DescriptorHeap* h,const ComPtr<ID3D12Resource>* x,ID3D12Resource* saida){
  for(unsigned i=0;i<2;++i)for(unsigned f=0;f<filtro_fontes;++f)for(unsigned d=0;d<2;++d){const unsigned o=filtro_variante(i,f,d);
   srv(h,o,fonte_recurso(f),DXGI_FORMAT_R16G16B16A16_FLOAT);srv(h,o+1,x[1-i].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
   srv(h,o+2,history[1].Get(),DXGI_FORMAT_R16G16_FLOAT,true);srv(h,o+3,history[2].Get(),DXGI_FORMAT_R32_FLOAT,true);
   srv(h,o+4,history[0].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,true);
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
  D3D12_RESOURCE_STATES* st=cadeia_fonte<=2u?&est_estado[cadeia_fonte-1u]:cadeia_fonte==3u?&cro_out_estado:&lg_out_estado;
  ID3D12Resource* f=fonte_recurso(cadeia_fonte);cadeia_fonte=0u;if(!f)return;
  transition(c,f,*st,D3D12_RESOURCE_STATE_COPY_SOURCE);*st=D3D12_RESOURCE_STATE_COPY_SOURCE;
  transition(c,warped.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);c->CopyResource(warped.Get(),f);
  transition(c,warped.Get(),D3D12_RESOURCE_STATE_COPY_DEST,read_state);
 }
 bool alocar_croma(){

  {static const bool lds=[]{double v=0.;return ler_ini(L"NeuralCustoCromaLds",v)&&v>.5;}();const bool usa=lds&&cro_raio*cro_passo<=4u;
   if(!root(device.Get(),20,5,2,cro_root)||(usa?!pipeline(device.Get(),cro_root.Get(),g_nr_reproject_cromalds,cro_pso):!pipeline(device.Get(),cro_root.Get(),g_nr_reproject_croma,cro_pso)))return false;
   if(usa)nrlogfila::linha(L"nr_reproject.log","pid=%lu CUSTO_CROMA_LDS ligado (raio=%u passo=%u)\n",GetCurrentProcessId(),cro_raio,cro_passo);}
  const auto wd=warped->GetDesc();D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  for(unsigned i=0;i<2;++i){if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&wd,write_state,nullptr,IID_PPV_ARGS(&cro[i]))))return false;cro_estado[i]=write_state;}
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&wd,write_state,nullptr,IID_PPV_ARGS(&cro_out))))return false;cro_out_estado=write_state;
  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,filtro_heap_n,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&cro_heap))))return false;

  filtro_descritores(cro_heap.Get(),cro,cro_out.Get());
  cro_cur=0;cro_valido=false;
  {{nrlogfila::linha(L"nr_reproject.log","pid=%lu CROMA_ESTAVEL ligado alfa=%.3f mov=%.3f ref=%.2f delta=%.3f raio=%u passo=%u sigma=%.3f limite=%.3f %ux%u\n",GetCurrentProcessId(),
   double(cro_alfa),double(cro_mov),double(cro_ref),double(cro_delta),cro_raio,cro_passo,double(cro_sigma),double(cro_limite),width,height);}}
  return true;
 }

 void croma(ID3D12GraphicsCommandList* c,float expo,unsigned tom,bool ultimo=true){
  if(!roda_cro(expo)){cro_valido=false;return;}
  const unsigned cur=cro_cur,prev=1u-cur;
  const bool valido=cro_valido&&cro_quadro+1u==policy.frame;
  transition(c,cro[cur].Get(),cro_estado[cur],write_state);cro_estado[cur]=write_state;
  transition(c,cro[prev].Get(),cro_estado[prev],read_state);cro_estado[prev]=read_state;
  bool direto{};const unsigned o=filtro_inicio(c,cur,ultimo,cro_out.Get(),cro_out_estado,direto);
  const unsigned historico_n=Policy::history_count,fatia_atual=constants.slice%historico_n,fatia_anterior=(fatia_atual+historico_n-1u)%historico_n;
  struct K{unsigned w,h,fatia,fatiaAnterior,valido,raio,tom,passo;float alfa,alfaMov,delta,expo,profRel,profAbs,refMov,limite,sigma,r0,r1,r2;}
   k{width,height,fatia_atual,fatia_anterior,valido?1u:0u,cro_raio,tom&1u,cro_passo,cro_alfa,cro_mov,cro_delta,expo,
     constants.depth_relative,constants.depth_absolute,cro_ref,cro_limite,cro_sigma,0.f,0.f,0.f};
  static_assert(sizeof(K)==80);
  ID3D12DescriptorHeap* heaps[]{cro_heap.Get()};c->SetDescriptorHeaps(1,heaps);
  c->SetComputeRootSignature(cro_root.Get());c->SetPipelineState(cro_pso.Get());
  c->SetComputeRoot32BitConstants(0,20,&k,0);
  c->SetComputeRootDescriptorTable(1,gpu(cro_heap.Get(),o));c->SetComputeRootDescriptorTable(2,gpu(cro_heap.Get(),o+5));
  c->Dispatch((width+7)/8,(height+7)/8,1);
  transition(c,cro[cur].Get(),write_state,read_state);cro_estado[cur]=read_state;
  filtro_fim(c,ultimo,direto,cro_out.Get(),cro_out_estado,3u);
  cro_valido=true;cro_quadro=policy.frame;cro_cur=prev;++cro_quadros;
 }
 bool alocar_luz_grossa(){
  if(!root(device.Get(),20,5,2,lg_root)||!pipeline(device.Get(),lg_root.Get(),g_nr_reproject_luzgrossa,lg_pso))return false;
  const auto wd=warped->GetDesc();D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  for(unsigned i=0;i<2;++i){if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&wd,write_state,nullptr,IID_PPV_ARGS(&lg[i]))))return false;lg_estado[i]=write_state;}
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&wd,write_state,nullptr,IID_PPV_ARGS(&lg_out))))return false;lg_out_estado=write_state;
  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,filtro_heap_n,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&lg_heap))))return false;
  filtro_descritores(lg_heap.Get(),lg,lg_out.Get());
  lg_cur=0;lg_valido=false;
  {{nrlogfila::linha(L"nr_reproject.log","pid=%lu LUZ_GROSSA ligada alfa=%.3f mov=%.3f ref=%.2f delta=%.3f raio=%u passo=%u limite=%.3f segura_max=%u segura_fade=%u %ux%u\n",GetCurrentProcessId(),
   double(lg_alfa),double(lg_mov),double(lg_ref),double(lg_delta),lg_raio,lg_passo,double(lg_limite),segura_max,segura_fade,width,height);}}
  return true;
 }
 void luz_grossa(ID3D12GraphicsCommandList* c,float expo,unsigned tom,bool ultimo=true){
  if(!roda_lg(expo)){lg_valido=false;return;}
  const unsigned cur=lg_cur,prev=1u-cur;
  const bool valido=lg_valido&&lg_quadro+1u==policy.frame;
  transition(c,lg[cur].Get(),lg_estado[cur],write_state);lg_estado[cur]=write_state;
  transition(c,lg[prev].Get(),lg_estado[prev],read_state);lg_estado[prev]=read_state;
  bool direto{};const unsigned o=filtro_inicio(c,cur,ultimo,lg_out.Get(),lg_out_estado,direto);
  const unsigned historico_n=Policy::history_count,fatia_atual=constants.slice%historico_n,fatia_anterior=(fatia_atual+historico_n-1u)%historico_n;
  struct K{unsigned w,h,fatia,fatiaAnterior,valido,raio,tom,passo;float alfa,alfaMov,delta,expo,profRel,profAbs,refMov,limite,r0,r1,r2,r3;}
   k{width,height,fatia_atual,fatia_anterior,valido?1u:0u,lg_raio,tom&1u,lg_passo,lg_alfa,lg_mov,lg_delta,expo,
     constants.depth_relative,constants.depth_absolute,lg_ref,lg_limite,0.f,0.f,0.f,0.f};
  static_assert(sizeof(K)==80);
  ID3D12DescriptorHeap* heaps[]{lg_heap.Get()};c->SetDescriptorHeaps(1,heaps);
  c->SetComputeRootSignature(lg_root.Get());c->SetPipelineState(lg_pso.Get());
  c->SetComputeRoot32BitConstants(0,20,&k,0);
  c->SetComputeRootDescriptorTable(1,gpu(lg_heap.Get(),o));c->SetComputeRootDescriptorTable(2,gpu(lg_heap.Get(),o+5));
  c->Dispatch((width+7)/8,(height+7)/8,1);
  transition(c,lg[cur].Get(),write_state,read_state);lg_estado[cur]=read_state;
  filtro_fim(c,ultimo,direto,lg_out.Get(),lg_out_estado,4u);
  lg_valido=true;lg_quadro=policy.frame;lg_cur=prev;++lg_quadros;
 }
 bool alocar_suave(){
  if(!root(device.Get(),20,5,2,su_root)||!pipeline(device.Get(),su_root.Get(),g_nr_reproject_suave,su_pso))return false;
  const auto wd=warped->GetDesc();D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  for(unsigned i=0;i<2;++i){if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&wd,write_state,nullptr,IID_PPV_ARGS(&su[i]))))return false;su_estado[i]=write_state;}
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&wd,write_state,nullptr,IID_PPV_ARGS(&su_out))))return false;su_out_estado=write_state;
  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,filtro_heap_n,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&su_heap))))return false;

  filtro_descritores(su_heap.Get(),su,su_out.Get());
  su_cur=0;su_valido=false;
  {{nrlogfila::linha(L"nr_reproject.log","pid=%lu SUAVE ligado forca=%.3f entrada=%.2f residuo=%.2f mov=%.2f escuro=%d banda=%u %ux%u\n",GetCurrentProcessId(),
   double(su_cfg.forca),double(su_cfg.entrada),double(su_cfg.residuo),double(su_cfg.mov),su_cfg.escuro?1:0,su_cfg.banda,width,height);}}
  return true;
 }

 void suave(ID3D12GraphicsCommandList* c,float expo,unsigned tom){
  if(!roda_su(expo)){su_valido=false;return;}
  const unsigned cur=su_cur,prev=1u-cur;
  const bool valido=su_valido&&su_quadro+1u==policy.frame;
  transition(c,su[cur].Get(),su_estado[cur],write_state);su_estado[cur]=write_state;
  transition(c,su[prev].Get(),su_estado[prev],read_state);su_estado[prev]=read_state;
  bool direto{};const unsigned o=filtro_inicio(c,cur,true,su_out.Get(),su_out_estado,direto);
  const unsigned historico_n=Policy::history_count,fatia_atual=constants.slice%historico_n,fatia_anterior=(fatia_atual+historico_n-1u)%historico_n;
  const nrsuave::Constantes k=nrsuave::constantes(su_cfg,width,height,fatia_atual,fatia_anterior,valido,tom,expo,constants.depth_relative,constants.depth_absolute);
  ID3D12DescriptorHeap* heaps[]{su_heap.Get()};c->SetDescriptorHeaps(1,heaps);
  c->SetComputeRootSignature(su_root.Get());c->SetPipelineState(su_pso.Get());
  c->SetComputeRoot32BitConstants(0,20,&k,0);
  c->SetComputeRootDescriptorTable(1,gpu(su_heap.Get(),o));c->SetComputeRootDescriptorTable(2,gpu(su_heap.Get(),o+5));
  c->Dispatch((width+7)/8,(height+7)/8,1);
  transition(c,su[cur].Get(),write_state,read_state);su_estado[cur]=read_state;
  filtro_fim(c,true,direto,su_out.Get(),su_out_estado,0u);
  su_valido=true;su_quadro=policy.frame;su_cur=prev;++su_quadros;
 }
 bool alocar_preenchimento(){
  if(!root(device.Get(),8,2,1,fill_root)||!pipeline(device.Get(),fill_root.Get(),g_nr_reproject_fill,fill_pso))return false;
  const auto wd=warped->GetDesc();D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&wd,read_state,nullptr,IID_PPV_ARGS(&fill_copia))))return false;
  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,3,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&fill_heap))))return false;
  srv(fill_heap.Get(),0,history[2].Get(),DXGI_FORMAT_R32_FLOAT,true);srv(fill_heap.Get(),1,fill_copia.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
  uav(fill_heap.Get(),2,warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
  return true;
 }

 void preencher(ID3D12GraphicsCommandList* c){

  struct K{unsigned w,h,fatia;float limiar,relativa,absoluta;unsigned raio,pad;}
   k{width,height,constants.slice,preenchimento,constants.depth_relative,constants.depth_absolute,256u,modo_preenchimento()};

  static const bool dupla=[]{double v=0.;return ler_ini(L"NeuralTesteFillCopiaDupla",v)&&v>.5;}();
  {
  transition(c,fill_copia.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);
  transition(c,warped.Get(),read_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
  c->CopyResource(fill_copia.Get(),warped.Get());
  if(dupla)c->CopyResource(fill_copia.Get(),warped.Get());
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

  if(suprimir_despejo)return {};
  if(const unsigned pedido=nrreproject::despejo_pedido.exchange(0)){despejo.restantes=pedido;despejo.espera=0;despejo.intervalo=0;nrreproject::despejo_gravados.store(0);}
  if(!despejo.restantes||despejos_reservados>=despejo_maximo||!warped||!device)return {};
  if(despejo.espera){--despejo.espera;return {};}
  if(despejo.intervalo){if(despejo.contador++%despejo.intervalo)return {};}
  const auto wd=warped->GetDesc();
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 total{};
  device->GetCopyableFootprints(&wd,0,1,0,&fp,nullptr,nullptr,&total);
  if(!total)return {};
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;
  D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=total;bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  ComPtr<ID3D12Resource> leitura;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&leitura))))return {};
  DespejoPendente d{leitura,begins+3,width,height,despejos_reservados,
                      fp.Footprint.RowPitch,std::size_t(total),despejo.ganho};
  d.par_solicitado=despejo.etapas_fill;

  if(despejo.etapas_fill&&SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&d.antes)))){
   d.raw=true;d.fatia=constants.slice;d.idade=constants.age;d.quadro=policy.frame;
   d.pontual=pontual;d.fill_ativo=preenchimento>0&&fill_pso;d.fill_limiar=d.fill_ativo?preenchimento:0.f;
   transition(c,warped.Get(),read_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
   D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=d.antes.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=fp;
   src.pResource=warped.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
   c->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
   transition(c,warped.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,read_state);
  }
  --despejo.restantes;++despejos_reservados;nrreproject::despejo_gravados.fetch_add(1);
  return d;
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
   if(stotal&&SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&sbd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&leitura_fonte)))) {
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
   if(mtotal&&SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&mbd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&leitura_mv)))) {
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
   if(dtotal&&SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&dbd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&leitura_depth)))) {
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
 void log(int job,int age,const char* reason){trace(job,age,reason);const auto events=composed+skipped+unsupported;if(events>3&&(events&255)!=0)return;{nrlogfila::linha(L"nr_reproject.log","pid=%lu frame=%llu job=%d age=%d composed=%llu skipped=%llu unsupported=%llu retidos=%llu preservadas=%llu reason=%s\n",GetCurrentProcessId(),policy.frame,job,age,composed,skipped,unsupported,retidos,preservacoes,reason);}}
};

Backend::Backend():p_([]{nrinijogo::Lote lote;return std::make_unique<Impl>();}()){}Backend::~Backend()=default;

void Backend::invalidate() noexcept {if(p_){p_->policy.invalidate();p_->est_valido=false;p_->cro_valido=false;p_->lg_valido=false;p_->su_valido=false;p_->ganho_valido=false;p_->luz_valido=false;p_->perder_post("post_lost_invalidate");
 for(auto& a:p_->late.armed)a=0;}}

void Backend::invalidate_suave() noexcept {if(p_){p_->policy.invalidate();p_->perder_post("post_lost_invalidate_suave");for(auto& a:p_->late.armed)a=0;}}

void Backend::reset() noexcept {nrinijogo::Lote lote;p_.reset(new(std::nothrow) Impl);}
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
                 unsigned(p.retired.size()),p.post_composed,p.post_refused,p.post_lost};
}
bool Backend::begin(const NrCaptureRecord& r){
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
   NRLOGF(detalhe,sizeof detalhe,"campo=\"%s\" cor=fmt%u_%llux%u_flags0x%x mv=fmt%u_%llux%u_flags0x%x prof=fmt%u_%llux%u_flags0x%x mvScale=%.6g,%.6g limite=4096",campo,
                 unsigned(dc.Format),static_cast<unsigned long long>(dc.Width),dc.Height,unsigned(dc.Flags),unsigned(dm.Format),static_cast<unsigned long long>(dm.Width),dm.Height,unsigned(dm.Flags),
                 unsigned(dp.Format),static_cast<unsigned long long>(dp.Width),dp.Height,unsigned(dp.Flags),static_cast<double>(r.motion_scale_x),static_cast<double>(r.motion_scale_y));
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
 const unsigned states[]{r.color_state,r.motion_state,r.depth_state,r.exposure_state};for(unsigned i=0;i<(exposure?4u:3u);++i)if(state(states[i])==D3D12_RESOURCE_STATES(~0u)){
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
    if(!post||!nrposteto::folga_pre.load()){
     const std::uint64_t folga=orc>uso?orc-uso:1;const auto q=nrprescale::unpack_quadro(nrprescale::quadro.load(std::memory_order_acquire));
     nrposteto::folga_pre.store(folga);

     if(post)nrposteto::medida_vram.uso_pre=uso;
     const unsigned vram=unsigned(nrposteto::teto_vram(folga,std::uint64_t(q.saida_w)*q.saida_h)*100.+.5);
     nrposteto::teto_vram_centi.store(vram);

     if(nrposteto::automatico()&&vram<nrprescale::post_teto_centi.load(std::memory_order_relaxed))nrprescale::post_teto_centi.store(vram,std::memory_order_relaxed);}

    {const auto q=nrprescale::unpack_quadro(nrprescale::quadro.load(std::memory_order_acquire));
     const std::uint64_t saida_px=std::uint64_t(q.saida_w)*q.saida_h;
     const double medido=nrposteto::amostra_vram(nrposteto::medida_vram,post,uso,nrposteto::folga_pre.load(),saida_px,
                                                 post?nrprescale::efetivo.load(std::memory_order_relaxed):100.,agora);
     if(!post)nrposteto::medida_vram_relatada=false;
     const double atual=double(nrposteto::teto_vram_centi.load())/100.;
     const double aplicado=nrposteto::aplicar_medido(nrposteto::medida_vram,atual,medido,uso);
     if(medido>0){const unsigned centi=unsigned(aplicado*100.+.5);

      if(nrposteto::teto_vram_centi.exchange(centi)!=centi||!nrposteto::medida_vram_relatada){nrposteto::medida_vram_relatada=true;char d[160];
       NRLOGF(d,sizeof d,"teto_vram_medido=%.1f%% aplicado=%.1f%% custo=%llu MB rede=%.1f%% folga_pre=%llu MB",medido,aplicado,
                     static_cast<unsigned long long>((uso>nrposteto::medida_vram.uso_pre?uso-nrposteto::medida_vram.uso_pre:0)>>20),
                     nrprescale::efetivo.load(std::memory_order_relaxed),static_cast<unsigned long long>(nrposteto::folga_pre.load()>>20));
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
 p.preservar_fonte(c);
 if(p.histories_read)for(auto& h:p.history)transition(c,h.Get(),read_state,write_state);p.histories_read=false;
 const float dual=Impl::ajuste(nrreproject::DualMotion,thresholds().dual_motion),limiar_dual=Impl::ajuste(nrreproject::DualThreshold,thresholds().dual_threshold);
 auto& k=p.constants;k={};k.width=p.width;k.height=p.height;k.slice=p.policy.begin();k.mask=p.policy.mask();k.count=Policy::history_count;k.flags=1u|bits_cor(cor_vista)|(p.pontual?512u:0u)|(exposure&&p.portao_exposicao?2u:0u)|(dual>0.f?(unsigned(std::min(7.f,std::max(1.f,std::floor(dual+.5f))))<<2):0u)|(unsigned(std::min(15.f,std::max(0.f,std::floor(limiar_dual*4.f+.5f))))<<5);

 if(nrreproject::index_of(nrreproject::controls,nrreproject::DepthAwareRead)!=0u)k.flags|=1024u;
 k.inverse_x=1.f/p.width;k.inverse_y=1.f/p.height;k.depth_absolute=Impl::ajuste(nrreproject::DepthAbsolute,thresholds().depth_absolute);k.current_exposure=k.source_exposure=1;{k.residual_exposure=nrreproject::multiplicador_escala();}
 k.motion_reference=Impl::ajuste(nrreproject::MotionReference,thresholds().motion_reference);

 const float motion_factor=nrreproject::value(nrreproject::controls,nrreproject::MotionFactor);
 k.scale_x=r.motion_scale_x*motion_factor/float(md.Width);k.scale_y=r.motion_scale_y*motion_factor/float(md.Height);

 k.depth_relative=nrreproject::value(nrreproject::controls,nrreproject::Depth);
 k.color_threshold=nrreproject::value(nrreproject::controls,nrreproject::Color);
 k.clamp_strength=nrreproject::value(nrreproject::controls,nrreproject::Clamp);
 k.motion_divergence=nrreproject::value(nrreproject::controls,nrreproject::Divergence);
 k.confidence_rate=nrreproject::value(nrreproject::controls,nrreproject::Hysteresis);
 k.deform_tolerance=nrreproject::value(nrreproject::controls,nrreproject::DeformTolerance);
 k.gate_smooth=Impl::ajuste(nrreproject::GateSmooth,thresholds().gate_smooth);
 k.deform_blur=Impl::ajuste(nrreproject::DeformBlur,thresholds().deform_blur);
 k.detail_motion=nrreproject::value(nrreproject::controls,nrreproject::DetailMotion);
 k.occlusion_tolerance=nrreproject::value(nrreproject::controls,nrreproject::OcclusionTolerance);
 k.residual_floor=Impl::ajuste(nrreproject::ResidualFloor,thresholds().residual_floor);
 k.confidence_floor=Impl::ajuste(nrreproject::ConfidenceFloor,thresholds().confidence_floor);

 {static const unsigned motivo=[]{double v=0;return ler_ini(L"NeuralReprojectDebugReason",v)&&v==1.?1u:0u;}();

  k.pad=motivo|(unsigned(nrreproject::value(nrreproject::controls,nrreproject::DepthSnap)+.5f)&3u)<<8;

  {const float rp=nrreproject::value(nrreproject::controls,nrreproject::MotionRepair);k.pad|=(rp>3.5f?3u:rp>2.5f?2u:rp>1.5f?1u:0u)<<10;}
  {static const unsigned simula=[]{double v=0;return ler_ini(L"NeuralReprojectSimulateHalo",v)&&v>=1&&v<=3?unsigned(v):0u;}();k.pad|=simula<<12;}

  {const float rd=nrreproject::value(nrreproject::controls,nrreproject::RawDivergence);k.pad|=(rd<.5f?0u:rd>15.f?1u:rd>6.f?2u:3u)<<16;}

  k.pad|=(nrreproject::index_of(nrreproject::controls,nrreproject::MotionStrength)&7u)<<20;
  k.pad|=(nrreproject::index_of(nrreproject::controls,nrreproject::MotionStrengthRef)&3u)<<24;

  k.pad|=(nrreproject::index_of(nrreproject::controls,nrreproject::NoiseCancel)&7u)<<26;

  k.pad|=(nrreproject::index_of(nrreproject::controls,nrreproject::RatioTransfer)&1u)<<30;

  if(nrreproject::value(nrreproject::controls,nrreproject::EdgeCarry)>.5f){
   const unsigned rb=nrreproject::faixa_borda_raio(p.borda_r100,p.pct_raio());
   if(rb){k.flags|=2048u;k.pad|=(rb&127u)<<1;++p.borda_quadros;p.borda_raio=rb;}
  }}
 if(k.slice<Policy::history_count){p.jitter_fatia[k.slice*2]=nrreproject::jitter_atual_x.load();p.jitter_fatia[k.slice*2+1]=nrreproject::jitter_atual_y.load();p.jitter_ok[k.slice]=nrreproject::jitter_valido.load();}
 p.log_constants(k);
 p.bind(c,p.temporal_heap.Get(),p.temporal_root.Get(),p.capture_pso.Get(),32,&k,8);c->Dispatch((p.width+7)/8,(p.height+7)/8,1);
 for(auto& h:p.history)transition(c,h.Get(),write_state,read_state);p.histories_read=true;

 rj_recorte(r.command_list,color,unsigned(cor_vista),post);
 if(nrrj::npc2.load(std::memory_order_relaxed))for(int kn=1;kn<=nrrj::npc_n();++kn){nrrj::Inst i1(kn);rj_recorte(r.command_list,color,unsigned(cor_vista),post);}
 for(unsigned i=0;i<(exposure?4u:3u);++i)transition(c,inputs[i],read_state,state(states[i]));p.captured_command=r.command_list;return true;
}
Result Backend::compose(void* command,void* destination,unsigned destination_state,void* residual,int job,float exposure,unsigned tone){
 if(!p_)return Result::Failed;auto& p=*p_;p.atualizar_ao_vivo();int age=p.policy.age(job);p.trace_expo=exposure;

 int idade_modelo=-1;bool da_preservada=false;
 if(p.ready&&!p.reduzido&&age<0&&p.bilateral.retencao>0&&p.bilateral.forca>0&&p.bil_apply){

  if(job==p.bil_preservado_job&&job!=p.bil_job){const int longa=p.policy.long_age(job);if(longa>=0&&unsigned(longa)<=p.bilateral.retencao){idade_modelo=longa;da_preservada=true;}}
  else if(p.bil_coef_read&&p.bil_job>0){const int longa=p.policy.long_age(p.bil_job);if(longa>=0&&unsigned(longa)<=p.bilateral.retencao)idade_modelo=longa;}}

 bool segurar=false;
 if(!p.ready||(age<0&&idade_modelo<0)){
  if(p.pode_segurar(command))segurar=true;
  else{++p.skipped;p.log(job,age,"old_or_missing_source");return Result::Skipped;}
 }

 if(p.composed_frame||command!=p.captured_command){++p.skipped;p.log(job,age,"duplicate_or_different_command");return Result::Skipped;}
 auto* c=static_cast<ID3D12GraphicsCommandList*>(command);auto* dst=static_cast<ID3D12Resource*>(destination);auto* res=static_cast<ID3D12Resource*>(residual);
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
 nrcusto::marca(c,nrcusto::ReduzirFim);
 if(!segurar)p.srv(warp_heap,6,res_warp,res_warp==res?rd.Format:DXGI_FORMAT_R16G16B16A16_FLOAT);
 p.uav(p.compose_heap.Get(),compose_variante*variante+compose_uavs,dst,dd.Format);
 if(segurar){++p.segurados_seguidos;++p.segurados_total;nrposteto::segurados_total.fetch_add(1,std::memory_order_relaxed);

  if(!p.warped_read){transition(c,p.warped.Get(),write_state,read_state);p.warped_read=true;}
  {float f=0.f;

   if(p.segura_fade>0&&p.segurados_seguidos>p.segura_fade&&p.segura_max>p.segura_fade){
    auto rampa=[&](unsigned n){return n<=p.segura_fade?1.f:(std::max)(1e-3f,1.f-float(n-p.segura_fade)/float(p.segura_max-p.segura_fade));};
    f=rampa(p.segurados_seguidos)/rampa(p.segurados_seguidos-1u);}
   p.cadeia_fonte=0u;p.estabilizar(c,f,false,true,false,p.cadeia_segue(exposure));}

  if(p.cro_ligado())p.croma(c,exposure,tone,p.ultimo_cro(exposure));
  if(p.lg_ligado())p.luz_grossa(c,exposure,tone,p.ultimo_lg(exposure));
  if(p.su_cfg.ligado())p.suave(c,exposure,tone);
  p.cadeia_fechar(c);
 }else{
 p.segurados_seguidos=0;
 if(idade_modelo<0){
 if(p.warped_read)transition(c,p.warped.Get(),read_state,write_state);p.warped_read=false;p.constants.age=unsigned(age);
 {

  const float sinal=nrreproject::value(nrreproject::controls,nrreproject::JitterComp);
  const unsigned n=Policy::history_count,atual=p.constants.slice%n,fonte=(p.constants.slice+n-unsigned(age)%n)%n;
  p.constants.jitter_x=p.constants.jitter_y=0.f;

  p.constants.pad=(p.constants.pad&~(1u<<29))|((tone&1u)<<29);
  if(std::isfinite(exposure)&&exposure>0.f)p.constants.current_exposure=p.constants.source_exposure=exposure;
  if(sinal!=0.f&&p.jitter_ok[atual]&&p.jitter_ok[fonte]&&p.width&&p.height){
   p.constants.jitter_x=sinal*(p.jitter_fatia[fonte*2]-p.jitter_fatia[atual*2])/float(p.width);
   p.constants.jitter_y=sinal*(p.jitter_fatia[fonte*2+1]-p.jitter_fatia[atual*2+1])/float(p.height);}
 }
 {

  const unsigned nivel=unsigned(nrreproject::value(nrreproject::controls,nrreproject::HaloCutMoving)+.5f);
  p.corte_raio_warp=nrreproject::corte_raio_warp(nivel,p.pct_raio());
  p.constants.flags&=~((3u<<12)|(63u<<16));
  if(p.corte_raio_warp)p.constants.flags|=((nivel&3u)<<12)|((p.corte_raio_warp&63u)<<16);
  if(age>=0)p.idade_media=p.idade_media>0.f?p.idade_media+.1f*(float(age)-p.idade_media):float(age);
 }
 p.bind(c,warp_heap,p.temporal_root.Get(),p.warp_pso.Get(),32,&p.constants,8);c->Dispatch((p.width+7)/8,(p.height+7)/8,1);transition(c,p.warped.Get(),write_state,read_state);p.warped_read=true;
 }else if(!p.warped_read){transition(c,p.warped.Get(),write_state,read_state);p.warped_read=true;}
 nrcusto::marca(c,nrcusto::WarpFim);

 const auto despejo=p.reservar_despejo(c);

 if(despejo.leitura){{nrlogfila::linha(L"nr_reproject.log","pid=%lu frame=%llu DUMPINFO indice=%u job=%d age=%d modelo=%d\n",GetCurrentProcessId(),p.policy.frame,despejo.indice,job,age,idade_modelo);}}

 if(idade_modelo>=0)p.transferir(c,res,da_preservada?job:p.bil_job,unsigned(idade_modelo),true,da_preservada);
 else if(!p.reduzido&&p.bilateral.forca>0&&p.bil_apply)p.transferir(c,res,job,unsigned(age));
 else if(p.preenchimento>0&&p.fill_pso)p.preencher(c);
 nrcusto::marca(c,nrcusto::FillFim);

 {float alfa=nrreproject::value(nrreproject::controls,nrreproject::Stabilize);
  bool sem_limite=false;
  if(p.post_atual&&!(alfa>0.f&&alfa<1.f)&&p.post_estab>0.f){alfa=p.post_estab;sem_limite=true;}
  {const bool troca=job!=p.est_ultimo_job;p.est_ultimo_job=job;
   const bool fg=nrreproject::fg_ativo();
   p.est_luz_mov=(troca&&p.fg_troca_luz>0.f&&(p.fg_troca_sempre||fg))?p.fg_troca_luz:p.post_luz_mov;

   if(p.fg_log_estado!=int(fg)){p.fg_log_estado=int(fg);{
    nrlogfila::linha(L"nr_reproject.log","pid=%lu FG_TROCA fg_ativo=%d NeuralFgTrocaLuz=%.3f NeuralFgTrocaLuzSempre=%d NeuralPostLuzMov=%.3f NeuralFgCrossfade=%d estabilizador_post=%.3f mov=%.3f quadro=%llu\n",
     GetCurrentProcessId(),int(fg),p.fg_troca_luz,int(p.fg_troca_sempre),p.post_luz_mov,int(p.fg_cross),p.post_estab,p.post_alfa_mov,static_cast<unsigned long long>(p.policy.frame));}}}

  p.est_cross=0.f;
  if(p.fg_cross&&(p.fg_cross_sempre||nrreproject::fg_ativo())){
   if(job!=p.cross_job){if(p.cross_job>0&&p.cross_j>0)p.cross_P=p.cross_P>0.f?p.cross_P+.125f*(float(p.cross_j)-p.cross_P):float(p.cross_j);p.cross_job=job;p.cross_j=0;}
   const float P=std::clamp(p.cross_P>0.f?p.cross_P:2.f,1.f,8.f);
   const float a=1.f/(std::max)(1.f,P-float(p.cross_j));++p.cross_j;
   p.est_cross=std::clamp(a,.05f,.999f);alfa=p.est_cross;sem_limite=true;
  }
  p.cadeia_fonte=0u;
  if(alfa>0.f&&alfa<1.f&&p.est_pso)p.estabilizar(c,alfa,sem_limite,false,p.est_cross>0.f||(sem_limite&&p.post_filtro_mov),p.cadeia_segue(exposure));else p.est_valido=false;}
 nrcusto::marca(c,nrcusto::EstabFim);

 if(p.cro_ligado())p.croma(c,exposure,tone,p.ultimo_cro(exposure));
 if(p.lg_ligado())p.luz_grossa(c,exposure,tone,p.ultimo_lg(exposure));

 if(p.su_cfg.ligado())p.suave(c,exposure,tone);
 p.cadeia_fechar(c);
 p.gravar_despejo(c,res_warp,despejo);
 }
 nrcusto::marca(c,nrcusto::CadeiaFim);
 if(p.reduzido){

  p.ganho_valido=false;p.luz_valido=false;p.halo_exc_raio=0u;p.corte_raio_comp=0u;

  ID3D12DescriptorHeap* const ph=p.halo_post(c,exposure,tone)?p.hpost.post_heap.Get():p.q_post(c,exposure,tone)?p.qpost.post_heap.Get():p.post_heap.Get();
  p.uav(ph,2,dst,dd.Format);
  transition(c,dst,D3D12_RESOURCE_STATES(destination_state),write_state);
  ComposeConstants k=p.constantes_compose(teto_unico(tone)&~6u,p.comp_w,p.comp_h,exposure);
  if(p.q_post_ganho!=1.f)std::memcpy(&k.p2,&p.q_post_ganho,sizeof k.p2);

  const bool npc_cena=ph==p.post_heap.Get()&&p.cnpc_cfg.modo&&p.cena_npc(c,dst,dd.Format,&k);
  if(!npc_cena)p.bind(c,ph,p.post_root.Get(),p.post_pso.Get(),12,&k,2);
  c->Dispatch((p.comp_w+7)/8,(p.comp_h+7)/8,1);order_uav(c,dst);

  if(!segurar)rj_compor(c,dst,unsigned(dd.Format),job,age,exposure,tone);
  if(!segurar&&nrrj::npc2.load(std::memory_order_relaxed))for(int kn=1;kn<=nrrj::npc_n();++kn){nrrj::Inst i1(kn);rj_compor(c,dst,unsigned(dd.Format),job,age,exposure,tone);}
  transition(c,dst,write_state,D3D12_RESOURCE_STATES(destination_state));
  nrcusto::marca(c,nrcusto::ComposeFim);
  p.composed_frame=true;++p.composed;nrreproject::idade_composta.store(age);
  nrreproject::base_fatia.store(p.constants.slice);nrreproject::base_recurso.store(p.history[0].Get());
  p.log(job,age,segurar?"ok_segurado_post_reduzido":"ok_post_reduzido");return Result::Composed;
 }
 if(p.modo_post()){

  p.ganho_valido=false;p.luz_valido=false;
  p.perder_post("post_lost_sobrescrito");p.post={true,command,job,exposure,tone};p.composed_frame=true;++p.composed;nrreproject::idade_composta.store(idade_modelo>=0?idade_modelo:age);
  nrreproject::base_fatia.store(p.constants.slice);nrreproject::base_recurso.store(p.history[0].Get());
  if(idade_modelo>=0)++p.retidos;
  p.log(job,idade_modelo>=0?idade_modelo:age,segurar?"ok_segurado_adiado_pos_fsr":"ok_adiado_pos_fsr");return Result::Composed;
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

  {static const float forcaTeto=[]{double v=0;return ler_ini(L"NeuralHaloTeto",v)&&v>0.?float(std::min(v,1.)):0.f;}();
   static const double raioTeto=[]{double v=16;return ler_ini(L"NeuralHaloTetoRaio",v)&&v>=2.&&v<=30.?v:16.;}();
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
  static const unsigned forca15=[]{double v=0;return ler_ini(L"NeuralHaloTeto",v)&&v>0.?unsigned(std::lround(std::min(v,1.)*15.)):0u;}();
  if(forca15){k.p0|=((p.halo_teto_raio/2u)&15u)<<28;k.p1|=32u|(forca15<<28);if(!limitar)k.p1|=(p.constants.slice%Policy::history_count)<<8;++p.halo_teto_quadros;}
 }
 {ID3D12DescriptorHeap* heaps[]{p.compose_heap.Get()};c->SetDescriptorHeaps(1,heaps);c->SetComputeRootSignature(p.compose_root.Get());c->SetPipelineState(p.compose_pso.Get());
  c->SetComputeRoot32BitConstants(0,16,&k,0);c->SetComputeRootDescriptorTable(1,p.gpu(p.compose_heap.Get(),compose_variante*variante));c->SetComputeRootDescriptorTable(2,p.gpu(p.compose_heap.Get(),compose_variante*variante+compose_uavs));}
 c->Dispatch((p.width+7)/8,(p.height+7)/8,1);order_uav(c,dst);
 transition(c,dst,write_state,D3D12_RESOURCE_STATES(destination_state));
 nrcusto::marca(c,nrcusto::ComposeFim);
 if(limitar){p.ganho_valido=true;p.ganho_dividido=dividir;p.ganho_quadro=p.policy.frame;p.ganho_cur=1u-variante;}else p.ganho_valido=false;
 p.composed_frame=true;++p.composed;nrreproject::idade_composta.store(idade_modelo>=0?idade_modelo:age);
 nrreproject::base_fatia.store(p.constants.slice);nrreproject::base_recurso.store(p.history[0].Get());
 if(idade_modelo>=0){++p.retidos;p.log(job,idade_modelo,da_preservada?"ok_modelo_fonte_preservada":"ok_modelo_retido");}else p.log(job,age,segurar?"ok_segurado":"ok");return Result::Composed;
}

Result Backend::compose_post(void* command,void* output,std::uint32_t ffx_state){
 if(!p_)return Result::Failed;auto& p=*p_;
 if(!p.post.ativo||command!=p.post.command||command!=p.captured_command)return Result::Skipped;
 p.post.ativo=false;
 auto* c=static_cast<ID3D12GraphicsCommandList*>(command);auto* out=static_cast<ID3D12Resource*>(output);
 auto recusar=[&](const char* motivo,const D3D12_RESOURCE_DESC* od){p.post_recusado=true;++p.post_refused;p.log_post(p.post.job,motivo,od);return Result::Unsupported;};

 const auto estado=state(ffx_state);
 if(estado==D3D12_RESOURCE_STATES(~0u))return recusar("post_unsupported_state_fallback_pre",nullptr);
 if(!c||!out||!device_matches(p.device.Get(),c)||!device_matches(p.device.Get(),out))return recusar("post_unsupported_device_fallback_pre",nullptr);
 const auto od=out->GetDesc();

 if(od.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||od.DepthOrArraySize!=1||od.MipLevels!=1||od.SampleDesc.Count!=1||!(od.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)||
    od.Width<p.width||od.Height<p.height||od.Width>16384||od.Height>16384||!format_support(p.device.Get(),od.Format,true))return recusar("post_unsupported_format_fallback_pre",&od);
 if(!p.warped_read){transition(c,p.warped.Get(),write_state,read_state);p.warped_read=true;}
 ID3D12DescriptorHeap* const ph=p.halo_post(c,p.post.exposure,p.post.tone)?p.hpost.post_heap.Get():p.q_post(c,p.post.exposure,p.post.tone)?p.qpost.post_heap.Get():p.post_heap.Get();
 p.uav(ph,2,out,od.Format);

 if(estado==write_state)order_uav(c,out);else transition(c,out,estado,write_state);
 ComposeConstants k=p.constantes_compose(teto_unico(p.post.tone)&~6u,unsigned(od.Width),od.Height,p.post.exposure);
 if(p.q_post_ganho!=1.f)std::memcpy(&k.p2,&p.q_post_ganho,sizeof k.p2);
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

constexpr unsigned lados_final=6;
struct Buffers { ComPtr<ID3D12Resource> leitura[lados_final];UINT64 tamanho[lados_final]{}; };
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
 if(FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&f->buf)))||FAILED(f->buf->Map(0,nullptr,&m))||!m){delete f;return nullptr;}
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
unsigned final_reserva_modo(){static const unsigned v=[]{double x=0.;return ler_ini(L"NeuralGravarF9Reserva",x)?(x==1.?1u:x==2.?2u:0u):0u;}();return v;}
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
  if(SUCCEEDED(dev->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&b.leitura[k])))){b.tamanho[k]=total[k];bytes+=total[k];{nrorc::EscopoOrigem eo(nrorc::Origem::F9);nrorc_vram::registrar(nrorc::Api::Recurso,0,"f9_readback");}}}
 return b;
}

bool reserva_serve(const std::vector<Buffers>& v,ID3D12Resource* const* rs,const UINT64* total){
 for(const auto& b:v)for(unsigned k=0;k<lados_final;++k)if(rs[k]&&b.tamanho[k]<total[k])return false;
 return true;
}

void soltar_no_fundo(std::vector<Buffers>& v){
 if(v.empty())return;std::vector<Buffers> w;w.swap(v);
 try{std::thread([x=std::move(w)]()mutable{x.clear();}).detach();}catch(...){}
}

void lancar_reserva(ID3D12Device* dev,const UINT64* total,ID3D12Resource* const* rs,unsigned n){
 if(!n||reserva_voo.exchange(true))return;
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
 try{
  final_preparo_pedido.store(false);
  if(reserva_modo_efetivo()!=2u)return;
  auto* c=static_cast<ID3D12GraphicsCommandList*>(command);if(!c)return;
  std::lock_guard<std::mutex> trava(final_mutex);
  if(reserva_voo.load()||despejo_final.load()||final_indice>=final_maximo_())return;
  ComPtr<ID3D12Device> dev;if(FAILED(c->GetDevice(IID_PPV_ARGS(&dev)))||!dev)return;

  const FinalExtras extras=final_extras;const bool exato=final_movimento_ini()&&extras.valido;
  ID3D12Resource* rs[lados_final]{static_cast<ID3D12Resource*>(input),static_cast<ID3D12Resource*>(output),modo==0?static_cast<ID3D12Resource*>(base_recurso.load()):nullptr,
   exato?static_cast<ID3D12Resource*>(extras.motion):nullptr,exato?static_cast<ID3D12Resource*>(extras.depth):nullptr,exato?static_cast<ID3D12Resource*>(extras.exposure):nullptr};
  const unsigned sub[lados_final]{0,0,base_fatia.load(),0,0,0};UINT64 total[lados_final]{};
  for(unsigned k=0;k<lados_final;++k){
   if(!rs[k])continue;const D3D12_RESOURCE_DESC desc=rs[k]->GetDesc();
   if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||desc.SampleDesc.Count!=1||desc.Width>8192||desc.Height>8192||sub[k]>=desc.DepthOrArraySize){rs[k]=nullptr;continue;}
   if(k>=3){D3D12_FEATURE_DATA_FORMAT_INFO fi{desc.Format,0};if(FAILED(dev->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO,&fi,sizeof fi))||fi.PlaneCount!=1){rs[k]=nullptr;continue;}}
   D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT linhas{};UINT64 linha{};dev->GetCopyableFootprints(&desc,sub[k],1,0,&fp,&linhas,&linha,&total[k]);if(!total[k])rs[k]=nullptr;
  }
  if(!rs[0]&&!rs[1])return;
  if(!reserva_serve(reserva,rs,total))soltar_no_fundo(reserva);
  const unsigned alvo=std::min(gravar_quadros.load(),final_maximo_()-final_indice);
  if(reserva.size()<alvo)lancar_reserva(dev.Get(),total,rs,alvo-unsigned(reserva.size()));
 }catch(...){}
}
bool despejar_final(void* command,void* input,std::uint32_t estado_entrada,void* output,std::uint32_t estado_saida,unsigned modo,std::uint64_t quadro) noexcept{
 try{
  auto* c=static_cast<ID3D12GraphicsCommandList*>(command);if(!c)return false;
  std::lock_guard<std::mutex> trava(final_mutex);
  if(final_indice>=final_maximo_()){if(gravar_aviso.load(std::memory_order_relaxed)){final_cheio.store(true);despejo_final.store(0);}return false;}
  ComPtr<ID3D12Device> dev;if(FAILED(c->GetDevice(IID_PPV_ARGS(&dev)))||!dev)return false;
  const unsigned modo_reserva=reserva_modo_efetivo();
  if(modo_reserva)final_movimento=final_movimento_ini();
  else if(reserva.empty()){double v{};final_movimento=ler_ini(L"NeuralDumpFinalMovimento",v)&&v==1.;}
  const FinalExtras extras=final_extras;const bool exato=final_movimento&&extras.valido;
  ID3D12Resource* rs[lados_final]{static_cast<ID3D12Resource*>(input),static_cast<ID3D12Resource*>(output),modo==0?static_cast<ID3D12Resource*>(base_recurso.load()):nullptr,
   exato?static_cast<ID3D12Resource*>(extras.motion):nullptr,exato?static_cast<ID3D12Resource*>(extras.depth):nullptr,exato?static_cast<ID3D12Resource*>(extras.exposure):nullptr};
  const unsigned fatia_base=base_fatia.load();
  const std::uint32_t est[lados_final]{estado_entrada,estado_saida,4u,extras.motion_state,extras.depth_state,extras.exposure_state};
  D3D12_RESOURCE_DESC desc[lados_final]{};D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp[lados_final]{};UINT linhas[lados_final]{};UINT64 total[lados_final]{};unsigned sub[lados_final]{0,0,fatia_base,0,0,0};
  for(unsigned k=0;k<lados_final;++k){
   if(!rs[k])continue;desc[k]=rs[k]->GetDesc();
   if(desc[k].Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||desc[k].SampleDesc.Count!=1||desc[k].Width>8192||desc[k].Height>8192||sub[k]>=desc[k].DepthOrArraySize){rs[k]=nullptr;continue;}
   if(k>=3){

    D3D12_FEATURE_DATA_FORMAT_INFO fi{desc[k].Format,0};
    if(FAILED(dev->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO,&fi,sizeof fi))||fi.PlaneCount!=1){rs[k]=nullptr;continue;}
   }
   UINT64 linha{};dev->GetCopyableFootprints(&desc[k],sub[k],1,0,&fp[k],&linhas[k],&linha,&total[k]);fp[k].Offset=0;if(!total[k])rs[k]=nullptr;
  }
  if(!rs[0]&&!rs[1])return false;
  if(modo_reserva){

   const unsigned n=std::min(despejo_final.load(),final_maximo_()-final_indice);
   if(!reserva_gravando&&!reserva_serve(reserva,rs,total)){if(reserva_voo.load())return false;soltar_no_fundo(reserva);}
   if(reserva.empty()||(!reserva_gravando&&reserva.size()<n)){lancar_reserva(dev.Get(),total,rs,n-unsigned((std::min)(reserva.size(),size_t(n))));return false;}
   reserva_gravando=true;
  }else if(reserva.empty()){

   const unsigned n=std::min(despejo_final.load(),final_maximo_()-final_indice);
   const DWORD chamadora=GetCurrentThreadId();bool lado[lados_final]{};for(unsigned k=0;k<lados_final;++k)lado[k]=rs[k]!=nullptr;UINT64 bytes=0;
   for(unsigned j=0;j<n;++j)reserva.push_back(alocar_lote(dev.Get(),total,lado,chamadora,bytes));
   return false;
  }
  Final f;f.modo=modo;f.quadro=quadro;f.indice=final_indice;f.idade=modo?-1:idade_composta.load();f.b=std::move(reserva.back());reserva.pop_back();
  f.exato=exato;f.extras=extras;f.seguro=final_seguro_ini();
  if(exato){LARGE_INTEGER t{},q{};QueryPerformanceCounter(&t);QueryPerformanceFrequency(&q);f.qpc_ms=q.QuadPart?double(t.QuadPart)*1000.0/double(q.QuadPart):0.0;}
  for(unsigned k=0;k<lados_final;++k){
   if(!rs[k]||!f.b.leitura[k]||f.b.tamanho[k]<total[k]){f.b.leitura[k].Reset();continue;}
   const auto e=state(est[k]);if(e==D3D12_RESOURCE_STATES(~0u)){f.b.leitura[k].Reset();continue;}
   f.fp[k]=fp[k];f.altura[k]=linhas[k];f.formato[k]=unsigned(desc[k].Format);
   transition(c,rs[k],e,D3D12_RESOURCE_STATE_COPY_SOURCE);
   D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=f.b.leitura[k].Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=f.fp[k];
   src.pResource=rs[k];src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.SubresourceIndex=sub[k];
   c->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
   transition(c,rs[k],D3D12_RESOURCE_STATE_COPY_SOURCE,e);
  }
  if(f.seguro&&f.indice<final_teto_max){
   ComPtr<ID3D12GraphicsCommandList2> c2;
   if(auto* fm=final_marcas(dev.Get());fm&&SUCCEEDED(c->QueryInterface(IID_PPV_ARGS(&c2)))&&c2){
    f.valor=0xF1A10000u|(f.indice+1u);fm->m[f.indice]=0u;
    const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER par{fm->buf->GetGPUVirtualAddress()+4u*f.indice,f.valor};
    const D3D12_WRITEBUFFERIMMEDIATE_MODE modo_m=D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT;
    c2->WriteBufferImmediate(1,&par,&modo_m);f.marca=fm->m+f.indice;
   }
  }
  ++final_indice;final_total.store(final_indice);finais.push_back(std::move(f));return true;
 }catch(...){return false;}
}
void despejo_final_tick() noexcept{
 try{
  {static bool lido=false;if(!lido){lido=true;double v{};
   gravar_aviso.store(ler_ini(L"NeuralGravarAviso",v)&&v==1.);v=0.;gravar_f9.store(ler_ini(L"NeuralGravarF9",v)&&v==1.);
   {double q=0.,t=0.;const bool tq=ler_ini(L"NeuralGravarF9Quadros",q),tt=ler_ini(L"NeuralGravarTeto",t);
    const unsigned n=gravar_quadros_de(tq,q);gravar_quadros.store(n);final_teto_sessao.store(gravar_teto_de(n,tt,t));
    if(tq||tt){char l[96];NRLOGF(l,sizeof l,"config quadros=%u teto=%u",n,final_teto_sessao.load());log_reserva(l);}}
   if(const unsigned m=final_reserva_modo()){char t[96];NRLOGF(t,sizeof t,"config modo=%u gravar_f9=%d",m,gravar_f9.load()?1:0);log_reserva(t);}}}
  std::vector<Final> prontos;
  {std::lock_guard<std::mutex> trava(final_mutex);
   for(auto it=finais.begin();it!=finais.end();){if(--it->espera==0){prontos.push_back(std::move(*it));it=finais.erase(it);}else ++it;}
   if(!despejo_final.load())reserva_gravando=false;
   if(finais.empty()&&!despejo_final.load()){
    const unsigned m=reserva_modo_efetivo();
    if(m==0u)reserva.clear();
    else if(m==1u)soltar_no_fundo(reserva);
    else if(!reserva_voo.load()&&final_indice<final_maximo_()&&(gravar_f9.load()||final_indice>0)&&
            reserva.size()<std::min(gravar_quadros.load(),final_maximo_()-final_indice))final_preparo_pedido.store(true);
   }}
  for(auto& f:prontos)std::thread([g=std::move(f)]()mutable{try{escrever_final(std::move(g));}catch(...){}}).detach();
 }catch(...){}
}
}
#include "late_compose.inl"
#include "halo_post.inl"
#include "q_post.inl"
#include "rosto_jogador_d3d.inl"
#include "cena_npc.inl"
