#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "nr_publico.h"
#include <windows.h>
#include "multipass_runtime.h"
#include "pre_scale_state.h"
#include "tone_control.h"
#include "native_ini_path.h"
#include "ini_jogo.h"
#ifndef NRMP_CPU_TEST
#include "reg_swin32.h"
#include "style_trace.h"
#include "measurement_live.h"
#include "hip_priority.h"
#include "../source/implementation/live-transition-binding/job_timeline.h"
#endif
#include "../source/implementation/live-transition-binding/mode_state.h"
#include "../source/implementation/live-transition-binding/late_state.h"
#include "multipass_math.h"
#include "cena_mascara.h"
#include "reuso_state.h"
#include "rede_cada.h"
#include "painel_rede.h"

#include "rosto_jogador_state.h"
#include "rosto_jogador_detecta.h"
#include "yunet.h"
#include "scrfd.h"
#include <condition_variable>
#include <deque>
#include <map>
#include <thread>
#include <algorithm>
#include <cmath>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <exception>
#include <mutex>
#include <string>
#include <vector>
#include <cstdio>

namespace nrmptiming {
namespace {
struct Recorder {Snapshot snapshot;std::size_t remaining{};};
thread_local Recorder recorder;
}
void arm_current_thread(std::size_t limit) noexcept {recorder.snapshot={};recorder.remaining=(std::min)(limit,capacity);}
Snapshot snapshot_current_thread() noexcept {return recorder.snapshot;}
#ifdef NRMP_CPU_TEST
std::int64_t test_now() noexcept;
#endif
std::int64_t now() noexcept {
#ifdef NRMP_CPU_TEST
    return test_now();
#else
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}
struct Scope {
    Sample sample;bool active=false;std::int64_t begin{};int exceptions{};
    Scope(int width,int height,int input_pitch,int output_pitch,int input_format,int output_format,int tone,void* metadata) noexcept {
        if(!recorder.remaining)return;
        --recorder.remaining;active=true;exceptions=std::uncaught_exceptions();begin=now();
        sample.width=width;sample.height=height;sample.input_pitch=input_pitch;sample.output_pitch=output_pitch;
        sample.input_format=input_format;sample.output_format=output_format;sample.tone=tone;sample.metadata_present=metadata!=nullptr;
    }
    ~Scope() noexcept {
        if(!active)return;
        sample.wrapper_wall_ns=now()-begin;
        sample.remainder_host_wall_ns=sample.wrapper_wall_ns-sample.primary_blocking_wall_ns;
        sample.wrapper_threw=std::uncaught_exceptions()>exceptions;
        if(recorder.snapshot.count<capacity)recorder.snapshot.samples[recorder.snapshot.count++]=sample;
    }
    struct Primary {
        Scope& scope;std::int64_t begin{};int exceptions{};
        explicit Primary(Scope& value) noexcept:scope(value) {
            if(!scope.active)return;
            scope.sample.route=Route::direct_primary;exceptions=std::uncaught_exceptions();begin=now();
        }
        ~Primary() noexcept {
            if(!scope.active)return;
            scope.sample.primary_blocking_wall_ns=now()-begin;
            scope.sample.primary_returned=std::uncaught_exceptions()==exceptions;
        }
    };
};
}

namespace {
using Eval=void(*)(void*,void*,int,int,int,int,void*,int,int,int,void*);
#ifdef NRMP_CPU_TEST
void* test_symbol(const char*);
void* test_native(size_t);
#endif
template<class T>T native(char* base,size_t rva) {
#ifdef NRMP_CPU_TEST
    (void)base;return reinterpret_cast<T>(test_native(rva));
#else
    return reinterpret_cast<T>(base+rva);
#endif
}
template<class T>T read(const void* p,size_t offset) {T v;std::memcpy(&v,static_cast<const char*>(p)+offset,sizeof(v));return v;}
template<class T>void write(void* p,size_t offset,T v) {std::memcpy(static_cast<char*>(p)+offset,&v,sizeof(v));}
struct HipFailure:std::runtime_error {
    int status;const char* phase;unsigned pass;bool uncertain;
    HipFailure(int value,const char* where,unsigned index,bool pending):std::runtime_error("multipass HIP failure"),status(value),phase(where),pass(index),uncertain(pending){}
};
void check(int status) {if(status)throw HipFailure(status,"HIP operation",0,false);}

thread_local bool job_assincrono=false;
struct PortaoMovimento {bool ligado{};float t0{.01f},t1{.04f};unsigned raio{6},erosao{2};};
#ifdef NRMP_CPU_TEST
PortaoMovimento portao_teste{};
#endif
void diagnostic_escreve(const char* phase,unsigned pass,int status,double milliseconds,unsigned request,unsigned long long completed_jobs) noexcept {
#ifndef NRMP_CPU_TEST
    try {
        HMODULE self{};wchar_t path[32768]{};
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&diagnostic_escreve),&self)||!GetModuleFileNameW(self,path,32768))return;

std::ofstream log(std::filesystem::path(path).parent_path()/L"nr_multipass.log",std::ios::app);
        log<<"pid="<<GetCurrentProcessId()<<" tick="<<GetTickCount64()<<" phase="<<phase<<" pass="<<pass
           <<" hip_status="<<status<<" elapsed_ms="<<milliseconds<<" request="<<request<<" completed_jobs="<<completed_jobs<<'\n';

    }catch(...){}
#else
    (void)phase;(void)pass;(void)status;(void)milliseconds;(void)request;(void)completed_jobs;
#endif
}
void diagnostic(const char* phase,unsigned pass,int status,double milliseconds,unsigned request=0,unsigned long long completed_jobs=0) noexcept {

    static std::atomic<unsigned> records{0};if(records.fetch_add(1)>=128)return;
    diagnostic_escreve(phase,pass,status,milliseconds,request,completed_jobs);
}

namespace nrlento {
using relogio=std::chrono::steady_clock;
thread_local relogio::time_point t0{};thread_local double f[6]{};thread_local double pesos=0.;thread_local bool multi=false;
inline double agora_ms() noexcept {return std::chrono::duration<double,std::milli>(relogio::now()-t0).count();}
inline void inicio() noexcept {t0=relogio::now();for(auto& v:f)v=-1.;pesos=0.;multi=false;}
inline void marca(int i) noexcept {if(i>=0&&i<6)f[i]=agora_ms();}
inline void registra(int job,double total,unsigned passes,unsigned request,bool desligou) noexcept {
    static std::atomic<unsigned> linhas{0};if(linhas.fetch_add(1)>=64)return;
    char l[360];std::snprintf(l,sizeof l,"lento job=%d total=%.1f primario=%.1f espera_p1=%.1f pesos=%.1f extra=%.1f fold=%.1f status=%.1f trava_desliga=%d",
        job,total,f[0],f[1],pesos,f[2],f[3],f[4],desligou?1:0);
    diagnostic_escreve(l,passes,0,total,request,(unsigned long long)(job>0?job:0));
}
}

nrcena::Trava trava_sessao{};std::atomic<bool> trava_desligada{false};
struct SlowPhase {
    const char* phase;unsigned pass;std::chrono::steady_clock::time_point begin=std::chrono::steady_clock::now();
    ~SlowPhase(){const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();if(ms>100)diagnostic(phase,pass,0,ms);}
};
struct Hip {
    int (*allocate)(void**,size_t){};
    int (*release)(void*){};
    int (*copy)(void*,const void*,size_t,int){};
    int (*sync)(){};
    int (*error)(){};
    int (*device)(int*){};
    int (*set)(void*,int,size_t){};
    int (*load_module)(void**,const char*){};
    int (*get_kernel)(void**,void*,const char*){};
    int (*launch)(void*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,void*,void**,void**){};

    int (*copy_async)(void*,const void*,size_t,int,void*){};
    template<class T>static T symbol(HMODULE h,const char* n) {
#ifdef NRMP_CPU_TEST
        (void)h;return reinterpret_cast<T>(test_symbol(n));
#else
        auto p=GetProcAddress(h,n);if(!p)throw std::runtime_error("missing HIP API");return reinterpret_cast<T>(p);
#endif
    }
    Hip() {
        auto h=GetModuleHandleW(L"amdhip64_7.dll");
#ifndef NRMP_CPU_TEST
        if(!h)throw std::runtime_error("HIP unavailable");
#endif
        allocate=symbol<decltype(allocate)>(h,"hipMalloc");release=symbol<decltype(release)>(h,"hipFree");
#ifndef NRMP_CPU_TEST
        if(nrorc::modo()>0){allocate=&nrorc_vram::hip_malloc_contado;release=&nrorc_vram::hip_free_contado;}
#endif
        copy=symbol<decltype(copy)>(h,"hipMemcpy");sync=symbol<decltype(sync)>(h,"hipDeviceSynchronize");
        error=symbol<decltype(error)>(h,"hipGetLastError");device=symbol<decltype(device)>(h,"hipGetDevice");
        set=symbol<decltype(set)>(h,"hipMemset");
        load_module=symbol<decltype(load_module)>(h,"hipModuleLoad");
        get_kernel=symbol<decltype(get_kernel)>(h,"hipModuleGetFunction");
        launch=symbol<decltype(launch)>(h,"hipModuleLaunchKernel");
#ifndef NRMP_CPU_TEST
        copy_async=reinterpret_cast<decltype(copy_async)>(GetProcAddress(h,"hipMemcpyAsync"));
#endif
    }

    static inline bool async_on=false;
    int d2d(void* dst,const void* src,size_t n){return async_on&&copy_async?copy_async(dst,src,n,3,nullptr):copy(dst,src,n,3);}
    void completed(const char* phase="completion",unsigned pass=0){
        const int synced=sync();if(synced)throw HipFailure(synced,phase,pass,true);
        const int pending=error();if(pending)throw HipFailure(pending,phase,pass,true);
    }
};
}

namespace nrmpsessao {inline std::atomic<unsigned long long> t0{0};
inline void carimba() noexcept {unsigned long long z=0;if(!t0.load(std::memory_order_relaxed))t0.compare_exchange_strong(z,GetTickCount64());}
inline unsigned long long inicio(unsigned long long falta) noexcept {const unsigned long long v=t0.load(std::memory_order_relaxed);return v?v:falta;}}
static unsigned extra_passes(Eval eval,char* base,void* primary,int height,int width,void* destination,
    int output_pitch,int output_format,void* metadata,unsigned request,float* controls,bool publish) noexcept(false);
