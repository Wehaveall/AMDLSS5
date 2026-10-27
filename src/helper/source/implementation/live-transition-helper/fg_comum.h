#pragma once

#include <windows.h>
#include <cstring>
#include <filesystem>
#include <string>
namespace nrfg {
inline std::filesystem::path pasta_de(HMODULE m) noexcept {
    try{std::wstring b(32768,L'\0');const DWORD n=GetModuleFileNameW(m,b.data(),DWORD(b.size()));
        if(!n||n>=b.size())return {};b.resize(n);return std::filesystem::path(b).parent_path();}catch(...){return {};}
}
inline HMODULE provedor_fg(const std::wstring& ini) noexcept {
    if(HMODULE fg=GetModuleHandleW(L"amd_fidelityfx_framegeneration_dx12.dll"))return fg;
    try{
        std::filesystem::path pastas[3];
        if(HMODULE ld=GetModuleHandleW(L"amd_fidelityfx_loader_dx12.dll"))pastas[0]=pasta_de(ld);
        pastas[1]=pasta_de(nullptr);
        if(!ini.empty())pastas[2]=std::filesystem::path(ini).parent_path();
        for(const auto& d:pastas){
            if(d.empty())continue;
            const auto f=d/L"amd_fidelityfx_framegeneration_dx12.dll";std::error_code ec;
            if(std::filesystem::is_regular_file(f,ec))if(HMODULE fg=LoadLibraryW(f.c_str()))return fg;
        }
    }catch(...){}
    return nullptr;
}

inline bool gravar_codigo(void* alvo,const void* bytes,std::size_t n) noexcept {
    DWORD velho=0;if(!VirtualProtect(alvo,n,PAGE_EXECUTE_READWRITE,&velho))return false;
    std::memcpy(alvo,bytes,n);DWORD x=0;VirtualProtect(alvo,n,velho,&x);FlushInstructionCache(GetCurrentProcess(),alvo,n);
    return true;
}
}
