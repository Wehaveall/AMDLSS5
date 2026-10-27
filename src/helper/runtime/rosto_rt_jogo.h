#pragma once

#include <windows.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cwctype>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
namespace nrrtjogo {

inline bool igual_ci(std::string_view a,std::string_view b) noexcept {
    if(a.size()!=b.size())return false;
    for(size_t i=0;i<a.size();++i){char x=a[i],y=b[i];if(x>='A'&&x<='Z')x=char(x-'A'+'a');if(y>='A'&&y<='Z')y=char(y-'A'+'a');if(x!=y)return false;}
    return true;
}
inline std::string_view apara(std::string_view s) noexcept {
    while(!s.empty()&&(s.front()==' '||s.front()=='\t'||s.front()=='\r'||s.front()=='\n'||s.front()=='"'))s.remove_prefix(1);
    while(!s.empty()&&(s.back()==' '||s.back()=='\t'||s.back()=='\r'||s.back()=='\n'||s.back()=='"'))s.remove_suffix(1);
    return s;
}

inline int rt_do_texto(std::string_view txt) noexcept {
    int r=-1;size_t i=0;
    while(i<txt.size()){
        size_t f=txt.find('\n',i);if(f==std::string_view::npos)f=txt.size();
        std::string_view l=apara(txt.substr(i,f-i));i=f+1;
        if(l.empty()||l[0]==';'||l[0]=='#'||l[0]=='[')continue;
        const size_t eq=l.find('=');if(eq==std::string_view::npos)continue;
        const std::string_view k=apara(l.substr(0,eq)),v=apara(l.substr(eq+1));
        if(!igual_ci(k,"RayTracingSetting")&&!igual_ci(k,"RayTracing"))continue;
        r=(v.empty()||igual_ci(v,"Off")||igual_ci(v,"Disable")||igual_ci(v,"Disabled")||igual_ci(v,"False")||igual_ci(v,"None")||v=="0")?0:1;
    }
    return r;
}
struct Leitor {
    std::mutex m;std::wstring caminho;bool tem_caminho{};
    std::chrono::steady_clock::time_point ultima{};bool ja{};
    FILETIME data{};std::uint64_t tamanho{};int estado{-1};std::string valor;
    std::atomic<int> atual{-1};
};
inline Leitor& leitor() {static auto* l=new Leitor;return *l;}
inline std::wstring config_do_exe() {
    wchar_t p[32768];const DWORD n=GetModuleFileNameW(nullptr,p,32768);
    if(n==0||n>=32768)return {};
    std::wstring s(p,n);const size_t b=s.find_last_of(L"\\/");if(b==std::wstring::npos)return {};
    return s.substr(0,b+1)+L"config.ini";
}

inline int estado(bool* mudou=nullptr,std::string* valor=nullptr) noexcept {
    if(mudou)*mudou=false;
    try{
        auto& L=leitor();std::lock_guard<std::mutex> g(L.m);
        const auto ag=std::chrono::steady_clock::now();
        if(L.ja&&ag-L.ultima<std::chrono::seconds(1)){if(valor)*valor=L.valor;return L.estado;}
        L.ja=true;L.ultima=ag;
        if(!L.tem_caminho){L.caminho=config_do_exe();L.tem_caminho=true;}
        int novo=-1;std::string val;
        WIN32_FILE_ATTRIBUTE_DATA a{};
        if(!L.caminho.empty()&&GetFileAttributesExW(L.caminho.c_str(),GetFileExInfoStandard,&a)){
            const std::uint64_t tam=(std::uint64_t(a.nFileSizeHigh)<<32)|a.nFileSizeLow;
            if(CompareFileTime(&a.ftLastWriteTime,&L.data)==0&&tam==L.tamanho){if(valor)*valor=L.valor;return L.estado;}
            if(tam<=(4u<<20)){std::ifstream f(L.caminho,std::ios::binary);std::string t((std::istreambuf_iterator<char>(f)),{});
                if(f.good()||f.eof()){novo=rt_do_texto(t);
                    const size_t k=t.find("RayTracingSetting=");if(k!=std::string::npos){size_t e=t.find_first_of("\r\n",k);val=t.substr(k+18,(e==std::string::npos?t.size():e)-(k+18));}}}
            L.data=a.ftLastWriteTime;L.tamanho=tam;
        }else{L.data={};L.tamanho=0;}
        if(mudou)*mudou=(novo!=L.estado)||(val!=L.valor);
        L.estado=novo;L.valor=val;L.atual.store(novo,std::memory_order_relaxed);
        if(valor)*valor=val;
        return novo;
    }catch(...){return -1;}
}
}