namespace {
#ifndef NRMP_CPU_TEST
#include "pre_scale_runtime.h"
#endif

PortaoMovimento portao(char* base) {
#ifdef NRMP_CPU_TEST
    (void)base;return portao_teste;
#else
    static const PortaoMovimento lido=[base]{
        PortaoMovimento p{};p.ligado=true;
        const std::wstring ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&job_assincrono));
        if(ini.empty())return p;
        auto real=[&](const wchar_t* k,float d){double v{};return nrinijogo::ler_double_em(ini.c_str(),k,v,32)?float(v):d;};
        p.ligado=NRCFG_INT(L"DlssNrOnAmd",L"NeuralPassMotionGate",1,ini.c_str())!=0;
        p.t0=std::clamp(real(L"NeuralPassMotionLow",p.t0),0.f,1.f);p.t1=std::clamp(real(L"NeuralPassMotionHigh",p.t1),p.t0,1.f);
        p.raio=unsigned(std::clamp<int>(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralPassMotionRadius",6,ini.c_str())),0,8));
        p.erosao=unsigned(std::clamp<int>(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralPassMotionErode",2,ini.c_str())),1,3));
        return p;}();
    return lido;
#endif
}

const nrcena::Config& cena_config(char* base) {
#ifdef NRMP_CPU_TEST
    (void)base;static const nrcena::Config c{};return c;
#else
    static const nrcena::Config lido=[base]{
        nrcena::Config c{};
        const std::wstring ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&job_assincrono));
        if(ini.empty())return c;
        auto real=[&](const wchar_t* k,float d){double v{};return nrinijogo::ler_double_em(ini.c_str(),k,v,32)?float(v):d;};
        auto inteiro=[&](const wchar_t* k,int d){return int(NRCFG_INT(L"DlssNrOnAmd",k,d,ini.c_str()));};
        c.modo=inteiro(L"NeuralPassoExtraCena",0);c.forca=real(L"NeuralPassoExtraCenaForca",c.forca);
        c.t0=real(L"NeuralPassoExtraCenaLimiar0",c.t0);c.t1=real(L"NeuralPassoExtraCenaLimiar1",c.t1);
        c.rel=real(L"NeuralPassoExtraCenaRelativo",c.rel);c.base_px=real(L"NeuralPassoExtraCenaBasePx",c.base_px);
        c.raio=inteiro(L"NeuralPassoExtraCenaRaio",c.raio);c.borra=inteiro(L"NeuralPassoExtraCenaBorra",c.borra);
        c.decai=real(L"NeuralPassoExtraCenaDecai",c.decai);c.passo=inteiro(L"NeuralPassoExtraCenaPasso",c.passo);
        c.iter=inteiro(L"NeuralPassoExtraCenaIter",c.iter);c.c=real(L"NeuralPassoExtraCenaEscala",c.c);
        c.ganho=real(L"NeuralPassoExtraGanho",c.ganho);
        c.escala=inteiro(L"NeuralPassoExtraEscala",c.escala);
        c.perto_k0=real(L"NeuralPassoExtraCenaPerto0",c.perto_k0);c.perto_k1=real(L"NeuralPassoExtraCenaPerto1",c.perto_k1);
        {double v{};if(nrinijogo::ler_double_em(ini.c_str(),L"NeuralPassoExtraPele",v,32)){c.pele_ligada=true;c.pele=float(v);}}

        c.parado=real(L"NeuralPassoExtraParado",c.parado);c.cada=inteiro(L"NeuralPassoExtraCada",c.cada);
        c.parado_max=inteiro(L"NeuralPassoExtraParadoMax",c.parado_max);c.reusa=inteiro(L"NeuralPassoExtraReusa",c.reusa);
        c.guia=real(L"NeuralPassoExtraGuia",c.guia);

        c.cam_px=real(L"NeuralPassoExtraCamPx",c.cam_px);c.cam_frac=real(L"NeuralPassoExtraCamFrac",c.cam_frac);
        c.cam_reusa=inteiro(L"NeuralPassoExtraCamReusa",c.cam_reusa);

        c.warp=inteiro(L"NeuralPassoExtraWarp",c.warp);c.warp_fator=real(L"NeuralPassoExtraWarpFator",c.warp_fator);
        c.crono=inteiro(L"NeuralPassoExtraCrono",c.crono);c.dilata_sep=inteiro(L"NeuralPassoExtraDilataSep",c.dilata_sep);c.mascara_pulo=inteiro(L"NeuralPassoExtraMascaraPulo",c.mascara_pulo);c.copia_async=inteiro(L"NeuralPassoExtraCopiaAsync",c.copia_async);c.menos_esperas=inteiro(L"NeuralPassoExtraMenosEsperas",c.menos_esperas);

        c.ret_mov=real(L"NeuralPassoExtraRetMovPx",c.ret_mov);c.ret_tudo=inteiro(L"NeuralPassoExtraRetMovTudo",c.ret_tudo);
        c.ret_idade=real(L"NeuralPassoExtraRetIdade",c.ret_idade);c.ret_gama=real(L"NeuralPassoExtraRetGama",c.ret_gama);
        c.ret_gama_mov=real(L"NeuralPassoExtraRetGamaMovPx",c.ret_gama_mov);c.ret_escuro=real(L"NeuralPassoExtraRetEscuro",c.ret_escuro);
        c.ret_raio=inteiro(L"NeuralPassoExtraRetRaio",c.ret_raio);
        c.alinha=inteiro(L"NeuralPassoExtraAlinha",c.alinha);c.ret_v1=inteiro(L"NeuralPassoExtraRetV1",c.ret_v1);
        c.fade=inteiro(L"NeuralPassoExtraFade",c.fade);c.fade_sobe=inteiro(L"NeuralPassoExtraFadeSobe",c.fade_sobe);
        c.protege=inteiro(L"NeuralPassoExtraProtege",c.protege);c.protege_sessao=inteiro(L"NeuralPassoExtraProtegeSessao",c.protege_sessao);c.protege_ini_s=inteiro(L"NeuralPassoExtraProtegeInicioS",c.protege_ini_s);c.protege_frac=real(L"NeuralPassoExtraProtegeFrac",c.protege_frac);c.fade_curva=inteiro(L"NeuralPassoExtraFadeCurva",c.fade_curva);

        c.comp=real(L"NeuralPassoExtraCompensa",c.comp);c.comp_max=real(L"NeuralPassoExtraCompensaMax",c.comp_max);
        c.comp_ini=real(L"NeuralPassoExtraCompensaIni",c.comp_ini);c.comp_mascara=inteiro(L"NeuralPassoExtraCompensaMascara",c.comp_mascara);
        c.comp_escuro=real(L"NeuralPassoExtraCompensaEscuro",c.comp_escuro);c.comp_media=real(L"NeuralPassoExtraCompensaMedia",c.comp_media);
        c.comp_baixa=inteiro(L"NeuralPassoExtraCompensaBaixa",c.comp_baixa);c.comp_baixa_k=real(L"NeuralPassoExtraCompensaBaixaGanho",c.comp_baixa_k);

        c.pre_carrega=inteiro(L"NeuralPassoExtraPreCarrega",c.pre_carrega);c.trava_ms=inteiro(L"NeuralPassoExtraTravaMs",c.trava_ms);
        c.trava_n=inteiro(L"NeuralPassoExtraTravaN",c.trava_n);c.trava_janela_s=inteiro(L"NeuralPassoExtraTravaJanelaS",c.trava_janela_s);

        c.cont=inteiro(L"NeuralPassoExtraContinuo",c.cont);c.cont_escala=inteiro(L"NeuralPassoExtraContinuoEscala",c.cont_escala);
        c.cont_raio=real(L"NeuralPassoExtraContinuoRaio",c.cont_raio);c.cont_forca=real(L"NeuralPassoExtraContinuoForca",c.cont_forca);
        c.cont_portao=inteiro(L"NeuralPassoExtraContinuoPortao",c.cont_portao);c.cont_mov=real(L"NeuralPassoExtraContinuoMovPx",c.cont_mov);
        c.cont_mascara=inteiro(L"NeuralPassoExtraContinuoMascaraCada",c.cont_mascara);
        c.sobe=inteiro(L"NeuralPassoExtraSobeGuiada",c.sobe);c.sobe_luma=real(L"NeuralPassoExtraSobeLuma",c.sobe_luma);
        c.sobe_prof=real(L"NeuralPassoExtraSobeProf",c.sobe_prof);c.sobe_raio=real(L"NeuralPassoExtraSobeRaio",c.sobe_raio);
        c=nrcena::limitar(c);

        nrcena::quer_mv.store(c.modo>0||c.warp>0||c.cam_px>0.f,std::memory_order_relaxed);
        char line[800];std::snprintf(line,sizeof line,"cena_config modo=%d perto=%.2f..%.2f forca=%.2f t0=%.2f t1=%.2f rel=%.3f base_px=%.2f raio=%d borra=%d decai=%.2f passo=%d iter=%d c=%.2f ganho=%.2f escala=%d pele=%s%.2f parado=%.3f cada=%d parado_max=%d reusa=%d guia=%.3f cam_px=%.2f cam_frac=%.2f cam_reusa=%d warp=%d warp_fator=%.2f crono=%d dilata_sep=%d mascara_pulo=%d copia_async=%d menos_esperas=%d",
            c.modo,double(c.perto_k0),double(c.perto_k1),double(c.forca),double(c.t0),double(c.t1),double(c.rel),double(c.base_px),c.raio,c.borra,double(c.decai),c.passo,c.iter,double(c.c),double(c.ganho),c.escala,c.pele_ligada?"":"(nao) ",double(c.pele),double(c.parado),c.cada,c.parado_max,c.reusa,double(c.guia),double(c.cam_px),double(c.cam_frac),c.cam_reusa,c.warp,double(c.warp_fator),c.crono,c.dilata_sep,c.mascara_pulo,c.copia_async,c.menos_esperas);
        diagnostic(line,0,0,0);
        if(c.retifica()||c.alinha>0||c.fade>0){
            std::snprintf(line,sizeof line,"ret_config v1=%d alinha=%d mov_px=%.2f tudo=%d idade=%.2f gama=%.2f gama_mov=%.2f escuro=%.2f raio=%d fade=%d sobe=%d curva=%d protege=%d/%ds/%.4f",
                c.ret_v1,c.alinha,double(c.ret_mov),c.ret_tudo,double(c.ret_idade),double(c.ret_gama),double(c.ret_gama_mov),double(c.ret_escuro),c.ret_raio,c.fade,c.fade_sobe,c.fade_curva,c.protege,c.protege_ini_s,double(c.protege_frac));
            diagnostic(line,0,0,0);
        }
        if(c.pre_carrega||c.trava_ms>0){
            std::snprintf(line,sizeof line,"trava_config pre_carrega=%d trava_ms=%d trava_n=%d janela_s=%d",c.pre_carrega,c.trava_ms,c.trava_n,c.trava_janela_s);
            diagnostic(line,0,0,0);
        }
        if(c.comp>0.f){
            std::snprintf(line,sizeof line,"comp_config comp=%.2f max=%.2f ini=%.2f mascara=%d escuro=%.2f media=%.2f baixa=%d baixa_k=%.2f",
                double(c.comp),double(c.comp_max),double(c.comp_ini),c.comp_mascara,double(c.comp_escuro),double(c.comp_media),c.comp_baixa,double(c.comp_baixa_k));
            diagnostic(line,0,0,0);
        }
        if(c.cont>0||c.sobe>0){
            std::snprintf(line,sizeof line,"cont_config cont=%d valido=%d escala=%d raio=%.1f forca=%.2f portao=%d mov_px=%.1f mascara_cada=%d sobe=%d luma=%.3f prof=%.3f raio_sobe=%.2f",
                c.cont,nrcena::continuo(c)?1:0,c.cont_escala,double(c.cont_raio),double(c.cont_forca),c.cont_portao,double(c.cont_mov),c.cont_mascara,c.sobe,double(c.sobe_luma),double(c.sobe_prof),double(c.sobe_raio));
            diagnostic(line,0,0,0);
        }
        return c;}();
    return lido;
#endif
}
#pragma warning(push)
#pragma warning(disable:4324)
struct Cache {

    void* primary{};int device=-1;char* base{};bool poisoned{};std::atomic<bool> completion_uncertain{false};unsigned long long completed_jobs{};
    alignas(64) std::array<unsigned char,0x400> contexts[2]{};
    bool constructed[2]{},loaded[2]{};
    void* input{};void* output{};void* temporary{};void* rollback{};void* status{};
    size_t capacity{};
    void* module{};void* compose_kernel{};void* sanitize_kernel{};void* fold_kernel{};

    void* diff_kernel{};void* erode_kernel{};void* gated_kernel{};bool gate_kernels_tried{};
    void* previous{};void* diff{};void* eroded{};void* gate_stats{};unsigned gate_w{},gate_h{};bool gate_valid{};
    unsigned long long gate_jobs{},gate_sum{},gate_pixels{};

    unsigned gate_fail_w{},gate_fail_h{};bool gate_fail_logged{};
    std::mutex mutex;
    bool gate_alloc_failed(Hip& hip,unsigned w,unsigned h,int erro) noexcept {
        (void)hip.error();
        for(void** p:{&previous,&diff,&eroded})if(*p){(void)hip.release(*p);*p=nullptr;}
        gate_w=gate_h=0;gate_valid=false;gate_fail_w=w;gate_fail_h=h;
        if(!gate_fail_logged){gate_fail_logged=true;diagnostic("motion_gate_alloc_failed",0,erro,0,w,(unsigned long long)h);}
        return false;
    }
    bool ensure_gate(Hip& hip,unsigned w,unsigned h) {
        if(!gate_kernels_tried){
            gate_kernels_tried=true;
            if(hip.get_kernel(&diff_kernel,module,"nrmp_motion_diff")||hip.get_kernel(&erode_kernel,module,"nrmp_motion_erode")||
               hip.get_kernel(&gated_kernel,module,"nrmp_fold_gated")){diff_kernel=erode_kernel=gated_kernel=nullptr;(void)hip.error();}
        }
        if(!gated_kernel)return false;
        if(gate_w!=w||gate_h!=h||!previous){
            if(gate_fail_w==w&&gate_fail_h==h)return false;
            void* fresh[3]{};const size_t px=size_t(w)*h;const size_t bytes[3]={px*12,px*4,px*4};
            for(unsigned i=0;i<3;++i)if(const int r=hip.allocate(&fresh[i],bytes[i])){
                fresh[i]=nullptr;for(auto p:fresh)if(p)(void)hip.release(p);
                return gate_alloc_failed(hip,w,h,r);
            }
            void* prior[3]={previous,diff,eroded};
            previous=fresh[0];diff=fresh[1];eroded=fresh[2];gate_w=w;gate_h=h;gate_valid=false;gate_fail_w=gate_fail_h=0;
            int failed=0;for(auto p:prior)if(p){const int r=hip.release(p);if(r)failed=r;}check(failed);
        }
        if(!gate_stats)if(const int r=hip.allocate(&gate_stats,4)){gate_stats=nullptr;return gate_alloc_failed(hip,w,h,r);}
        return true;
    }
    void ensure_buffers(Hip& hip,size_t bytes) {
        if(capacity>=bytes)return;
        void* fresh[4]{};
        try {for(auto& p:fresh)check(hip.allocate(&p,bytes));}
        catch(...) {for(auto p:fresh)if(p)hip.release(p);throw;}
        void* prior[4]={input,output,temporary,rollback};
        input=fresh[0];output=fresh[1];temporary=fresh[2];rollback=fresh[3];capacity=bytes;
        int failed=0;for(auto p:prior)if(p){const int r=hip.release(p);if(r)failed=r;}check(failed);
    }
    void ensure_kernels(Hip& hip) {
        if(fold_kernel && status)return;
        if(!module) {
            wchar_t path[32768];HMODULE self{};
#ifndef NRMP_CPU_TEST
            if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&NRMultipassEval_v1),&self))throw std::runtime_error("helper path unavailable");
#endif
            const auto length=GetModuleFileNameW(self,path,32768);
            if(!length || length>=32768)throw std::runtime_error("helper path unavailable");
            const auto file=(std::filesystem::path(path).parent_path()/L"nr_multipass.hsaco").string();
            check(hip.load_module(&module,file.c_str()));
        }
        check(hip.get_kernel(&compose_kernel,module,"nrmp_compose"));
        check(hip.get_kernel(&sanitize_kernel,module,"nrmp_sanitize"));
        check(hip.get_kernel(&fold_kernel,module,"nrmp_fold"));
        if(!status)check(hip.allocate(&status,4));
    }
    void* context(Hip& hip,unsigned index,const float* controls) {
        auto p=contexts[index].data();
        if(!constructed[index]) {
            native<void*(*)(void*)>(base,0x1ce20)(p);constructed[index]=true;
        }
        if(!loaded[index]) {
            wchar_t path[32768];
#ifdef NRMP_CPU_TEST
            auto weight_module=HMODULE{};
#else
            auto weight_module=reinterpret_cast<HMODULE>(base);
#endif
            auto length=GetModuleFileNameW(weight_module,path,32768);
            if(!length||length>=32768)throw std::runtime_error("proxy path unavailable");
            std::string weights=(std::filesystem::path(path).parent_path()/L"dlssnr_on_amd_weights.bin").string();
            bool ok=native<bool(*)(void*,const std::string*)>(base,0x1e120)(p,&weights);
            hip.completed();
            if(!ok||!read<void*>(p,0x10)||!read<void*>(p,0x168)||!read<size_t>(p,0x160))
                throw std::runtime_error("extra context weights unavailable");
            loaded[index]=true;
        }
        std::memcpy(p+0x20,controls,20);

        write<unsigned>(p,0xf0,0);write<unsigned>(p,0x120,0);write<unsigned>(p,0x38,0);
        write<void*>(p,0x118,nullptr);
        return p;
    }
};
#pragma warning(pop)

