#pragma once

#include "nr_publico.h"
#include "ini_jogo.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#include <cmath>
#include <cwchar>
namespace nrrjcl {
inline std::atomic<bool> cor{true};

inline std::atomic<float> cor_f{1.f};
inline std::atomic<bool> luz{true};
inline std::atomic<float> sigma{0.f};
inline std::atomic<bool> menu_escreveu{false};
inline std::atomic<bool> ini_lido{false};
inline constexpr float razao_max=4.f;

inline bool valor_de(const wchar_t* t) noexcept {if(!t||!t[0])return true;wchar_t* f=nullptr;const long v=std::wcstol(t,&f,10);return f==t||v!=0;}

inline float forca_de(const wchar_t* t) noexcept {if(!t||!t[0])return 1.f;wchar_t* f=nullptr;const double v=std::wcstod(t,&f);if(f==t||!std::isfinite(v))return 1.f;return float(v<0.?0.:v>1.?1.:v);}
inline void cor_publicar(float f) noexcept {cor_f.store(f,std::memory_order_relaxed);cor.store(f>.0005f,std::memory_order_relaxed);}

inline bool compoe_cor() noexcept {return cor_f.load(std::memory_order_relaxed)<.9995f;}
inline unsigned peso_jogo_8() noexcept {const float g=1.f-cor_f.load(std::memory_order_relaxed);return unsigned(std::lround((g<0.f?0.f:g>1.f?1.f:g)*255.f));}

inline unsigned modo() noexcept {return (cor.load(std::memory_order_relaxed)?0u:1u)|(luz.load(std::memory_order_relaxed)?0u:2u);}
inline bool ativo() noexcept {return modo()!=0u;}

inline std::atomic<float> cor_estilo[3]{-1.f,-1.f,-1.f};
inline std::atomic<int> luz_estilo[3]{-1,-1,-1};
inline std::atomic<float> forca_estilo[3]{0.f,0.f,0.f};
inline std::atomic<float> cor_base{1.f};inline std::atomic<bool> luz_base{true};
inline void ler_estilos(const wchar_t* ini) noexcept {
    wchar_t t[32]{};NRCFG_STR(L"DlssNrOnAmd",L"NeuralRostoCor",L"1",t,32,ini);cor_base.store(forca_de(t));
    NRCFG_STR(L"DlssNrOnAmd",L"NeuralRostoLuz",L"1",t,32,ini);luz_base.store(valor_de(t));
    for(int s=0;s<3;++s){
        wchar_t k[48]{};
        std::swprintf(k,48,L"NeuralRostoCorEstilo%d",s);if(NRCFG_STR(L"DlssNrOnAmd",k,L"",t,32,ini)>0)cor_estilo[s].store(forca_de(t));
        std::swprintf(k,48,L"NeuralRostoLuzEstilo%d",s);if(NRCFG_STR(L"DlssNrOnAmd",k,L"",t,32,ini)>0)luz_estilo[s].store(valor_de(t)?1:0);
        std::swprintf(k,48,L"NeuralRostoJogadorPasso1ForcaEstilo%d",s);
        {double v{};if(nrinijogo::ler_double_em(ini,k,v,32)&&v>=.25&&v<=4.)forca_estilo[s].store(float(v));}
    }
}
inline bool tem_estilo() noexcept {for(int s=0;s<3;++s)if(cor_estilo[s].load()>=0.f||luz_estilo[s].load()>=0||forca_estilo[s].load()>0.f)return true;return false;}

template<class T> struct CampoEstilo {T aplicado{};bool de_chave{false};};
template<class T> inline T campo_estilo(CampoEstilo<T>& c,T em_uso,bool tem_chave,T chave,T base) noexcept {
    T v;
    if(tem_chave){v=chave;c.de_chave=true;}
    else if(c.de_chave&&em_uso==c.aplicado){v=base;c.de_chave=false;}
    else{v=em_uso;c.de_chave=false;}
    c.aplicado=v;return v;}
inline void ler(const wchar_t* ini) noexcept {
    if(!ini)return;
    ler_estilos(ini);
    if(!menu_escreveu.load()){wchar_t t[16]{};NRCFG_STR(L"DlssNrOnAmd",L"NeuralRostoCor",L"1",t,16,ini);cor_publicar(forca_de(t));
        NRCFG_STR(L"DlssNrOnAmd",L"NeuralRostoLuz",L"1",t,16,ini);luz.store(valor_de(t));}
    {double v{};sigma.store(nrinijogo::ler_double_em(ini,L"NeuralRostoLuzSigma",v,32)&&v>=4.&&v<=2048.?float(v):0.f);}
    ini_lido.store(true,std::memory_order_release);
}

inline bool em_uso(const std::atomic<bool>& a,const wchar_t* texto_ini) noexcept {return (menu_escreveu.load()||ini_lido.load())?a.load():valor_de(texto_ini);}
}
