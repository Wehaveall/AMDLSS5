#pragma once

#include <windows.h>
#include <filesystem>
#include <mutex>
#include <string>
namespace nrpasta {
namespace detalhe {
inline const int ancora=0;
inline std::filesystem::path calcular_modulo() noexcept {
    try{
        HMODULE self{};
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ancora),&self))return {};
        std::wstring b(32768,L'\0');
        const DWORD n=GetModuleFileNameW(self,b.data(),DWORD(b.size()));
        if(!n||n>=b.size())return {};
        b.resize(n);return std::filesystem::path(b);
    }catch(...){return {};}
}
}

inline const std::filesystem::path& modulo() noexcept {
    static const std::filesystem::path* p=[]()noexcept->const std::filesystem::path*{
        try{return new std::filesystem::path(detalhe::calcular_modulo());}catch(...){return nullptr;}}();
    static const std::filesystem::path vazio;
    return p?*p:vazio;
}

inline const std::filesystem::path& helper() noexcept {
    static const std::filesystem::path* p=[]()noexcept->const std::filesystem::path*{
        try{const auto& m=modulo();return new std::filesystem::path(m.empty()?std::filesystem::path{}:m.parent_path());}catch(...){return nullptr;}}();
    static const std::filesystem::path vazio;
    return p?*p:vazio;
}

inline std::filesystem::path arquivo(const wchar_t* nome) {
    const auto& p=helper();return p.empty()?std::filesystem::path{}:p/nome;
}

inline std::filesystem::path modulo_de(const void* base) noexcept {
    static std::mutex* m=new std::mutex;static const void* guardado_base=nullptr;static std::filesystem::path* guardado=nullptr;
    try{
        std::lock_guard<std::mutex> t(*m);
        if(guardado&&guardado_base==base)return *guardado;
        std::wstring b(32768,L'\0');
        const DWORD n=GetModuleFileNameW(reinterpret_cast<HMODULE>(const_cast<void*>(base)),b.data(),DWORD(b.size()));
        if(!n||n>=b.size())return {};
        b.resize(n);
        if(!guardado)guardado=new std::filesystem::path;
        *guardado=std::filesystem::path(b);guardado_base=base;
        return *guardado;
    }catch(...){return {};}
}

inline std::filesystem::path proxy(const void* base) noexcept {
    try{const auto m=modulo_de(base);return m.empty()?std::filesystem::path{}:m.parent_path();}catch(...){return {};}
}
}