Cache& cache() {static Cache* p=new Cache;return *p;}

struct CenaMv {const void* mv;int pitch,fmt,mvw,mvh;float sx,sy;int neth,netw;const void* dep;int dpitch,dfmt,dw,dh,dinv;};
static_assert(sizeof(CenaMv)==72,"CenaMv 72 B");
struct CenaGpu {
    bool tentou{},pronto{};void* k_reduz{};void* k_fold_baixo{};void* l0{};void* la{};void* lb{};size_t lcap{};void* module{};void* k_dmax{};void* k_acum{};void* k_resolve{};void* k_bruta{};void* k_filtro{};void* k_persiste{};void* k_fold{};
    void* dmax{};void* acc{};void* coef{};void* stats{};void* a{};void* b{};void* prev{};size_t cap{};int pw{},ph{};bool prev_ok{};
    unsigned long long q_ant{};unsigned long long jobs{},sem_mv{},mascaras{},invalidos{};double soma_w{},soma_m{},px{};float coef_host[20]{};

    void* k_diff_ancora{};void* k_conta{};void* k_fold_baixo2{};void* cnt{};void* ancora{};size_t anc_cap{};
    bool delta_ok{};unsigned delta_lw{},delta_lh{};int sem{};
    unsigned long long p_calc{},p_pul{},p_reus{},p_jobs{},p_hist[7]{};double p_frac{};

    void* k_mv_conta{};void* cnt2{};unsigned long long p_cam{};double p_fcam{};

    unsigned cnt_pend{};void* k_diff_warp{};void* k_fold_warp{};unsigned long long anc_q{},p_warp{},levadas{};double p_nq{};
    double cr_ms[2][6]{};unsigned long long cr_n[2]{};

    bool ler_mv(CenaMv& m) const noexcept {
        if(!nrcena::params_ok)return false;
        const unsigned char* p=nrcena::params;
        std::memcpy(&m.mv,p+0x10,8);std::memcpy(&m.pitch,p+0x18,4);std::memcpy(&m.fmt,p+0x1c,4);std::memcpy(&m.mvw,p+0x20,4);std::memcpy(&m.mvh,p+0x24,4);
        std::memcpy(&m.sx,p+0x58,4);std::memcpy(&m.sy,p+0x5c,4);std::memcpy(&m.neth,p+0x68,4);std::memcpy(&m.netw,p+0x6c,4);
        std::memcpy(&m.dep,p+0x30,8);std::memcpy(&m.dpitch,p+0x38,4);std::memcpy(&m.dfmt,p+0x3c,4);std::memcpy(&m.dw,p+0x40,4);std::memcpy(&m.dh,p+0x44,4);std::memcpy(&m.dinv,p+0x48,4);
        if(m.dep&&(m.dw<=0||m.dh<=0||m.dw>16384||m.dh>16384||m.dpitch<m.dw*4))m.dep=nullptr;
        const int bpp=m.fmt==1||m.fmt==2?8:4;
        return m.mv&&m.fmt>=0&&m.fmt<=2&&m.mvw>=16&&m.mvh>=16&&m.mvw<=8192&&m.mvh<=8192&&m.pitch>=m.mvw*bpp&&m.netw>0&&m.neth>0&&std::isfinite(m.sx)&&std::isfinite(m.sy);
    }

    void* k_fold_ret{};void* k_ret_delta{};void* k_ret_baixa{};void* k_fold_ret2{};void* ret_dl{};void* ret_dlb{};size_t ret_cap{};CenaMv m_ult{};bool m_ok{};unsigned long long q_ret_ant{},q_ancora{};int entre_ret=2;
    unsigned long long r_jobs{};double r_t{},r_k{},r_px{},r_idade{};

    float fade_v=1.f;unsigned long long q_fade{},p_fade{};

    unsigned long long t_ini{},n_prot{};bool prot_ant=false;unsigned long long jobs_log=~0ull;

    void* k_cp_mede{};void* k_cp_aplica{};void* cp_soma{};float cp_r{};bool cp_mediu{};unsigned long long cp_jobs{},cp_comp{},cp_med{};double cp_g{};

    void* k_cont_borra{};void* k_fold_cont{};void* cont_tmp{};size_t cont_cap{};
    unsigned long long c_jobs{},c_cont{},c_andando{},c_barato{},c_guiado{};double c_kd{},c_lw{};unsigned c_masc{};
    bool cont_tamanho(Hip& hip,unsigned lw,unsigned lh) {
        const size_t need=size_t(lw)*lh*16;if(need<=cont_cap&&cont_tmp)return true;
        void* n{};if(hip.allocate(&n,need)){(void)hip.error();return false;}
        if(cont_tmp)(void)hip.release(cont_tmp);cont_tmp=n;cont_cap=need;return true;
    }
    bool carregar(Hip& hip) {
        if(tentou)return pronto;tentou=true;
#ifndef NRMP_CPU_TEST
        wchar_t path[32768];HMODULE self{};
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&NRMultipassEval_v1),&self))return false;
        const auto length=GetModuleFileNameW(self,path,32768);if(!length||length>=32768)return false;
        const auto file=std::filesystem::path(path).parent_path()/L"nr_cena.hsaco";std::error_code ec;
        if(!std::filesystem::is_regular_file(file,ec)){diagnostic("cena_modulo_ausente",0,0,0);return false;}
        if(hip.load_module(&module,file.string().c_str())){(void)hip.error();diagnostic("cena_modulo_recusado",0,0,0);return false;}
        const char* nomes[]={"cn_dmax","cn_acum","cn_resolve","cn_bruta","cn_filtro","cn_persiste","cn_fold"};
        void** alvos[]={&k_dmax,&k_acum,&k_resolve,&k_bruta,&k_filtro,&k_persiste,&k_fold};
        for(int i=0;i<7;++i)if(hip.get_kernel(alvos[i],module,nomes[i])){(void)hip.error();diagnostic("cena_kernel_ausente",unsigned(i),0,0);return false;}
        if(hip.get_kernel(&k_reduz,module,"cn_reduz")||hip.get_kernel(&k_fold_baixo,module,"cn_fold_baixo")){(void)hip.error();k_reduz=k_fold_baixo=nullptr;}
        if(hip.get_kernel(&k_diff_ancora,module,"cn_diff_ancora")||hip.get_kernel(&k_conta,module,"cn_conta")||hip.get_kernel(&k_fold_baixo2,module,"cn_fold_baixo2")){
            (void)hip.error();k_diff_ancora=k_conta=k_fold_baixo2=nullptr;diagnostic("cena_ghost2_ausente",0,0,0);}
        if(hip.get_kernel(&k_fold_ret,module,"cn_fold_ret")){(void)hip.error();k_fold_ret=nullptr;diagnostic("cena_ret_ausente",0,0,0);}
        if(hip.get_kernel(&k_ret_delta,module,"cn_ret_delta")||hip.get_kernel(&k_ret_baixa,module,"cn_ret_baixa")||hip.get_kernel(&k_fold_ret2,module,"cn_fold_ret2")){
            (void)hip.error();k_ret_delta=k_ret_baixa=k_fold_ret2=nullptr;diagnostic("cena_ret2_ausente",0,0,0);}
        else if(hip.allocate(&cnt,4)){(void)hip.error();cnt=nullptr;k_diff_ancora=k_conta=k_fold_baixo2=nullptr;diagnostic("cena_ghost2_alloc",0,0,0);}
        if(hip.get_kernel(&k_cp_mede,module,"cp_mede")||hip.get_kernel(&k_cp_aplica,module,"cp_aplica")){(void)hip.error();k_cp_mede=k_cp_aplica=nullptr;diagnostic("cena_comp_ausente",0,0,0);}
        else if(hip.allocate(&cp_soma,8)){(void)hip.error();cp_soma=nullptr;k_cp_mede=k_cp_aplica=nullptr;diagnostic("cena_comp_alloc",0,0,0);}
        if(hip.get_kernel(&k_cont_borra,module,"cn_cont_borra")||hip.get_kernel(&k_fold_cont,module,"cn_fold_cont")){(void)hip.error();k_cont_borra=k_fold_cont=nullptr;diagnostic("cena_cont_ausente",0,0,0);}
        if(hip.get_kernel(&k_mv_conta,module,"cn_mv_conta")){(void)hip.error();k_mv_conta=nullptr;diagnostic("cena_camera_ausente",0,0,0);}
        else if(hip.allocate(&cnt2,8)){(void)hip.error();cnt2=nullptr;k_mv_conta=nullptr;diagnostic("cena_camera_alloc",0,0,0);}
        if(hip.get_kernel(&k_diff_warp,module,"cn_diff_ancora_warp")||hip.get_kernel(&k_fold_warp,module,"cn_fold_baixo_warp")){
            (void)hip.error();k_diff_warp=k_fold_warp=nullptr;diagnostic("cena_warp_ausente",0,0,0);}
        void** bufs[]={&dmax,&acc,&coef,&stats};const size_t tam[]={64,65*4,20*4,64};
        for(int i=0;i<4;++i)if(hip.allocate(bufs[i],tam[i])){(void)hip.error();*bufs[i]=nullptr;diagnostic("cena_alloc",unsigned(i),0,0);return false;}
        pronto=true;diagnostic("cena_pronto",0,0,0);
#else
        (void)hip;
