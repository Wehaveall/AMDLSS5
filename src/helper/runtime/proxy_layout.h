#pragma once

#include <cstdint>
#include <atomic>
#include <vector>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "proxy_rva.h"
#include "../source/implementation/live-transition-image/live_image.h"
#include "../source/implementation/active-binding/memory_contract.h"
#include "native_ini_path.h"
#include "ini_jogo.h"
#include "pasta_helper.h"
#include <cstring>
#include <string>
namespace nrproxy {

inline std::wstring ini(void* base) {
    if(nrinijogo::pronto().load(std::memory_order_acquire))return nrinijogo::guardado();
    return nrinipath::resolve(base,&nrpasta::detalhe::ancora);
}

template<class T> T field(const nractive::Globals& g,unsigned r) noexcept {T v{};std::memcpy(&v,g.data()+r-nractive::globals_begin,sizeof(v));return v;}
inline bool copy(const void* p,void* out,std::size_t n) noexcept {SIZE_T done=0;return p&&ReadProcessMemory(GetCurrentProcess(),p,out,n,&done)&&done==n;}

inline bool admitido(void* base) noexcept {
    static std::atomic<std::uintptr_t> guardado{0};
    const auto b=reinterpret_cast<std::uintptr_t>(base);
    if(!b||(b&1u))return false;
    const auto g=guardado.load(std::memory_order_acquire);
    if(g&&(g&~std::uintptr_t(1))==b)return (g&1u)!=0;
    bool ok=false;
    try{
        HMODULE modulo{};
        if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,static_cast<LPCWSTR>(base),&modulo)&&
           reinterpret_cast<std::uintptr_t>(modulo)==b){
            std::vector<wchar_t> caminho(32768);
            const DWORD n=GetModuleFileNameW(modulo,caminho.data(),32768);
            ok=n&&n<32768&&nrliveimage::disk_file_admitted(caminho.data());
        }
    }catch(...){ok=false;}
    guardado.store(b|(ok?1u:0u),std::memory_order_release);
    return ok;
}
inline volatile std::uint8_t& menu_visivel(void* base) noexcept {return *reinterpret_cast<volatile std::uint8_t*>(static_cast<std::uint8_t*>(base)+rva_menu_visivel);}
inline volatile std::uint8_t& enabled(void* base) noexcept {return *reinterpret_cast<volatile std::uint8_t*>(static_cast<std::uint8_t*>(base)+rva_enabled);}

inline void avisar_menu_fechado(void* base) noexcept {
    if(!admitido(base))return;
    auto* b=static_cast<std::uint8_t*>(base);
    if(void* obj=*reinterpret_cast<void* volatile*>(b+rva_menu_aviso)){int novo=0;using Aviso=void(__fastcall*)(void*,int*);(*reinterpret_cast<Aviso**>(obj))[2](obj,&novo);}
}
}
