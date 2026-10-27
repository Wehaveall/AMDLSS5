#pragma once
#include "nr_publico.h"

#include <windows.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
namespace nrinijogo {
inline std::wstring& guardado() noexcept { static std::wstring s; return s; }
inline std::atomic<bool>& pronto() noexcept { static std::atomic<bool> p{false}; return p; }

inline void definir(const std::wstring& caminho) noexcept {
    if(caminho.empty()||pronto().load(std::memory_order_acquire))return;
    try{guardado()=caminho;pronto().store(true,std::memory_order_release);}catch(...){}
}
inline const wchar_t* caminho() noexcept {
    return pronto().load(std::memory_order_acquire)?guardado().c_str():L".\\dlssnr_on_amd.ini";
}

inline bool numero(const wchar_t* texto,double& v) noexcept {
    if(!texto||!*texto)return false;
    wchar_t* fim{};const double lido=std::wcstod(texto,&fim);
    if(fim==texto||!std::isfinite(lido))return false;
    v=lido;return true;
}

inline bool numero_f(const wchar_t* texto,float& v) noexcept {
    if(!texto||!*texto)return false;
    wchar_t* fim{};const float lido=std::wcstof(texto,&fim);
    if(fim==texto||!std::isfinite(lido))return false;
    v=lido;return true;
}

inline bool ler_double_em(const wchar_t* ini,const wchar_t* chave,double& v,unsigned n=32) noexcept {
    wchar_t texto[256]{};if(!ini||n<2||n>256)return false;
    if(!NRCFG_STR(L"DlssNrOnAmd",chave,L"",texto,n,ini))return false;
    return numero(texto,v);
}
inline bool ler_float_em(const wchar_t* ini,const wchar_t* chave,float& v,unsigned n=64) noexcept {
    wchar_t texto[256]{};if(!ini||n<2||n>256)return false;
    if(!NRCFG_STR(L"DlssNrOnAmd",chave,L"",texto,n,ini))return false;
    return numero_f(texto,v);
}

inline float passo_extra_cam_px(const wchar_t* ini) noexcept {
    double v{};if(!ler_double_em(ini,L"NeuralPassoExtraCamPx",v,32)||!(v>0.))return 0.f;
    return v>64.?64.f:float(v);
}

inline bool ler_wcstod_em(const wchar_t* ini,const wchar_t* chave,double& v,unsigned n=32) noexcept {
    wchar_t texto[256]{};if(!ini||n<2||n>256)return false;
    if(!NRCFG_STR(L"DlssNrOnAmd",chave,L"",texto,n,ini))return false;
    v=std::wcstod(texto,nullptr);return true;
}

inline float ler_real_limitado_em(const wchar_t* ini,const wchar_t* chave,float padrao,float lo,float hi,unsigned n=32) noexcept {
    double d{};if(!ler_wcstod_em(ini,chave,d,n))return padrao;
    const float v=float(d);if(!std::isfinite(v))return padrao;
    return v<lo?lo:(hi<v?hi:v);
}

namespace detalhe {
struct Carimbo {
    std::wstring completo;bool existe{};unsigned long long tamanho{},escrita{};
    bool operator==(const Carimbo& o)const{return existe==o.existe&&tamanho==o.tamanho&&escrita==o.escrita&&completo==o.completo;}
};
struct Estatisticas {unsigned long long conferencias{},aberturas{},esquecimentos{},leituras_api{},acertos{};};
struct Cache {
    std::mutex m;bool valido{},confiavel{};Carimbo carimbo;std::string bytes;
    std::unordered_map<std::wstring,std::pair<bool,std::wstring>> chaves;Estatisticas est;
};

inline Cache& cache() noexcept {static Cache* c=new Cache;return *c;}
inline thread_local int lote_profundidade=0;
inline thread_local bool lote_conferido=false;
constexpr unsigned long long margem_confiavel=30000000ull;
constexpr DWORD maior_ini=4u<<20;
inline unsigned long long u64(DWORD alto,DWORD baixo){return (static_cast<unsigned long long>(alto)<<32)|baixo;}
inline bool ler_bytes(const std::wstring& arquivo,std::string& out){
    out.clear();
    const HANDLE h=CreateFileW(arquivo.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE)return false;
    bool ok=true;LARGE_INTEGER t{};
    if(!GetFileSizeEx(h,&t)||t.QuadPart<0||t.QuadPart>maior_ini)ok=false;
    if(ok){
        out.resize(static_cast<std::size_t>(t.QuadPart));DWORD total=0;
        while(ok&&total<out.size()){DWORD lido=0;if(!ReadFile(h,out.data()+total,DWORD(out.size()-total),&lido,nullptr)||!lido)ok=false;else total+=lido;}
        if(ok){char extra=0;DWORD lido=0;if(ReadFile(h,&extra,1,&lido,nullptr)&&lido)ok=false;}
    }
    CloseHandle(h);
    if(!ok)out.clear();
    return ok;
}

inline void conferir(Cache& c){
    ++c.est.conferencias;
    Carimbo novo;
    {const wchar_t* nome=caminho();wchar_t completo[MAX_PATH*4]{};
     const DWORD n=GetFullPathNameW(nome,DWORD(sizeof(completo)/sizeof(completo[0])),completo,nullptr);
     novo.completo=(n&&n<sizeof(completo)/sizeof(completo[0]))?std::wstring(completo,n):std::wstring(nome);}
    WIN32_FILE_ATTRIBUTE_DATA a{};
    if(GetFileAttributesExW(novo.completo.c_str(),GetFileExInfoStandard,&a)&&!(a.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)){
        novo.existe=true;novo.tamanho=u64(a.nFileSizeHigh,a.nFileSizeLow);novo.escrita=u64(a.ftLastWriteTime.dwHighDateTime,a.ftLastWriteTime.dwLowDateTime);
    }
    if(c.valido&&c.confiavel&&novo==c.carimbo)return;
    std::string bytes;bool lidos=true;
    if(novo.existe){++c.est.aberturas;lidos=ler_bytes(novo.completo,bytes);}
    const bool igual=c.valido&&lidos&&novo.completo==c.carimbo.completo&&novo.existe==c.carimbo.existe&&bytes==c.bytes;
    if(!igual){c.chaves.clear();++c.est.esquecimentos;}
    FILETIME agora{};GetSystemTimeAsFileTime(&agora);const unsigned long long t=u64(agora.dwHighDateTime,agora.dwLowDateTime);
    c.carimbo=std::move(novo);c.bytes=std::move(bytes);

    c.valido=lidos;
    c.confiavel=lidos&&(!c.carimbo.existe||(t>c.carimbo.escrita&&t-c.carimbo.escrita>margem_confiavel));
}
}

struct Lote {
    Lote() noexcept {if(detalhe::lote_profundidade++==0)detalhe::lote_conferido=false;}
    ~Lote() {if(--detalhe::lote_profundidade==0)detalhe::lote_conferido=false;}
    Lote(const Lote&)=delete;Lote& operator=(const Lote&)=delete;
};

inline bool ler_texto(const wchar_t* chave,wchar_t* out,unsigned n) noexcept {
    if(!out||!n)return false;out[0]=L'\0';
    try{
        auto& c=detalhe::cache();std::lock_guard<std::mutex> trava(c.m);
        if(!detalhe::lote_profundidade||!detalhe::lote_conferido){
            detalhe::conferir(c);if(detalhe::lote_profundidade)detalhe::lote_conferido=true;
        }
        auto it=c.chaves.find(chave);
        if(it==c.chaves.end()){
            wchar_t texto[256]{};
            const DWORD k=NRCFG_STR(L"DlssNrOnAmd",chave,L"",texto,256,caminho());++c.est.leituras_api;
            it=c.chaves.emplace(chave,std::make_pair(k>0,std::wstring(texto,k<256?k:255))).first;
        }else ++c.est.acertos;
        const auto& t=it->second.second;const std::size_t copia=t.size()<n-1?t.size():n-1;
        if(copia)std::wmemcpy(out,t.data(),copia);out[copia]=L'\0';
        return copia>0;
    }catch(...){

        return NRCFG_STR(L"DlssNrOnAmd",chave,L"",out,n,caminho())>0;
    }
}

inline bool ler_double(const wchar_t* chave,double& v) noexcept {
    wchar_t texto[32]{};
    if(!ler_texto(chave,texto,32))return false;
    return numero(texto,v);
}

inline detalhe::Estatisticas estatisticas() noexcept {auto& c=detalhe::cache();std::lock_guard<std::mutex> t(c.m);return c.est;}
inline void esquecer() noexcept {auto& c=detalhe::cache();std::lock_guard<std::mutex> t(c.m);c.chaves.clear();c.valido=c.confiavel=false;c.bytes.clear();c.est={};}
}