#endif
        return pronto;
    }
    bool tamanho(Hip& hip,int w,int h) {
        const size_t need=size_t(w)*size_t(h)*4;
        if(need<=cap&&w==pw&&h==ph)return true;
        void* novos[3]{};
        for(int i=0;i<3;++i)if(hip.allocate(&novos[i],need)){(void)hip.error();for(auto p:novos)if(p)(void)hip.release(p);return false;}
        for(void* p:{a,b,prev})if(p)(void)hip.release(p);
        a=novos[0];b=novos[1];prev=novos[2];cap=need;pw=w;ph=h;prev_ok=false;return true;
    }

    bool baixo_tamanho(Hip& hip,unsigned lw,unsigned lh) {
        const size_t need=size_t(lw)*lh*8;if(need<=lcap)return true;
        void* novos[3]{};
        for(int i=0;i<3;++i)if(hip.allocate(&novos[i],need)){(void)hip.error();for(auto p:novos)if(p)(void)hip.release(p);return false;}
        for(void* p:{l0,la,lb})if(p)(void)hip.release(p);
        l0=novos[0];la=novos[1];lb=novos[2];lcap=need;delta_ok=false;return true;
    }

    bool ret_tamanho(Hip& hip,unsigned lw,unsigned lh) {
        const size_t need=size_t(lw)*lh*16;if(need<=ret_cap&&ret_dl&&ret_dlb)return true;
        void* na{};void* nb{};if(hip.allocate(&na,need)){(void)hip.error();return false;}if(hip.allocate(&nb,need)){(void)hip.error();(void)hip.release(na);return false;}
        if(ret_dl)(void)hip.release(ret_dl);if(ret_dlb)(void)hip.release(ret_dlb);ret_dl=na;ret_dlb=nb;ret_cap=need;return true;
    }

    bool ancora_tamanho(Hip& hip,size_t bytes) {
        if(bytes<=anc_cap&&ancora)return true;
        void* novo{};if(hip.allocate(&novo,bytes)){(void)hip.error();return false;}
        if(ancora)(void)hip.release(ancora);
        ancora=novo;anc_cap=bytes;delta_ok=false;return true;
    }

    static bool mv_do_job(CenaMv& m) {
        if(!nrcena::params_ok)return false;
        const unsigned char* p=nrcena::params;m=CenaMv{};
        std::memcpy(&m.mv,p+0x10,8);std::memcpy(&m.pitch,p+0x18,4);std::memcpy(&m.fmt,p+0x1c,4);std::memcpy(&m.mvw,p+0x20,4);std::memcpy(&m.mvh,p+0x24,4);
        std::memcpy(&m.sx,p+0x58,4);std::memcpy(&m.sy,p+0x5c,4);std::memcpy(&m.neth,p+0x68,4);std::memcpy(&m.netw,p+0x6c,4);
        std::memcpy(&m.dep,p+0x30,8);std::memcpy(&m.dpitch,p+0x38,4);std::memcpy(&m.dfmt,p+0x3c,4);std::memcpy(&m.dw,p+0x40,4);std::memcpy(&m.dh,p+0x44,4);std::memcpy(&m.dinv,p+0x48,4);
        if(m.dep&&(m.dw<=0||m.dh<=0||m.dw>16384||m.dh>16384||m.dpitch<m.dw*4))m.dep=nullptr;
        const int bpp=m.fmt==1||m.fmt==2?8:4;
        return m.mv&&m.fmt>=0&&m.fmt<=2&&m.mvw>=16&&m.mvh>=16&&m.mvw<=8192&&m.mvh<=8192&&m.pitch>=m.mvw*bpp&&m.netw>0&&m.neth>0&&std::isfinite(m.sx)&&std::isfinite(m.sy);
    }

    float frac_camera(Hip& hip,float lim) {
        CenaMv m{};if(!k_mv_conta||!cnt2||!mv_do_job(m))return -1.f;
        int passo=8;const unsigned gw=unsigned((m.mvw+passo-1)/passo),gh=unsigned((m.mvh+passo-1)/passo);
        check(hip.set(cnt2,0,8));
        void* am[]={&m,&passo,&lim,&cnt2};check(hip.launch(k_mv_conta,(gw+255)/256u,gh,1,256,1,1,0,nullptr,am,nullptr));
        unsigned c[2]{};check(hip.copy(c,cnt2,8,2));
        return c[1]?float(double(c[0])/double(c[1])):-1.f;
    }

    const float* mascara(Hip& hip,const nrcena::Config& c,int& mw,int& mh) {
        ++jobs;mw=mh=0;m_ok=false;
        if(!nrcena::params_ok){++sem_mv;return nullptr;}
        CenaMv m{};const unsigned char* p=nrcena::params;
        std::memcpy(&m.mv,p+0x10,8);std::memcpy(&m.pitch,p+0x18,4);std::memcpy(&m.fmt,p+0x1c,4);std::memcpy(&m.mvw,p+0x20,4);std::memcpy(&m.mvh,p+0x24,4);
        std::memcpy(&m.sx,p+0x58,4);std::memcpy(&m.sy,p+0x5c,4);std::memcpy(&m.neth,p+0x68,4);std::memcpy(&m.netw,p+0x6c,4);
        std::memcpy(&m.dep,p+0x30,8);std::memcpy(&m.dpitch,p+0x38,4);std::memcpy(&m.dfmt,p+0x3c,4);std::memcpy(&m.dw,p+0x40,4);std::memcpy(&m.dh,p+0x44,4);std::memcpy(&m.dinv,p+0x48,4);
        if(m.dep&&(m.dw<=0||m.dh<=0||m.dw>16384||m.dh>16384||m.dpitch<m.dw*4))m.dep=nullptr;
        const int bpp=m.fmt==1||m.fmt==2?8:4;
        if(!(m.mv&&m.fmt>=0&&m.fmt<=2&&m.mvw>=16&&m.mvh>=16&&m.mvw<=8192&&m.mvh<=8192&&m.pitch>=m.mvw*bpp&&m.netw>0&&m.neth>0&&std::isfinite(m.sx)&&std::isfinite(m.sy))){++invalidos;return nullptr;}
        m_ult=m;m_ok=true;
        if(!tamanho(hip,m.mvw,m.mvh))return nullptr;
        int passo=c.passo,gw=(m.mvw+passo-1)/passo,gh=(m.mvh+passo-1)/passo;
        check(hip.set(dmax,0,4));
        {void* args[]={&m,&passo,&dmax};check(hip.launch(k_dmax,unsigned(gw+63)/64u,unsigned(gh),1,64,1,1,0,nullptr,args,nullptr));}
        float ridge=1e-6f,cc=c.c;
        for(int it=0;it<c.iter;++it){
            int usar=it>0?1:0;
            check(hip.set(acc,0,65*4));
            void* aa[]={&m,&passo,&dmax,&coef,&usar,&cc,&acc};check(hip.launch(k_acum,unsigned(gw+255)/256u,unsigned(gh),1,256,1,1,0,nullptr,aa,nullptr));
            void* ar[]={&acc,&ridge,&coef};check(hip.launch(k_resolve,1,1,1,1,1,1,0,nullptr,ar,nullptr));
        }
        const unsigned gx=unsigned(m.mvw+255)/256u,gy=unsigned(m.mvh);
        float base_px=c.base_px,rel=c.rel,t0=c.t0,t1=c.t1,pk0=c.perto_k0,pk1=c.modo>=2?c.perto_k1:0.f;
        {void* ab[]={&m,&dmax,&coef,&acc,&base_px,&rel,&t0,&t1,&pk0,&pk1,&a};check(hip.launch(k_bruta,gx,gy,1,256,1,1,0,nullptr,ab,nullptr));}
        auto filtro=[&](void* src,void* dst,int r,int dir,int op){int w=m.mvw,h=m.mvh;void* af[]={&src,&dst,&w,&h,&r,&dir,&op};check(hip.launch(k_filtro,gx,gy,1,256,1,1,0,nullptr,af,nullptr));};
        filtro(a,b,1,0,0);filtro(b,a,1,1,0);filtro(a,b,1,0,1);filtro(b,a,1,1,1);
        if(c.raio>0){filtro(a,b,c.raio,0,1);filtro(b,a,c.raio,1,1);}
        {const unsigned long long q=nrreusostate::quadro_cena;int k=nrcena::quadros_entre(q,q_ant);q_ant=q;int valido=prev_ok&&c.decai>0.f?1:0;float d=c.decai;
         void* ap[]={&m,&a,&prev,&valido,&d,&k};check(hip.launch(k_persiste,gx,gy,1,256,1,1,0,nullptr,ap,nullptr));}
        check(hip.d2d(prev,a,size_t(m.mvw)*m.mvh*4));prev_ok=true;
        if(c.borra>0){filtro(a,b,c.borra,0,2);filtro(b,a,c.borra,1,2);}
        ++mascaras;mw=m.mvw;mh=m.mvh;return static_cast<const float*>(a);
    }

    const float* mascara_levada(Hip& hip,const nrcena::Config& c,int& mw,int& mh) {
        mw=mh=0;CenaMv m{};
        if(!prev_ok||!ler_mv(m)||m.mvw!=pw||m.mvh!=ph)return nullptr;
        ++jobs;
        const unsigned gx=unsigned(m.mvw+255)/256u,gy=unsigned(m.mvh);
        check(hip.set(a,0,size_t(m.mvw)*m.mvh*4));
        {const unsigned long long q=nrreusostate::quadro_cena;int k=nrcena::quadros_entre(q,q_ant);q_ant=q;int valido=1;float d=1.f;
         void* ap[]={&m,&a,&prev,&valido,&d,&k};check(hip.launch(k_persiste,gx,gy,1,256,1,1,0,nullptr,ap,nullptr));}
        check(hip.d2d(prev,a,size_t(m.mvw)*m.mvh*4));
        auto filtro=[&](void* src,void* dst,int r,int dir,int op){int w=m.mvw,h=m.mvh;void* af[]={&src,&dst,&w,&h,&r,&dir,&op};check(hip.launch(k_filtro,gx,gy,1,256,1,1,0,nullptr,af,nullptr));};
        if(c.borra>0){filtro(a,b,c.borra,0,2);filtro(b,a,c.borra,1,2);}
        ++mascaras;++levadas;mw=m.mvw;mh=m.mvh;return static_cast<const float*>(a);
    }
};
CenaGpu& cena_gpu() {static CenaGpu* p=new CenaGpu;return *p;}
#ifndef NRMP_CPU_TEST
#include "rosto_jogador_runtime.h"
#endif
#ifndef NRMP_CPU_TEST

void cadencia_camera() noexcept {
 namespace c=nrredecada::cam;
 nrcena::quer_mv.store(true,std::memory_order_relaxed);
 int e=-1;
 try{Hip hip;auto& cg=cena_gpu();
  if(cg.carregar(hip)){const float f=cg.frac_camera(hip,c::px.load(std::memory_order_relaxed));e=f<0.f?-1:c::anda(f,c::frac.load(std::memory_order_relaxed))?1:0;}
 }catch(...){e=-1;}
 c::estado.store(e,std::memory_order_release);c::medidos.fetch_add(1,std::memory_order_relaxed);if(e>0)c::andando.fetch_add(1,std::memory_order_relaxed);
}
#endif
void acknowledge(char* base,unsigned value) {
    const LONG prior=InterlockedExchange(reinterpret_cast<volatile LONG*>(base+0x71d094),LONG(value));

    if(unsigned(prior)!=value)diagnostic("effective",value&255u,0,0,value,unsigned(prior));
}
}

#ifndef NRMP_CPU_TEST

constexpr int job1_teto_padrao=1;
static void job1_log(const char* linha) noexcept {
    try{wchar_t path[32768]{};HMODULE self{};
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&job1_log),&self)||!GetModuleFileNameW(self,path,32768))return;
        std::ofstream log(std::filesystem::path(path).parent_path()/L"nr_pre_scale.log",std::ios::app);
        log<<linha<<std::endl;}catch(...){}
}
extern "C" bool NRAquecerJob1_v1(void* proxy_base,int width,int height) noexcept {
    auto base=static_cast<char*>(proxy_base);
    if(!base)return false;
    volatile auto& feito=*reinterpret_cast<volatile std::uint8_t*>(base+0x948B0);
    if(feito)return false;
    static int chave=-1;
    if(chave<0){chave=job1_teto_padrao;try{const auto ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&job1_log));
        if(!ini.empty())chave=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralCenaJob1Teto",job1_teto_padrao,ini.c_str()));}catch(...){}
        if(chave!=1){char l[160];std::snprintf(l,sizeof l,"PRE_SCALE_AQUECE pid=%lu desligado NeuralCenaJob1Teto=%d (o proxy aquece no tamanho da captura)",GetCurrentProcessId(),chave);job1_log(l);}}
    if(chave!=1)return false;
    void* primary=base+0x93f48;
    const unsigned request=nrmp::valid_config(unsigned(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(base+0x71d090),0,0)));
    unsigned w{},h{};double pct{};
    try{if(!nrprescale_runtime::aquecer_tamanho(primary,width,height,request,w,h,pct)){
        static bool avisado=false;
        if(!avisado){avisado=true;char l[160];std::snprintf(l,sizeof l,"PRE_SCALE_AQUECE pid=%lu proxy captura=%dx%d (o job nao reduziria: o proxy aquece como sempre)",GetCurrentProcessId(),width,height);job1_log(l);}
        return false;}}
    catch(...){return false;}

    std::unique_lock<std::recursive_mutex> trava_rj(nrregswin::rjsync::trava(),std::defer_lock);
    if(nrregswin::rjsync::ligado.load(std::memory_order_relaxed))trava_rj.lock();
    void* in{};void* out{};bool chamou=false;const auto t0=std::chrono::steady_clock::now();int profile=0;
    try{
        Hip hip;const size_t bytes=size_t(w)*h*8;
        check(hip.allocate(&in,bytes));check(hip.allocate(&out,bytes));check(hip.set(in,0,bytes));
        feito=1;
        profile=read<int>(base,0x9487C)!=0?2:0;*reinterpret_cast<volatile int*>(static_cast<char*>(primary)+0x98)=profile;
        chamou=true;
        native<Eval>(base,0x2a080)(primary,in,int(w*8),0,int(h),int(w),out,int(w*8),0,read<int>(base,0x94698),nullptr);
        *reinterpret_cast<volatile int*>(static_cast<char*>(primary)+0x38)=0;
        *reinterpret_cast<volatile std::uint8_t*>(static_cast<char*>(primary)+0x120)=0;
        hip.completed("aquecer_job1");
        check(hip.release(in));in=nullptr;check(hip.release(out));out=nullptr;
    }catch(...){
        try{Hip hip;if(in)hip.release(in);if(out)hip.release(out);}catch(...){}
        if(!chamou)feito=0;
        char l[160];std::snprintf(l,sizeof l,"PRE_SCALE_AQUECE pid=%lu falhou captura=%dx%d neural=%ux%u eval=%d",GetCurrentProcessId(),width,height,w,h,chamou?1:0);job1_log(l);
        return chamou;
    }
    const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
    char l[200];std::snprintf(l,sizeof l,"PRE_SCALE_AQUECE pid=%lu captura=%dx%d neural=%ux%u percent=%.4g request=%u profile=%d ms=%.1f (no lugar do aquecimento do proxy na captura inteira)",
        GetCurrentProcessId(),width,height,w,h,pct,request,profile,ms);job1_log(l);
    return true;
}
#endif

extern "C" bool NRMultipassInstall_v1(void* proxy_base) noexcept {
    if(!proxy_base)return false;
    auto base=static_cast<char*>(proxy_base);

    if(read<unsigned>(base,0x71d098)!=0x3153504du)return false;
    void* empty=nullptr;
    if(!nrtone::owner.compare_exchange_strong(empty,base) && empty!=base)return false;
#ifndef NRMP_CPU_TEST
    (void)nrneuraltone::install(base);
    (void)nrregswin::install(base);
    (void)nrstyletrace::install(base);
    (void)nrmeasurelive::install(base);
    (void)nrhipprio::install(base);
#endif
    InterlockedExchangePointer(reinterpret_cast<void* volatile*>(base+0x71d0a0),
        reinterpret_cast<void*>(&NRMultipassEval_v1));
#ifndef NRMP_CPU_TEST
    nrprescale::aquecer_job1.store(&NRAquecerJob1_v1,std::memory_order_release);
#endif
    return true;
}

