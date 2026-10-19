#pragma once

#include <atomic>
#include <cmath>
#include <cstdint>

namespace nrexpotrava {
inline constexpr std::uint32_t rva_expo=0x94A94, rva_iniciada=0x94A98;
struct Config { float fixa{}, minimo{}, maximo{}, suave_s{}, alvo{}, media{}; bool ativa() const noexcept { return fixa>0.f||minimo>0.f||maximo>0.f||suave_s>0.f||alvo>0.f||media>0.f; } };

inline std::atomic<float> medida_expo{0.f}; inline std::atomic<unsigned long long> medida_ms{0};

inline std::atomic<int> tela_ligada{0}; inline std::atomic<float> tela_media{0.f}; inline std::atomic<unsigned long long> tela_ms{0};

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
}

#if defined(_WIN32) && !defined(NREXPOTRAVA_CPU_TEST)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <cstdio>
#include <cwchar>
namespace nrexpotrava {

inline void tick(void* base, const wchar_t* ini) noexcept {
    static int lido=-1; static Config c; static float estado=0.f; static ULONGLONG t_ant=0, t_log=0; static unsigned n_log=0;
    if(!base) return;
    if(lido<0){
        if(!ini) return;
        lido=0;
        auto f=[&](const wchar_t* k)->float{ wchar_t b[64]{}; if(!ini||!*ini) return 0.f;
            GetPrivateProfileStringW(L"DlssNrOnAmd",k,L"",b,64,ini); if(!*b) return 0.f; const double v=std::wcstod(b,nullptr);
            return std::isfinite(v)&&v>0.&&v<1000.?float(v):0.f; };
        c.fixa=f(L"NeuralExposicaoFixa"); c.minimo=f(L"NeuralExposicaoMin"); c.maximo=f(L"NeuralExposicaoMax"); c.suave_s=f(L"NeuralExposicaoSuaveS"); c.alvo=f(L"NeuralExposicaoAlvo"); c.media=f(L"NeuralExposicaoMedia");
        if(c.minimo>0.f&&c.maximo>0.f&&c.minimo>c.maximo){ const float t=c.minimo; c.minimo=c.maximo; c.maximo=t; }
        lido=c.ativa()?1:0;
        FILE* l{}; if(fopen_s(&l,"nr_exposure.log","a")==0&&l){ std::fprintf(l,"pid=%lu EXPO_TRAVA %s fixa=%.3f min=%.3f max=%.3f suave_s=%.2f\n",GetCurrentProcessId(),
            lido?"ligada":"desligada",double(c.fixa),double(c.minimo),double(c.maximo),double(c.suave_s)); std::fclose(l); }
    }
    if(lido!=1) return;
    auto* b=static_cast<std::uint8_t*>(base);
    auto* iniciada=reinterpret_cast<volatile std::int32_t*>(b+rva_iniciada);
    auto* expo=reinterpret_cast<volatile float*>(b+rva_expo);
    if(*iniciada==0) return;
    const ULONGLONG agora=GetTickCount64(); const float p=*expo;
    if(c.alvo>0.f&&p==estado) return;
    if(c.media>0.f&&agora-medida_ms.load(std::memory_order_relaxed)>2000){ if(estado>0.f&&p!=estado) *expo=estado; return; }
    const float dt=t_ant?float(agora-t_ant)/1000.f:0.f; t_ant=agora;
    const float v=proximo(c,p,estado,dt);
    if(std::isfinite(v)&&v>0.f){ estado=v; if(v!=p) *expo=v; }
    if(agora-t_log>=10000&&n_log<720){ t_log=agora; ++n_log;
        FILE* l{}; if(fopen_s(&l,"nr_exposure.log","a")==0&&l){ std::fprintf(l,"pid=%lu EXPO_TRAVA proxy=%.4f usada=%.4f\n",GetCurrentProcessId(),double(p),double(v)); std::fclose(l); } }
}
}
#endif
