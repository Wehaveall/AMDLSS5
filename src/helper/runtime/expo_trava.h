#pragma once

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include "jogo_conhecido.h"

namespace nrexpotrava {
inline constexpr std::uint32_t rva_expo=0x94A94, rva_iniciada=0x94A98;
struct Config { float fixa{}, minimo{}, maximo{}, suave_s{}, alvo{}, media{}; bool ativa() const noexcept { return fixa>0.f||minimo>0.f||maximo>0.f||suave_s>0.f||alvo>0.f||media>0.f; } };

inline std::atomic<float> medida_expo{0.f}; inline std::atomic<unsigned long long> medida_ms{0};

inline std::atomic<int> tela_ligada{0}; inline std::atomic<float> tela_media{0.f}; inline std::atomic<unsigned long long> tela_ms{0};

inline float faixa_media(double v) noexcept { return std::isfinite(v)&&v>0.&&v<1.?float(v):0.f; }
inline float faixa_tela(double v) noexcept { return std::isfinite(v)&&v>0.&&v<4.?float(v):0.f; }
inline float faixa_media_fator(double v,bool presente) noexcept { return presente&&std::isfinite(v)&&v>.1&&v<10.?float(v):1.2f; }

inline float proximo(const Config& c, float proxy, float anterior, float dt_s) noexcept {
    if(c.fixa>0.f) return c.fixa;
    if(c.media>0.f){ const float m=medida_expo.load(std::memory_order_relaxed);
        if(!(m>0.f)||!std::isfinite(m)) return anterior>0.f?anterior:proxy; proxy=m; }
    if(!std::isfinite(proxy)||proxy<=0.f) return anterior>0.f?anterior:proxy;
    float v=proxy;
    if(c.alvo>0.f){ const float w=anterior>0.f?anterior:proxy; const float a=w+4.f*(proxy-w);
        if(proxy==w) return w;
        v=(std::isfinite(a)&&a>0.f?a:proxy)*c.alvo; }
    if(c.minimo>0.f&&v<c.minimo) v=c.minimo;
    if(c.maximo>0.f&&v>c.maximo) v=c.maximo;
    if(c.suave_s>0.f&&anterior>0.f&&std::isfinite(anterior)){
        const float dt=dt_s<0.f?0.f:dt_s>1.f?1.f:dt_s;
        const float a=1.f-std::exp(-dt/c.suave_s);
        v=anterior+(v-anterior)*a;
    }
    return v;
}

struct Degraus { float segurado{}; int fora{}; };
inline constexpr float degrau_min=.05f, degrau_max=16.f;
inline float degrau(Degraus& s, float proxy, float escrito) noexcept {
    if(!std::isfinite(proxy)||proxy<=0.f) return s.segurado>0.f?s.segurado:proxy;
    if(s.segurado>0.f&&proxy==escrito) return s.segurado;
    const float w=escrito>0.f&&std::isfinite(escrito)?escrito:proxy;
    float alvo=w+4.f*(proxy-w);
    if(!std::isfinite(alvo)||alvo<=0.f) alvo=proxy;
    alvo=alvo<degrau_min?degrau_min:alvo>degrau_max?degrau_max:alvo;
    if(!(s.segurado>0.f)){ s.segurado=alvo; s.fora=0; return alvo; }
    const float d=std::log2(alvo/s.segurado);
    if(std::fabs(d)<1.f){ s.fora=0; return s.segurado; }
    if(++s.fora<3) return s.segurado;
    float v=s.segurado*std::exp2(d>.25f?.25f:d<-.25f?-.25f:d);
    v=v<degrau_min?degrau_min:v>degrau_max?degrau_max:v;
    return s.segurado=v;
}

inline bool auto_ligada(int chave, bool sem_chaves, const wchar_t* exe) noexcept {
    if(chave==0) return false;
    if(chave>0) return true;
    if(!sem_chaves||!exe) return false;
    return !nrjogo::conhecido(exe);
}
}

#if defined(_WIN32) && !defined(NREXPOTRAVA_CPU_TEST)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "log_fila.h"
#include "pasta_helper.h"
#include <cstdio>
#include <cwchar>
#include "../source/implementation/compat-diag/compat.h"
#include "nr_publico.h"
#include "ini_jogo.h"

