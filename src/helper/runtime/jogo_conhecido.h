#pragma once

#include <cstddef>
namespace nrjogo {
inline bool conhecido(const wchar_t* exe) noexcept {
    if(!exe) return true;
    static const wchar_t* const nomes[]{L"re9", L"re8", L"re4", L"re2", L"re3", L"re7", L"re9demo", L"Cyberpunk2077", L"witcher3", L"OnimushaWotS",
        L"MonsterHunterWilds", L"mini3d"};
    const wchar_t* base=exe;
    for(const wchar_t* p=exe;*p;++p) if(*p==L'\\'||*p==L'/') base=p+1;
    for(const wchar_t* k:nomes){
        std::size_t i=0;
        for(;k[i]&&base[i];++i){ wchar_t a=base[i],b=k[i]; if(a>=L'A'&&a<=L'Z')a=wchar_t(a-L'A'+L'a'); if(b>=L'A'&&b<=L'Z')b=wchar_t(b-L'A'+L'a'); if(a!=b) break; }
        if(!k[i]&&(base[i]==L'.'||base[i]==L'_'||base[i]==0)) return true;
    }
    return false;
}
}