static unsigned extra_passes(Eval eval,char* base,void* primary,int height,int width,void* destination,
    int output_pitch,int output_format,void* metadata,unsigned request,float* controls,bool publish) noexcept(false) {

    const auto done=[&](unsigned value){if(publish)acknowledge(base,value);return value;};
    const auto extra_chain_started=std::chrono::steady_clock::now();
    bool baseline_preserved=true;
    try {
        Hip hip;hip.completed("primary_completion",1);
        const auto passo1_pronto=std::chrono::steady_clock::now();
        Hip::async_on=cena_config(base).copia_async!=0;
        const unsigned passes=request&255u;

        static const unsigned long long t_sessao_mp=GetTickCount64();
        const unsigned long long t_sessao=cena_config(base).protege_sessao?nrmpsessao::inicio(t_sessao_mp):t_sessao_mp;
        nrlento::marca(1);
        if(passes==1)return done(request);

        if(trava_desligada.load(std::memory_order_relaxed))return done((request&0xff00u)|1u);
        nrlento::multi=true;

        if(!metadata||!read<unsigned>(metadata,0x40)||output_format!=0||!destination||
            width<128||height<128||width>16384||height>16384||output_pitch<width*8||(output_pitch&1)) {
            return done((request&0xff00u)|1u);
        }
        int padded_h=read<int>(primary,0x18),padded_w=read<int>(primary,0x1c);
        void* imported=read<void*>(primary,0xf8);
        if(!imported||padded_h!=((height+127)&~127)||padded_w!=((width+127)&~127))
            throw std::runtime_error("native workspace extent mismatch");
        auto& state=cache();std::lock_guard<std::mutex> lock(state.mutex);
        if(state.poisoned)return done((request&0xff00u)|1u);
        int device;check(hip.device(&device));
        if(state.primary && (state.primary!=primary||state.device!=device)) {
            return done((request&0xff00u)|1u);
        }
        if(!state.primary) {
            HMODULE retained{};
#ifndef NRMP_CPU_TEST
            if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCWSTR>(base),&retained))throw std::runtime_error("proxy lifetime pin failed");
#else
            (void)retained;
#endif
            state.primary=primary;state.device=device;state.base=base;
        }
        const size_t pixels=size_t(width)*height,stage_bytes=pixels*8;
        const size_t output_bytes=size_t(output_pitch)*height;

        const size_t imported_bytes=size_t(padded_w)*padded_h*12;
        if(stage_bytes>size_t(512)*1024*1024||output_bytes>size_t(512)*1024*1024||imported_bytes>size_t(512)*1024*1024)
            throw std::runtime_error("staging size limit");
        state.ensure_kernels(hip);
        state.ensure_buffers(hip,(std::max)(stage_bytes,output_bytes));
        check(hip.set(state.status,0,4));

        const bool no_lugar=cena_config(base).menos_esperas&&cena_config(base).fold_proprio()&&cena_gpu().carregar(hip);
        check(hip.d2d(state.rollback,destination,output_bytes));
        if(!no_lugar)check(hip.d2d(state.temporary,destination,output_bytes));
        void* resid=no_lugar?destination:state.temporary;
        unsigned w=unsigned(width),h=unsigned(height),pw=unsigned(padded_w),pitch=unsigned(output_pitch/2);
        const unsigned grid=unsigned((pixels+255)/256);
        void* compose_args[]={&imported,&destination,&state.input,&w,&h,&pw,&pitch,&state.status};
        check(hip.launch(state.compose_kernel,grid,1,1,256,1,1,0,nullptr,compose_args,nullptr));
        controls[4]=.03125f*nrmp::multiplier(request);
        const nrcena::Config& cena=cena_config(base);
        if(cena.pele_ligada)controls[2]=cena.pele;

        std::chrono::steady_clock::time_point cr_t[6]{};
        const auto marca=[&](int k){if(cena.crono==1){hip.completed("crono",0);cr_t[k]=std::chrono::steady_clock::now();}};
        marca(0);

        const PortaoMovimento pm=job_assincrono?portao(base):PortaoMovimento{};
        const bool pesar=pm.ligado&&state.ensure_gate(hip,w,h);
        auto& cg=cena_gpu();
        const bool proprio=cena.fold_proprio()&&cg.carregar(hip);
        const float* eroded=nullptr;bool eroded_feito=false;
        const bool pulavel=proprio&&job_assincrono&&cena.pula()&&pesar&&cg.k_conta&&cg.k_diff_ancora&&cg.cnt;
        float frac=-1.f;
        if(pulavel){
            const bool tinha=state.gate_valid;unsigned valido=tinha?1u:0u,erosao=pm.erosao;
            void* diff_args[]={&imported,&state.previous,&state.diff,&w,&h,&pw,&valido};
            check(hip.launch(state.diff_kernel,grid,1,1,256,1,1,0,nullptr,diff_args,nullptr));
            void* erode_args[]={&state.diff,&state.eroded,&w,&h,&erosao};
            check(hip.launch(state.erode_kernel,grid,1,1,256,1,1,0,nullptr,erode_args,nullptr));
            eroded=static_cast<const float*>(state.eroded);state.gate_valid=true;eroded_feito=true;

            if(cena.menos_esperas&&cg.cnt_pend){unsigned conta=0;check(hip.copy(&conta,cg.cnt,4,2));frac=float(double(conta)/double(cg.cnt_pend));cg.cnt_pend=0;}
            if(tinha&&cena.parado>0.f){
                unsigned n=w*h;float t0=pm.t0;check(hip.set(cg.cnt,0,4));
                void* ca[]={&state.eroded,&n,&t0,&cg.cnt};check(hip.launch(cg.k_conta,grid,1,1,256,1,1,0,nullptr,ca,nullptr));
                if(!cena.menos_esperas){unsigned conta=0;check(hip.copy(&conta,cg.cnt,4,2));frac=float(double(conta)/double(n));}
                else cg.cnt_pend=n;
            }
        }

        auto& cgb=cg;
        unsigned lw=(unsigned(width)*unsigned(cena.escala)/100u)&~1u,lh=(unsigned(height)*unsigned(cena.escala)/100u)&~1u;

        if(cena.alinha>1){lw=lw/unsigned(cena.alinha)*unsigned(cena.alinha);lh=lh/unsigned(cena.alinha)*unsigned(cena.alinha);}
        const bool baixo_pedido=cena.escala<100&&lw>=128&&lh>=128&&cgb.carregar(hip)&&cgb.k_reduz&&cgb.k_fold_baixo;

        const bool guardado=cg.delta_ok&&baixo_pedido&&cg.delta_lw==lw&&cg.delta_lh==lh&&cg.ancora;

        const float frac_cam=pulavel&&cena.cam_px>0.f?cg.frac_camera(hip,cena.cam_px):-1.f;
        const bool cam_anda=pulavel&&nrcena::camera_anda(cena,frac_cam);
        bool calcula=!pulavel||(!cam_anda&&nrcena::calcula_extra(cena,frac,cg.sem,cena.reusa?guardado:true));
        bool reusa_job=!calcula&&cena.reusa&&guardado&&(!cam_anda||cena.cam_reusa);

        bool protegido=false;
        if(pulavel&&cena.protege){
            const unsigned long long agora=GetTickCount64();if(!cg.t_ini)cg.t_ini=t_sessao;
            const double seg=double(agora-cg.t_ini)/1000.0;
            const float fz=seg<double(cena.protege_ini_s)?-1.f:cg.frac_camera(hip,.01f);
            protegido=nrcena::protege_job(cena,seg,fz);
            if(protegido){calcula=false;reusa_job=false;cg.fade_v=0.f;++cg.n_prot;}

            if(protegido&&cena.pre_carrega){
                for(unsigned k=0;k+1<passes&&k<2;++k)if(!state.loaded[k]){
                    const auto t=std::chrono::steady_clock::now();(void)state.context(hip,k,controls);
                    const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();nrlento::pesos+=ms;
                    char t2[96];std::snprintf(t2,sizeof t2,"pre_carga contexto=%u ms=%.1f",k,ms);diagnostic(t2,passes,0,ms);
                }
            }
            if(protegido!=cg.prot_ant){char t[120];std::snprintf(t,sizeof t,"protege_%s seg=%.1f frac_zero=%.5f protegidos=%llu",protegido?"liga":"desliga",seg,double(fz),cg.n_prot);diagnostic(t,0,0,0);cg.prot_ant=protegido;}
        }

        float peso_fade=1.f;

        float peso_comp=1.f;
        if(pulavel&&cena.fade<=0)peso_comp=(calcula||reusa_job)?1.f:0.f;
        bool fade_alvo=true;
        if(pulavel&&cena.fade>0){
            const unsigned long long q=nrreusostate::quadro_cena;const int k=nrcena::quadros_entre(q,cg.q_fade);cg.q_fade=q;
            const bool alvo=(calcula||reusa_job)&&!protegido;fade_alvo=alvo;
            cg.fade_v=protegido?0.f:nrcena::passo_fade2(cg.fade_v,alvo,k,cena.fade,cena.fade_sobe,cena.fade_curva);
            if(!alvo&&cg.fade_v>0.f){calcula=true;reusa_job=false;++cg.p_fade;}
            peso_fade=cg.fade_v;peso_comp=peso_fade;
        }

        bool cont_job=false;nrcena::Bandas bandas{};
        if(pulavel&&nrcena::continuo(cena)&&!protegido&&baixo_pedido&&cg.k_fold_cont&&cg.k_cont_borra&&cg.k_ret_delta){
            if(!calcula&&!reusa_job)calcula=true;
            cont_job=true;bandas=nrcena::bandas(cena,cg.fade_v,reusa_job);peso_fade=1.f;
            const int esc=nrcena::escala_job(cena,!fade_alvo,cg.fade_v);
            if(esc!=cena.escala&&calcula){
                unsigned lw2=(unsigned(width)*unsigned(esc)/100u)&~1u,lh2=(unsigned(height)*unsigned(esc)/100u)&~1u;
                if(cena.alinha>1){lw2=lw2/unsigned(cena.alinha)*unsigned(cena.alinha);lh2=lh2/unsigned(cena.alinha)*unsigned(cena.alinha);}
                if(lw2>=128&&lh2>=128){lw=lw2;lh=lh2;++cg.c_barato;}
            }
            ++cg.c_cont;if(!fade_alvo)++cg.c_andando;cg.c_kd+=double(bandas.detalhe);cg.c_lw+=double(lw);
        }
        marca(1);
        const bool baixo=calcula&&baixo_pedido&&cgb.baixo_tamanho(hip,lw,lh);
        if(baixo){
            int sw=width,sh=height,dw=int(lw),dh=int(lh);void* src=state.input;
            void* ra[]={&src,&sw,&sh,&cgb.l0,&dw,&dh};check(hip.launch(cgb.k_reduz,(lw+255)/256,lh,1,256,1,1,0,nullptr,ra,nullptr));
            check(hip.d2d(cgb.la,cgb.l0,size_t(lw)*lh*8));
            const unsigned lgrid=unsigned((size_t(lw)*lh+255)/256);void* li=cgb.la;void* lo=cgb.lb;
            for(unsigned pass=1;pass<passes;++pass) {
                const bool carregar=!state.loaded[pass-1];const auto tc=std::chrono::steady_clock::now();
                void* extra=state.context(hip,pass-1,controls);
                if(carregar)nrlento::pesos+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tc).count();
                {SlowPhase slow{"extra_baixo",pass+1};
                 eval(extra,li,int(lw)*8,0,int(lh),int(lw),lo,int(lw)*8,0,0,nullptr);
                 hip.completed("extra_completion",pass+1);}
                void* sa[]={&lo,&lw,&lh,&state.status};check(hip.launch(state.sanitize_kernel,lgrid,1,1,256,1,1,0,nullptr,sa,nullptr));
                std::swap(li,lo);
            }
            cgb.la=li;cgb.lb=lo;
        }
        for(unsigned pass=1;pass<passes&&calcula&&!baixo;++pass) {
            const bool carregar=!state.loaded[pass-1];const auto tc=std::chrono::steady_clock::now();
            void* extra=state.context(hip,pass-1,controls);
            if(carregar)nrlento::pesos+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tc).count();
            {
                SlowPhase slow{"extra",pass+1};
                eval(extra,state.input,width*8,0,height,width,state.output,width*8,0,0,nullptr);

                hip.completed("extra_completion",pass+1);
            }
            void* sanitize_args[]={&state.output,&w,&h,&state.status};
            check(hip.launch(state.sanitize_kernel,grid,1,1,256,1,1,0,nullptr,sanitize_args,nullptr));
            std::swap(state.input,state.output);
        }
        marca(2);nrlento::marca(2);
        cg.m_ok=false;
        int cmw=0,cmh=0;const float* cmask=nullptr;
        if(proprio&&cena.modo>0&&cena.mascara_pulo&&reusa_job)cmask=cg.mascara_levada(hip,cena,cmw,cmh);

        const bool sem_fold=!calcula&&!reusa_job;

        const float g_comp=pulavel&&cg.k_cp_aplica?nrcena::comp_ganho(cena,cg.cp_r,peso_comp):1.f;
        if(proprio&&cena.modo>0&&!cmask&&cena.mascara_pulo&&sem_fold&&g_comp>1.f&&cena.comp_mascara)cmask=cg.mascara_levada(hip,cena,cmw,cmh);

        if(proprio&&cena.modo>0&&!cmask&&cont_job&&!fade_alvo&&cena.cont_mascara>1){
            if(++cg.c_masc%unsigned(cena.cont_mascara)!=0)cmask=cg.mascara_levada(hip,cena,cmw,cmh);
        }else if(cont_job)cg.c_masc=0;
        if(proprio&&cena.modo>0&&!cmask&&!(cena.mascara_pulo&&sem_fold))cmask=cg.mascara(hip,cena,cmw,cmh);

        const bool sobe_job=proprio&&cena.sobe&&cg.k_fold_cont&&cg.k_ret_delta&&cg.k_ret_baixa;
        if(proprio&&(cena.retifica()||cont_job||sobe_job)&&!cg.m_ok)cg.m_ok=CenaGpu::mv_do_job(cg.m_ult);
        marca(3);
        if(proprio){

            if(pesar&&!eroded_feito){
                unsigned valido=state.gate_valid?1u:0u,erosao=pm.erosao;
                void* diff_args[]={&imported,&state.previous,&state.diff,&w,&h,&pw,&valido};
                check(hip.launch(state.diff_kernel,grid,1,1,256,1,1,0,nullptr,diff_args,nullptr));
                void* erode_args[]={&state.diff,&state.eroded,&w,&h,&erosao};
                check(hip.launch(state.erode_kernel,grid,1,1,256,1,1,0,nullptr,erode_args,nullptr));
                eroded=static_cast<const float*>(state.eroded);state.gate_valid=true;
            }else if(!pesar)state.gate_valid=false;
            check(hip.set(cg.stats,0,16));
            unsigned raio=pm.raio;float t0=pm.t0,t1=pm.t1,forca=cena.forca,ganho=cena.ganho*peso_fade;int mw=cmw,mh=cmh;

            const auto dilata=[&](const float*& g){
                if(!cena.dilata_sep||!g||raio==0||!cg.k_filtro||g!=static_cast<const float*>(state.eroded))return;
                int wi=int(w),hi2=int(h),r=int(raio),d0=0,d1=1,op=1;void* src=state.eroded;void* tmp=state.diff;const unsigned gx=(w+255)/256;
                {void* af[]={&src,&tmp,&wi,&hi2,&r,&d0,&op};check(hip.launch(cg.k_filtro,gx,h,1,256,1,1,0,nullptr,af,nullptr));}
                {void* af[]={&tmp,&src,&wi,&hi2,&r,&d1,&op};check(hip.launch(cg.k_filtro,gx,h,1,256,1,1,0,nullptr,af,nullptr));}
                raio=0;};

            float sigma=cena.guia;const void* hi=state.input;
            const bool guiado=sigma>0.f&&cg.k_fold_baixo2;

            const bool ret=cena.retifica()&&cg.k_fold_ret;
            float idade_q=0.f;
            if(ret||cont_job||sobe_job){
                const unsigned long long q=nrreusostate::quadro_cena;
                if(q&&cg.q_ret_ant&&q>cg.q_ret_ant)cg.entre_ret=int((std::min)(q-cg.q_ret_ant,8ull));
                if(q)cg.q_ret_ant=q;
                const unsigned long long desde=(!calcula&&q&&cg.q_ancora&&q>cg.q_ancora)?q-cg.q_ancora:0ull;
                idade_q=nrcena::idade_ret(cena,cg.entre_ret,desde);
                if(calcula&&q)cg.q_ancora=q;
                cg.r_idade=idade_q;
            }
            auto fold_baixo=[&](void* lin,void* lout,unsigned flw,unsigned flh,const float* gate){
                int lwi=int(flw),lhi=int(flh);

                if((cont_job||sobe_job)&&cg.ret_tamanho(hip,flw,flh)&&(!cont_job||cg.cont_tamanho(hip,flw,flh))){
                    void* da[]={&lin,&lout,&lwi,&lhi,&cg.ret_dl,&state.status};check(hip.launch(cg.k_ret_delta,(flw+255)/256,flh,1,256,1,1,0,nullptr,da,nullptr));
                    if(cont_job){

                        int r=int(std::lround(double(cena.cont_raio)*double(flw)/double(w)));r=(std::max)(1,(std::min)(r,32));
                        int d0=0,d1=1;
                        {void* ba[]={&cg.ret_dl,&cg.cont_tmp,&lwi,&lhi,&r,&d0};check(hip.launch(cg.k_cont_borra,(flw+255)/256,flh,1,256,1,1,0,nullptr,ba,nullptr));}
                        {void* ba[]={&cg.cont_tmp,&cg.ret_dlb,&lwi,&lhi,&r,&d1};check(hip.launch(cg.k_cont_borra,(flw+255)/256,flh,1,256,1,1,0,nullptr,ba,nullptr));}
                    }else{void* ba[]={&cg.ret_dl,&lwi,&lhi,&cg.ret_dlb};check(hip.launch(cg.k_ret_baixa,(flw+255)/256,flh,1,256,1,1,0,nullptr,ba,nullptr));}
                    struct ContP {float kl,kd,pl,movl,sl,sd,sr;int guia;} cp{1.f,1.f,1.f,0.f,cena.sobe_luma,cena.sobe_prof,cena.sobe_raio,sobe_job?1:0};
                    static_assert(sizeof(ContP)==32,"ContP 32 B");
                    if(cont_job){cp.kl=bandas.baixa;cp.kd=bandas.detalhe;cp.pl=bandas.portao_baixa;cp.movl=cena.cont_mov;}
                    CenaMv mv=cg.m_ult;int mvok=cg.m_ok?1:0;float movpx=cena.ret_mov,gama=cena.ret_gama,gamamov=cena.ret_gama_mov,escuro=cena.ret_escuro;
                    int rr=cena.ret_raio;void* r1s=state.rollback;
                    void* fc[]={&cg.ret_dl,&cg.ret_dlb,&lin,&lwi,&lhi,&resid,&r1s,&hi,&gate,&cmask,&mw,&mh,&w,&h,&pitch,&raio,&t0,&t1,&forca,&ganho,&cg.stats,&state.status,
                                &mv,&mvok,&idade_q,&movpx,&gama,&gamamov,&escuro,&rr,&cp};
                    check(hip.launch(cg.k_fold_cont,(w+15)/16,(h+15)/16,1,16,16,1,0,nullptr,fc,nullptr));
                    if(sobe_job)++cg.c_guiado;
                }else if(ret){
                    CenaMv mv=cg.m_ult;int mvok=cg.m_ok?1:0;float movpx=cena.ret_mov,gama=cena.ret_gama,gamamov=cena.ret_gama_mov,escuro=cena.ret_escuro;
                    int tudo=cena.ret_tudo,rr=cena.ret_raio;void* r1s=state.rollback;

                    void* fr[]={&lin,&lout,&lwi,&lhi,&resid,&r1s,&gate,&cmask,&mw,&mh,&w,&h,&pitch,&raio,&t0,&t1,&forca,&ganho,&cg.stats,&state.status,
                                &mv,&mvok,&idade_q,&movpx,&tudo,&gama,&gamamov,&escuro,&rr};
                    if(!cena.ret_v1&&cg.k_fold_ret2&&cg.ret_tamanho(hip,flw,flh)){

                        void* da[]={&lin,&lout,&lwi,&lhi,&cg.ret_dl,&state.status};check(hip.launch(cg.k_ret_delta,(flw+255)/256,flh,1,256,1,1,0,nullptr,da,nullptr));
                        if(movpx>0.f&&!tudo){void* ba[]={&cg.ret_dl,&lwi,&lhi,&cg.ret_dlb};check(hip.launch(cg.k_ret_baixa,(flw+255)/256,flh,1,256,1,1,0,nullptr,ba,nullptr));}
                        void* f2[]={&cg.ret_dl,&cg.ret_dlb,&lwi,&lhi,&resid,&r1s,&gate,&cmask,&mw,&mh,&w,&h,&pitch,&raio,&t0,&t1,&forca,&ganho,&cg.stats,&state.status,
                                    &mv,&mvok,&idade_q,&movpx,&tudo,&gama,&gamamov,&escuro,&rr};
                        check(hip.launch(cg.k_fold_ret2,(w+15)/16,(h+15)/16,1,16,16,1,0,nullptr,f2,nullptr));
                    }else check(hip.launch(cg.k_fold_ret,grid,1,1,256,1,1,0,nullptr,fr,nullptr));
                }else if(guiado){
                    void* fb[]={&lin,&lout,&lwi,&lhi,&resid,&gate,&cmask,&mw,&mh,&w,&h,&pitch,&raio,&t0,&t1,&forca,&ganho,&cg.stats,&state.status,&hi,&sigma};
                    check(hip.launch(cg.k_fold_baixo2,grid,1,1,256,1,1,0,nullptr,fb,nullptr));
                }else{
                    void* fb[]={&lin,&lout,&lwi,&lhi,&resid,&gate,&cmask,&mw,&mh,&w,&h,&pitch,&raio,&t0,&t1,&forca,&ganho,&cg.stats,&state.status};
                    check(hip.launch(cg.k_fold_baixo,grid,1,1,256,1,1,0,nullptr,fb,nullptr));
                }
            };
            if(calcula){
                dilata(eroded);
                if(baixo)fold_baixo(cg.l0,cg.la,lw,lh,eroded);
                else{
                    void* fold_args[]={&imported,&state.input,&resid,&eroded,&cmask,&mw,&mh,&w,&h,&pw,&pitch,&raio,&t0,&t1,&forca,&ganho,&cg.stats,&state.status};
                    check(hip.launch(cg.k_fold,grid,1,1,256,1,1,0,nullptr,fold_args,nullptr));
                }
            }else if(reusa_job){

                unsigned erosao=pm.erosao;

                const float nq=cena.warp?nrcena::quadros_warp(nrreusostate::quadro_cena,cg.anc_q,cena.warp_fator):0.f;
                CenaMv mvj{};
                if(nq!=0.f&&cg.k_diff_warp&&cg.k_fold_warp&&!guiado&&cg.ler_mv(mvj)){
                    float nqa=nq;
                    void* da[]={&imported,&cg.ancora,&state.diff,&w,&h,&pw,&mvj,&nqa};check(hip.launch(cg.k_diff_warp,grid,1,1,256,1,1,0,nullptr,da,nullptr));
                    void* ea[]={&state.diff,&state.eroded,&w,&h,&erosao};check(hip.launch(state.erode_kernel,grid,1,1,256,1,1,0,nullptr,ea,nullptr));
                    int lwi=int(cg.delta_lw),lhi=int(cg.delta_lh);const float* gate=static_cast<const float*>(state.eroded);dilata(gate);
                    void* fb[]={&cg.l0,&cg.la,&lwi,&lhi,&resid,&gate,&cmask,&mw,&mh,&w,&h,&pitch,&raio,&t0,&t1,&forca,&ganho,&cg.stats,&state.status,&mvj,&nqa};
                    check(hip.launch(cg.k_fold_warp,grid,1,1,256,1,1,0,nullptr,fb,nullptr));
                    ++cg.p_warp;cg.p_nq+=double(nq);
                }else{
                void* da[]={&imported,&cg.ancora,&state.diff,&w,&h,&pw};check(hip.launch(cg.k_diff_ancora,grid,1,1,256,1,1,0,nullptr,da,nullptr));
                void* ea[]={&state.diff,&state.eroded,&w,&h,&erosao};check(hip.launch(state.erode_kernel,grid,1,1,256,1,1,0,nullptr,ea,nullptr));
                const float* gate=static_cast<const float*>(state.eroded);dilata(gate);
                fold_baixo(cg.l0,cg.la,cg.delta_lw,cg.delta_lh,gate);
                }
            }

            cg.cp_mediu=false;
            if(pulavel&&cena.comp>0.f&&cg.k_cp_aplica){
                const float* pm2=cena.comp_mascara?cmask:nullptr;int mw2=cena.comp_mascara?cmw:0,mh2=cena.comp_mascara?cmh:0;float fo=forca;
                void* r1s=state.rollback;
                if(g_comp>1.f){
                    float g=g_comp,esc=cena.comp_escuro;int rb=cena.comp_baixa;
                    void* ca[]={&resid,&r1s,&w,&h,&pitch,&pm2,&mw2,&mh2,&fo,&g,&esc,&rb,&state.status};
                    check(hip.launch(cg.k_cp_aplica,grid,1,1,256,1,1,0,nullptr,ca,nullptr));
                    ++cg.cp_comp;cg.cp_g+=double(g);
                }else if(calcula&&peso_comp>=.999f){
                    check(hip.set(cg.cp_soma,0,8));
                    const unsigned mg=unsigned((size_t((w+1)/2)*((h+1)/2)+255)/256);
                    void* ma[]={&resid,&r1s,&imported,&w,&h,&pw,&pitch,&pm2,&mw2,&mh2,&fo,&cg.cp_soma};
                    check(hip.launch(cg.k_cp_mede,mg,1,1,256,1,1,0,nullptr,ma,nullptr));
                    cg.cp_mediu=true;
                }
                ++cg.cp_jobs;
            }
            if(pulavel){
                ++cg.p_jobs;cg.p_frac+=frac>0.f?double(frac):0.;if(cam_anda)++cg.p_cam;cg.p_fcam+=frac_cam>0.f?double(frac_cam):0.;
                {const float lim[6]={.01f,.02f,.05f,.1f,.2f,.5f};int k=0;while(k<6&&frac>lim[k])++k;if(frac>=0.f)++cg.p_hist[k];}
                if(calcula){
                    ++cg.p_calc;cg.sem=0;

                    const size_t anc=size_t(w)*h*12;
                    if(baixo&&cg.ancora_tamanho(hip,anc)){check(hip.d2d(cg.ancora,state.previous,anc));cg.delta_ok=true;cg.delta_lw=lw;cg.delta_lh=lh;cg.anc_q=nrreusostate::quadro_cena;}
                    else cg.delta_ok=false;
                }else{++cg.p_pul;++cg.sem;if(reusa_job)++cg.p_reus;}
                if(cg.p_jobs<=1||cg.p_jobs%64==0){
                    char line[480];std::snprintf(line,sizeof line,"parado jobs=%llu calculados=%llu pulados=%llu reusados=%llu frac_media=%.3f frac=%.3f hist=%llu,%llu,%llu,%llu,%llu,%llu,%llu camera=%llu fcam_media=%.3f fcam=%.3f warp=%llu nq_medio=%.2f fade=%.2f fade_calc=%llu",
                        cg.p_jobs,cg.p_calc,cg.p_pul,cg.p_reus,cg.p_jobs?cg.p_frac/double(cg.p_jobs):0.,double(frac),
                        cg.p_hist[0],cg.p_hist[1],cg.p_hist[2],cg.p_hist[3],cg.p_hist[4],cg.p_hist[5],cg.p_hist[6],
                        cg.p_cam,cg.p_jobs?cg.p_fcam/double(cg.p_jobs):0.,double(frac_cam),cg.p_warp,cg.p_warp?cg.p_nq/double(cg.p_warp):0.,double(cg.fade_v),cg.p_fade);
                    diagnostic(line,passes,0,0,request,cg.p_jobs);
                    if(nrcena::continuo(cena)||cena.sobe){
                        std::snprintf(line,sizeof line,"continuo jobs=%llu cont=%llu andando=%llu baratos=%llu guiados=%llu kd_medio=%.3f lw_medio=%.0f",
                            cg.p_jobs,cg.c_cont,cg.c_andando,cg.c_barato,cg.c_guiado,cg.c_cont?cg.c_kd/double(cg.c_cont):0.,cg.c_cont?cg.c_lw/double(cg.c_cont):0.);
                        diagnostic(line,passes,0,0,request,cg.p_jobs);
                    }
                }
            }
        }else if(pesar){
            unsigned valido=state.gate_valid?1u:0u,raio=pm.raio,erosao=pm.erosao;float t0=pm.t0,t1=pm.t1;
            check(hip.set(state.gate_stats,0,4));
            void* diff_args[]={&imported,&state.previous,&state.diff,&w,&h,&pw,&valido};
            check(hip.launch(state.diff_kernel,grid,1,1,256,1,1,0,nullptr,diff_args,nullptr));
            void* erode_args[]={&state.diff,&state.eroded,&w,&h,&erosao};
            check(hip.launch(state.erode_kernel,grid,1,1,256,1,1,0,nullptr,erode_args,nullptr));
            void* gated_args[]={&imported,&state.input,&state.temporary,&state.eroded,&w,&h,&pw,&pitch,&raio,&t0,&t1,&state.gate_stats,&state.status};
            check(hip.launch(state.gated_kernel,grid,1,1,256,1,1,0,nullptr,gated_args,nullptr));
            state.gate_valid=true;
        }else{
            state.gate_valid=false;
            void* fold_args[]={&imported,&state.input,&state.temporary,&w,&h,&pw,&pitch,&state.status};
            check(hip.launch(state.fold_kernel,grid,1,1,256,1,1,0,nullptr,fold_args,nullptr));
        }
        nrlento::marca(3);
        if(!no_lugar||cena.crono==1)hip.completed();
        marca(4);
        if(proprio){

            if(cg.jobs%8==0||cg.jobs<=1){unsigned st[4]{};check(hip.copy(st,cg.stats,16,2));
            cg.soma_w+=double(st[0]);cg.soma_m+=double(st[1]);cg.px+=double(w)*h;
            if(cena.retifica()&&cg.k_fold_ret&&(calcula||reusa_job)){
                ++cg.r_jobs;cg.r_t+=double(st[2]);cg.r_k+=double(st[3]);cg.r_px+=double(w)*h;
                if(cg.r_jobs<=1||cg.r_jobs%8==0){
                    char line[256];std::snprintf(line,sizeof line,"retifica jobs=%llu t_medio=%.3f k_medio=%.3f idade_q=%.2f mv=%d",
                        cg.r_jobs,cg.r_px>0?cg.r_t/(255.0*cg.r_px):0.,cg.r_px>0?cg.r_k/(255.0*cg.r_px):0.,double(cg.r_idade),cg.m_ok?1:0);
                    diagnostic(line,passes,0,0,request,cg.r_jobs);cg.r_t=cg.r_k=cg.r_px=0.;
                }
            }}

            if((cg.jobs<=1||cg.jobs%64==0)&&cg.jobs!=cg.jobs_log){cg.jobs_log=cg.jobs;
                char line[256];std::snprintf(line,sizeof line,"cena jobs=%llu mascaras=%llu levadas=%llu sem_mv=%llu invalidos=%llu peso_medio=%.3f mascara_media=%.3f",
                    cg.jobs,cg.mascaras,cg.levadas,cg.sem_mv,cg.invalidos,cg.px>0?cg.soma_w/(255.0*cg.px):0.,cg.px>0?cg.soma_m/(255.0*cg.px):0.);
                diagnostic(line,passes,0,0,request,cg.jobs);cg.soma_w=cg.soma_m=cg.px=0.;
            }
        }else if(pesar){
            unsigned soma=0;check(hip.copy(&soma,state.gate_stats,4,2));
            ++state.gate_jobs;state.gate_sum+=soma;state.gate_pixels+=size_t(w)*h;
            if(state.gate_jobs==1||state.gate_jobs%64==0){

                const double media=state.gate_pixels?double(state.gate_sum)/(255.0*double(state.gate_pixels)):0.;
                diagnostic("motion_gate",passes,0,media*1000.0,request,state.gate_jobs);
                state.gate_sum=state.gate_pixels=0;
            }
        }
        unsigned invalid=0;check(hip.copy(&invalid,state.status,4,2));nrlento::marca(4);
        if(invalid){if(no_lugar){check(hip.copy(destination,state.rollback,output_bytes,3));hip.completed();}throw std::runtime_error("invalid GPU working RGB");}

        if(proprio&&cg.cp_mediu){
            float sm[2]{};check(hip.copy(sm,cg.cp_soma,8,2));cg.cp_mediu=false;
            if(sm[0]>0.f){cg.cp_r=nrcena::comp_mede(cena,cg.cp_r,sm[1]/sm[0]);++cg.cp_med;}
        }
        if(proprio&&cena.comp>0.f&&cg.cp_jobs&&(cg.cp_jobs<=1||cg.cp_jobs%64==0)){
            char line[200];std::snprintf(line,sizeof line,"comp jobs=%llu medidas=%llu r=%.3f compensados=%llu ganho_medio=%.3f",
                cg.cp_jobs,cg.cp_med,double(cg.cp_r),cg.cp_comp,cg.cp_comp?cg.cp_g/double(cg.cp_comp):1.);
            diagnostic(line,passes,0,0,request,cg.cp_jobs);cg.cp_g=0.;cg.cp_comp=0;
        }

        baseline_preserved=false;
        int published=no_lugar?0:hip.d2d(destination,state.temporary,output_bytes);
        if(published) {
            baseline_preserved=hip.copy(destination,state.rollback,output_bytes,3)==0;
            NRDIAG(if(!baseline_preserved)OutputDebugStringA("NR multipass publication AND residual restore failed\n");)
            check(published);
        }
        if(!no_lugar)hip.completed();
        done(request);
        if(cena.crono==2){

            const int k=calcula?0:1;++cg.cr_n[k];cg.cr_ms[k][0]+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-extra_chain_started).count();
            cg.cr_ms[k][1]+=std::chrono::duration<double,std::milli>(passo1_pronto-extra_chain_started).count();
            if((cg.cr_n[0]+cg.cr_n[1])%64==0){
                char line[240];const auto m=[&](int a,int f){return cg.cr_n[a]?cg.cr_ms[a][f]/double(cg.cr_n[a]):0.;};
                std::snprintf(line,sizeof line,"crono2 calc=%llu total=%.2f espera_passo1=%.2f | pulado=%llu total=%.2f espera_passo1=%.2f",cg.cr_n[0],m(0,0),m(0,1),cg.cr_n[1],m(1,0),m(1,1));
                diagnostic(line,passes,0,0,request,cg.cr_n[0]+cg.cr_n[1]);for(auto& a:cg.cr_ms)for(auto& v:a)v=0.;cg.cr_n[0]=cg.cr_n[1]=0;}
        }
        if(cena.crono==1){
            marca(5);const int k=calcula?0:1;++cg.cr_n[k];
            for(int f=0;f<5;++f)cg.cr_ms[k][f]+=std::chrono::duration<double,std::milli>(cr_t[f+1]-cr_t[f]).count();
            cg.cr_ms[k][5]+=std::chrono::duration<double,std::milli>(cr_t[0]-extra_chain_started).count();
            if((cg.cr_n[0]+cg.cr_n[1])%64==0){
                char line[400];const auto m=[&](int a,int f){return cg.cr_n[a]?cg.cr_ms[a][f]/double(cg.cr_n[a]):0.;};
                std::snprintf(line,sizeof line,"crono calc=%llu entrada=%.2f portao=%.2f passos=%.2f mascara=%.2f fold=%.2f publica=%.2f | pulado=%llu entrada=%.2f portao=%.2f passos=%.2f mascara=%.2f fold=%.2f publica=%.2f",
                    cg.cr_n[0],m(0,5),m(0,0),m(0,1),m(0,2),m(0,3),m(0,4),cg.cr_n[1],m(1,5),m(1,0),m(1,1),m(1,2),m(1,3),m(1,4));
                diagnostic(line,passes,0,0,request,cg.cr_n[0]+cg.cr_n[1]);
                for(auto& a:cg.cr_ms)for(auto& v:a)v=0.;cg.cr_n[0]=cg.cr_n[1]=0;
            }
        }
        ++state.completed_jobs;
        if(state.completed_jobs==1 || state.completed_jobs%256==0){
            const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-extra_chain_started).count();
            diagnostic("multipass_completed",passes,0,ms,request,state.completed_jobs);
        }
        return request;
    }catch(const HipFailure& failure) {
        auto& state=cache();{std::lock_guard<std::mutex> lock(state.mutex);state.poisoned=true;
            if(failure.uncertain)state.completion_uncertain.store(true,std::memory_order_release);}
        diagnostic(failure.phase,failure.pass,failure.status,0);
        if(failure.uncertain){acknowledge(base,0u);throw;}
        return done(baseline_preserved?((request&0xff00u)|1u):0u);
    }catch(...) {

        try {auto& state=cache();std::lock_guard<std::mutex> lock(state.mutex);state.poisoned=true;}catch(...){}

        NRDIAG(OutputDebugStringA("NR multipass incomplete; effective onepass or unknown\n");)
        diagnostic("incomplete_poisoned",request&255u,0,0,request,0);
        return done(baseline_preserved?((request&0xff00u)|1u):0u);
    }
}