#define NR_EXPO_LOG NR_LOG_ATIVO
namespace nrexpotrava {

inline float exposicao_rede(void* base) noexcept {
    if(!base) return -1.f; auto* b=static_cast<std::uint8_t*>(base);
    if(*reinterpret_cast<volatile std::int32_t*>(b+rva_iniciada)==0) return -1.f;
    const float v=*reinterpret_cast<volatile float*>(b+rva_expo); return std::isfinite(v)?v:-1.f; }
inline void tick(void* base, const wchar_t* ini) noexcept {
    static int lido=-1; static Config c; static float estado=0.f; static ULONGLONG t_ant=0, t_log=0; static unsigned n_log=0;
    if(!base) return;
    if(lido<0){
        if(!ini) return;
        lido=0;
        auto f=[&](const wchar_t* k)->float{ if(!ini||!*ini) return 0.f;
            double v=0.; if(!nrinijogo::ler_wcstod_em(ini,k,v,64)) return 0.f;
            return std::isfinite(v)&&v>0.&&v<1000.?float(v):0.f; };
        c.fixa=f(L"NeuralExposicaoFixa"); c.minimo=f(L"NeuralExposicaoMin"); c.maximo=f(L"NeuralExposicaoMax"); c.suave_s=f(L"NeuralExposicaoSuaveS"); c.alvo=f(L"NeuralExposicaoAlvo");
        {double v=0.; const bool tem=nrinijogo::ler_wcstod_em(ini,L"NeuralExposicaoMedia",v,64); c.media=tem?faixa_media(v):0.f;
#if NR_EXPO_LOG
         if(tem&&!(c.media>0.f)&&v!=0.) nrlogfila::linha(nrpasta::arquivo(L"nr_exposure.log").c_str(),"pid=%lu EXPO_TRAVA NeuralExposicaoMedia=%.3f fora de 0..1: ignorada (o backend nao mede)\n",GetCurrentProcessId(),v);
#endif
        }
        if(c.minimo>0.f&&c.maximo>0.f&&c.minimo>c.maximo){ const float t=c.minimo; c.minimo=c.maximo; c.maximo=t; }
        lido=c.ativa()?1:0;

        if(!lido){
            bool sem_chaves=true;
            for(const wchar_t* k:{L"NeuralExposicaoFixa",L"NeuralExposicaoMin",L"NeuralExposicaoMax",L"NeuralExposicaoSuaveS",L"NeuralExposicaoAlvo",
                                  L"NeuralExposicaoMedia",L"NeuralExposicaoTela",L"NeuralExposicaoMediaFator"}){
                wchar_t b[64]{}; GetPrivateProfileStringW(L"DlssNrOnAmd",k,L"",b,64,ini); if(*b){ sem_chaves=false; break; } }
            const int chave=int(GetPrivateProfileIntW(L"DlssNrOnAmd",L"NeuralExposicaoAuto",-1,ini));
            wchar_t exe[MAX_PATH]{}; GetModuleFileNameW(nullptr,exe,MAX_PATH);
            if(auto_ligada(chave,sem_chaves,exe)) lido=2;
        }
#if NR_EXPO_LOG
        nrlogfila::linha(nrpasta::arquivo(L"nr_exposure.log").c_str(),"pid=%lu EXPO_TRAVA %s fixa=%.3f min=%.3f max=%.3f suave_s=%.2f\n",GetCurrentProcessId(),
            lido==2?"degraus (auto: jogo sem chave de exposicao)":lido?"ligada":"desligada",double(c.fixa),double(c.minimo),double(c.maximo),double(c.suave_s));
#endif
    }
    if(!lido) return;
    auto* b=static_cast<std::uint8_t*>(base);
    auto* iniciada=reinterpret_cast<volatile std::int32_t*>(b+rva_iniciada);
    auto* expo=reinterpret_cast<volatile float*>(b+rva_expo);
    if(*iniciada==0) return;
    if(lido==2){
        static Degraus deg; static bool avisado=false;
        if(!avisado){ avisado=true; nrcompat::report("tolerado_expo_degraus","sem textura de exposicao e sem chave no INI: auto-exposicao em degraus (1 stop, 0,25/job, 0,05-16)"); }
        const float p=*expo, v=degrau(deg,p,estado);
        if(std::isfinite(v)&&v>0.f){
            const ULONGLONG agora=GetTickCount64();
#if NR_EXPO_LOG

            if(v!=estado&&n_log<720){ t_log=agora; ++n_log;
                nrlogfila::linha(nrpasta::arquivo(L"nr_exposure.log").c_str(),"pid=%lu tick=%llu EXPO_DEGRAU %.4f -> %.4f (proxy %.4f)\n",GetCurrentProcessId(),
                                 static_cast<unsigned long long>(agora),double(estado),double(v),double(p)); }
#endif
            estado=v; if(v!=p) *expo=v; }
        return;
    }
    const ULONGLONG agora=GetTickCount64(); const float p=*expo;
    if(c.alvo>0.f&&p==estado) return;
    if(c.media>0.f&&agora-medida_ms.load(std::memory_order_relaxed)>2000){ if(estado>0.f&&p!=estado) *expo=estado; return; }
    const float dt=t_ant?float(agora-t_ant)/1000.f:0.f; t_ant=agora;
    const float v=proximo(c,p,estado,dt);
    if(std::isfinite(v)&&v>0.f){ estado=v; if(v!=p) *expo=v; }
#if NR_EXPO_LOG
    if(agora-t_log>=10000&&n_log<720){ t_log=agora; ++n_log;
        nrlogfila::linha(nrpasta::arquivo(L"nr_exposure.log").c_str(),"pid=%lu EXPO_TRAVA proxy=%.4f usada=%.4f\n",GetCurrentProcessId(),double(p),double(v)); }
#else
    (void)t_log; (void)n_log;
#endif
}
}
#endif
