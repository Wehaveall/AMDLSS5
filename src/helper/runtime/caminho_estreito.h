#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <filesystem>
#include <string>
namespace nrcaminho {
inline std::string estreito(const std::filesystem::path& p) noexcept {
    try{
        const UINT cp=AreFileApisANSI()?GetACP():GetOEMCP();const bool utf8=cp==CP_UTF8;
        auto conv=[&](const std::wstring& w,std::string& out)->bool{
            if(w.empty()||w.size()>0x7fffffffu)return false;
            BOOL perdeu=FALSE;const DWORD flags=utf8?0u:DWORD(WC_NO_BEST_FIT_CHARS);
            const int n=WideCharToMultiByte(cp,flags,w.c_str(),int(w.size()),nullptr,0,nullptr,utf8?nullptr:&perdeu);
            if(n<=0||perdeu)return false;
            out.assign(size_t(n),'\0');
            return WideCharToMultiByte(cp,flags,w.c_str(),int(w.size()),out.data(),n,nullptr,utf8?nullptr:&perdeu)==n&&!perdeu;};
        std::string s;
        if(conv(p.native(),s))return s;
        if(const DWORD n=GetShortPathNameW(p.c_str(),nullptr,0)){
            std::wstring curto(n,L'\0');const DWORD m=GetShortPathNameW(p.c_str(),curto.data(),n);
            if(m&&m<n){curto.resize(m);if(conv(curto,s))return s;}}
    }catch(...){}
    return {};
}
}