#ifndef NRMP_CPU_TEST
namespace reuso_cena {
thread_local Eval nativo{};thread_local unsigned chamadas{};
static void chamar(void* primary,void* source,int input_pitch,int input_format,int height,int width,void* destination,
    int output_pitch,int output_format,int tone,void* metadata) noexcept(false) {
    nrreusostate::EscopoCena escopo(chamadas++==0);
    nativo(primary,source,input_pitch,input_format,height,width,destination,output_pitch,output_format,tone,metadata);
}
}
#endif
static void run_multipass_eval(void* primary,void* source,int input_pitch,int input_format,
    int height,int width,void* destination,int output_pitch,int output_format,int tone,void* metadata,nrmptiming::Scope& timing) noexcept(false) {
    auto base=static_cast<char*>(primary)-0x93f48;
    if(cache().completion_uncertain.load(std::memory_order_acquire)){
        acknowledge(base,0);throw std::runtime_error("multipass device completion uncertain");
    }
    auto eval=native<Eval>(base,0x2a080);
    (void)cena_config(base);nrcena::params_ok=false;
#ifndef NRMP_CPU_TEST

    reuso_cena::nativo=eval;reuso_cena::chamadas=0;eval=&reuso_cena::chamar;
#endif
    unsigned request=nrmp::valid_config(unsigned(InterlockedCompareExchange(
        reinterpret_cast<volatile LONG*>(base+0x71d090),0,0)));
#if NR_LOG_ATIVO && !defined(NRMP_CPU_TEST)

    {static const std::vector<std::pair<unsigned,unsigned>> plano=[]{std::vector<std::pair<unsigned,unsigned>> v;char b[256]{};size_t n=0;
         if(getenv_s(&n,b,sizeof b,"NR_TESTE_PASSOS")==0&&n){char* ctx=nullptr;for(char* t=strtok_s(b,";",&ctx);t;t=strtok_s(nullptr,";",&ctx)){unsigned j=0,x=0;if(sscanf_s(t,"%u:%u",&j,&x)==2)v.emplace_back(j,x);}}
         return v;}();
     static std::atomic<unsigned> avaliacoes{0};const unsigned a=avaliacoes.fetch_add(1);
     for(const auto& p:plano)if(a>=p.first)request=nrmp::valid_config(p.second);}
#endif
    nrprescale::passos_pedidos.store(request&255u,std::memory_order_relaxed);
#ifndef NRMP_CPU_TEST

    if(nrneuraltone::requested&&!nrneuraltone::multipass_ok&&(request&255u)>1)
        throw std::runtime_error("ToneCurve requires one neural pass");
#endif
    if(timing.active){timing.sample.request=request;timing.sample.request_known=true;}
    nrtone::State legacy_state; nrtone::Worker legacy_worker;
    const bool tone_bound=nrtone::owner.load(std::memory_order_acquire)==base;
    auto tone_ticket=(tone_bound ? nrtone::worker : legacy_worker).prepare(
        tone_bound ? nrtone::state : legacy_state,primary,read<float>(base,0x94aa4));
    float controls[5];std::memcpy(controls,static_cast<char*>(primary)+0x20,20);
#ifndef NRMP_CPU_TEST
    if(timing.active)timing.sample.route=nrmptiming::Route::prescale_unattributed;

    unsigned effective=request;bool chained=false;
    try{
        if(nrprescale_runtime::evaluate(eval,primary,source,input_pitch,input_format,height,width,destination,output_pitch,output_format,tone,metadata,request,controls,chained,effective)){
            tone_ticket.complete();acknowledge(base,chained?effective:request);return;
        }
    }catch(...){if(chained)acknowledge(base,0u);throw;}
    if(timing.active)timing.sample.route=nrmptiming::Route::not_reached;
#endif
#ifndef NRMP_CPU_TEST
    nrstyletrace::begin(base);
    nrmeasurelive::begin(base,primary,width,height,metadata);
    struct TraceScope {bool captured=false;~TraceScope(){nrmeasurelive::abandon();if(!captured)nrstyletrace::abandon();}} trace_scope;
#endif

    if((request&255u)>1 && nrmp::multiplier(request)!=0){
        SlowPhase slow{"primary",1};
        nrmptiming::Scope::Primary primary_timing{timing};
        eval(primary,source,input_pitch,input_format,height,width,destination,output_pitch,output_format,tone,metadata);
    }else {
        nrmptiming::Scope::Primary primary_timing{timing};
        eval(primary,source,input_pitch,input_format,height,width,destination,output_pitch,output_format,tone,metadata);
    }
    nrlento::marca(0);
#ifndef NRMP_CPU_TEST
    nrmeasurelive::end();
#endif
    tone_ticket.complete();
#ifndef NRMP_CPU_TEST

    static std::array<float,5> previous_controls{};static unsigned previous_style=~0u;
    const auto completed_style=read<unsigned>(base,0x71d084);
    if(previous_style!=completed_style || std::memcmp(previous_controls.data(),controls,sizeof(controls))!=0){

std::ofstream log("nr_model_controls.log",std::ios::app);
        log<<"pid="<<GetCurrentProcessId()<<" eval_returned=1 style="<<completed_style
           <<" tone="<<controls[0]<<" structure="<<controls[1]<<" skin_mask="<<controls[2]
           <<" local_mask="<<controls[3]<<" amd_scale="<<controls[4]<<" width="<<width<<" height="<<height<<'\n';

        std::memcpy(previous_controls.data(),controls,sizeof(controls));previous_style=completed_style;
    }
    nrstyletrace::capture(primary,destination,output_pitch,output_format,width,height);
    trace_scope.captured=true;
#endif

    if((request&255u)==1 || nrmp::multiplier(request)==0) {acknowledge(base,request);return;}
    (void)extra_passes(eval,base,primary,height,width,destination,output_pitch,output_format,metadata,request,controls,true);
}

namespace nrpico {
inline int ligado=-1;inline std::string buf;inline unsigned long long ru0{},rn0{},re0{},ch0{},fo0{};
inline bool ativo(char* base) noexcept {
 if(ligado<0){ligado=0;try{const auto ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&ativo));
  if(!ini.empty())ligado=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralLogPicoRede",0,ini.c_str()))==1?1:0;}catch(...){ligado=0;}}
 return ligado==1;}
inline double ms(std::int64_t a,std::int64_t b) noexcept {return nrjob::timeline().ms(b-a);}

inline std::int64_t a_top{},a_d{},a_fim{},a_cerca{},a_subm{},a_capt{};inline int a_job{},a_flush{};
struct Fim {bool on{false};~Fim(){if(on)a_fim=nrjob::qpc();}};

inline void gravar(std::string&& texto) noexcept {try{std::thread([t=std::move(texto)]{FILE* f{};if(fopen_s(&f,"nr_pico_rede.log","a")==0&&f){std::fputs(t.c_str(),f);std::fclose(f);}}).detach();}catch(...){}}
}

namespace nrpainel {
inline void no_eval(char* base) noexcept {
 int n=jobs.load(std::memory_order_relaxed);
 if(n<0){n=0;try{const auto ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&no_eval));
  if(!ini.empty())n=std::clamp(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralPainelRedeJobs",0,ini.c_str())),0,31);}catch(...){n=0;}
  jobs.store(n,std::memory_order_relaxed);}
 if(n<=0)return;
 const int pj=read<int>(base,0x94b98);const unsigned long long pm=read<unsigned long long>(base,0x94b90);
 if(pj<=0||pm>100000ull)return;
 mediana_ms.store(janela().empurra(pj,std::uint32_t(pm),unsigned(n)),std::memory_order_relaxed);
}
}
extern "C" __declspec(dllexport) void NRMultipassEval_v1(void* primary,void* source,int input_pitch,int input_format,
    int height,int width,void* destination,int output_pitch,int output_format,int tone,void* metadata) noexcept(false) {
    const std::int64_t pc_top=nrjob::qpc();nrpico::Fim pc_fim;
#ifndef NRMP_CPU_TEST
    nrmpsessao::carimba();
#endif
    nrmptiming::Scope timing{width,height,input_pitch,output_pitch,input_format,output_format,tone,metadata};
    auto* base=static_cast<char*>(primary)-0x93f48;

    struct MarcaAssincrono {explicit MarcaAssincrono(bool v) noexcept {job_assincrono=v;} ~MarcaAssincrono(){job_assincrono=false;}}
        marca_assincrono(nrtone::owner.load(std::memory_order_acquire)==base);
#ifndef NRMP_CPU_TEST

    const int job=nrtone::owner.load(std::memory_order_acquire)==base ? read<int>(base,0x949bc) : -1;
    nrreusostate::quadro_cena=nrreusostate::quadro_do_job(job);
    nrhistmov::job_atual=job;
    const auto publish=[job](int status) noexcept {
        if(job<=0)return;

        nrjob::timeline().stamp(job,nrjob::Publicado,nrjob::qpc());
        nrmode::async_completion_status.store(status,std::memory_order_relaxed);
        nrmode::async_completed_job.store(job,std::memory_order_release);
        nrjob::signal_completion();
    };

    const bool pico=job>0&&nrpico::ativo(base);const std::int64_t pc_e=pico?nrjob::qpc():0;
    pc_fim.on=pico;if(pico)nrpainel::sonda.store(true,std::memory_order_relaxed);
    if(job>0)nrpainel::no_eval(base);
    const unsigned long long pc_painel=pico?read<unsigned long long>(base,0x94b90):0;const int pc_pjob=pico?read<int>(base,0x94b98):0;
    const bool inicio_depois_da_trava=nrrj::sj_q()>0;
    if(job>0&&!inicio_depois_da_trava)nrjob::timeline().stamp(job,nrjob::EvalInicio,nrjob::qpc());

    if(job>0){nrrj_runtime::ler_ini(base);if(nrrj::ativo())nrcena::quer_mv.store(true,std::memory_order_relaxed);}

    if(job>0)nrexpofresca::no_eval(job,metadata);

    nrhipprio::Scope priority_scope(nrhipprio::active());

    std::unique_lock<std::recursive_mutex> trava_rj(nrregswin::rjsync::trava(),std::defer_lock);
    const int pc_dentro=pico?nrregswin::rjsync::rosto_dentro.load(std::memory_order_relaxed):0;const std::int64_t pc_t0=pico?nrjob::qpc():0;
    if(nrregswin::rjsync::ligado.load(std::memory_order_relaxed))trava_rj.lock();
    const std::int64_t pc_t1=pico?nrjob::qpc():0;
    if(job>0&&inicio_depois_da_trava)nrjob::timeline().stamp(job,nrjob::EvalInicio,nrjob::qpc());
    try {
        if(nrneuraltone::failed||nrregswin::dispatch.failure.load(std::memory_order_acquire))
            throw std::runtime_error("Neural replacement dispatch previously failed");
#endif
        nrlento::inicio();
        run_multipass_eval(primary,source,input_pitch,input_format,height,width,destination,
            output_pitch,output_format,tone,metadata,timing);
#ifndef NRMP_CPU_TEST
        if(nrlento::multi){

            const double total=nrlento::agora_ms();const double sem_pesos=total-nrlento::pesos;
            const auto& cc=cena_config(base);bool desligou=false;
            if(cc.trava_ms>0&&!trava_desligada.load(std::memory_order_relaxed)&&
               nrcena::trava_registra(trava_sessao,cc,sem_pesos,GetTickCount64())){trava_desligada.store(true);desligou=true;}
            const unsigned pedido=read<unsigned>(base,0x71d090);
            if(total>300.)nrlento::registra(job,total,pedido&255u,pedido,desligou);
            if(desligou){char l[160];std::snprintf(l,sizeof l,"trava_desliga travadas=%d janela_s=%d (so o passo 1 ate o fim da sessao)",trava_sessao.n,cc.trava_janela_s);diagnostic_escreve(l,1,0,sem_pesos,0,0);}
        }
#endif
#ifndef NRMP_CPU_TEST
        if(nrneuraltone::failed||nrregswin::dispatch.failure.load(std::memory_order_acquire))
            throw std::runtime_error("Neural replacement dispatch failed; result not published");
        if(job>0)nrjob::timeline().stamp(job,nrjob::EvalFim,nrjob::qpc());
        const std::int64_t pc_f=pico?nrjob::qpc():0;
        if(job>0&&nrredecada::cam::par.load(std::memory_order_relaxed)>1)cadencia_camera();

        if(job>0)nrrj_runtime::antes_do_publish(base,primary,job,destination,width,height,output_pitch,output_format);
        const std::int64_t pc_a=pico?nrjob::qpc():0;
#endif
        if(timing.active){timing.sample.effective_config=read<unsigned>(base,0x71d094);timing.sample.effective_config_known=true;}
#ifndef NRMP_CPU_TEST
        if(job>0){

            Hip hip;hip.completed("async_completion",1);

            nrlate::publish(job);
            const std::int64_t pc_c=pico?nrjob::qpc():0;
            publish(0);
            const std::int64_t pc_p=pico?nrjob::qpc():0;
            nrrj_runtime::depois_do_publish();
            if(pico){const std::int64_t pc_d=nrjob::qpc();namespace R=nrregswin::rjsync;namespace D=nrregswin::r1d;
                const auto ru=R::rosto_us.load(std::memory_order_relaxed),rn=R::rosto_n.load(std::memory_order_relaxed);
                char l[400];std::snprintf(l,sizeof l,"PICO job=%d t=%.3f painel_job=%d painel_ms=%llu trava=%.2f rosto_dentro=%d eval=%.2f antes=%.2f compl=%.2f publ=%.2f depois=%.2f total=%.2f rosto_ms=%.2f rosto_n=%llu reuso=%llu cheio=%llu forcado=%llu mov=%.2f movfrac=%.1f ant_job=%d ant_cerca_top=%.2f ant_subm_top=%.2f ant_d_fim=%.2f ant_flush=%d ant_capt_fim=%.2f painel_med=%d quadro_max=%.2f\n",
                    job,double(pc_e)/double(nrjob::qpc_frequency()),pc_pjob,pc_painel,nrpico::ms(pc_t0,pc_t1),pc_dentro,nrpico::ms(pc_t1,pc_f),nrpico::ms(pc_f,pc_a),nrpico::ms(pc_a,pc_c),nrpico::ms(pc_c,pc_p),nrpico::ms(pc_p,pc_d),nrpico::ms(pc_e,pc_d),
                    double(ru-nrpico::ru0)/1000.0,rn-nrpico::rn0,D::reusos-nrpico::re0,D::cheios-nrpico::ch0,D::mov::forcados-nrpico::fo0,double(D::mov::ult.media),double(D::mov::ult.frac),
                    nrpico::a_job,nrpico::a_cerca?nrpico::ms(nrpico::a_cerca,nrpico::a_top):-1.0,nrpico::a_subm?nrpico::ms(nrpico::a_subm,nrpico::a_top):-1.0,nrpico::ms(nrpico::a_d,nrpico::a_fim),nrpico::a_flush,nrpico::a_capt?nrpico::ms(nrpico::a_capt,nrpico::a_fim):-1.0,
                    nrpainel::mediana_ms.load(std::memory_order_relaxed),nrjob::timeline().ms(nrpainel::present_max.exchange(0,std::memory_order_relaxed)));
                nrpico::ru0=ru;nrpico::rn0=rn;nrpico::re0=D::reusos;nrpico::ch0=D::cheios;nrpico::fo0=D::mov::forcados;nrpico::buf+=l;
                nrpico::a_flush=0;if(nrpico::buf.size()>8192){nrpico::a_flush=1;nrpico::gravar(std::move(nrpico::buf));nrpico::buf=std::string{};}
                nrpico::a_job=job;nrpico::a_top=pc_top;nrpico::a_d=pc_d;nrpico::a_cerca=nrjob::timeline().when(job,nrjob::CercaPronta);
                nrpico::a_subm=nrjob::timeline().when(job,nrjob::Submetido);nrpico::a_capt=nrjob::timeline().when(job,nrjob::Capturado);}
        }
    }catch(const HipFailure& failure){
        publish(failure.status ? failure.status : -1);
        throw;
    }catch(...){
        publish(-1);
        throw;
    }
#endif
}
