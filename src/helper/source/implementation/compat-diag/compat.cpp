#include "../../../runtime/nr_publico.h"
#include "compat.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "../../../runtime/pasta_helper.h"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <mutex>

namespace nrcompat {
namespace {

constexpr bool minimo=!NR_LOG_ATIVO;
std::atomic<unsigned> contador_tolerados{0};
constexpr unsigned origens=unsigned(Origem::Count);
const char* const nome_origem[origens]{"binding","descritor","admissao","captura","composicao"};
struct Estado {
    std::mutex mutex;
    Limiter limiter;
    bool cabecalho{};
    teste::Sink sink{};
    Motivo ativo[origens]{};
    std::atomic<bool> tem[origens]{};
};
Estado& estado() noexcept {static Estado e;return e;}
void copiar(char* out,std::size_t n,const char* s) noexcept {
    if(!out||!n)return;
    std::size_t i=0;if(s)for(;s[i]&&i+1<n;++i)out[i]=s[i];
    out[i]=0;
}

void limpar(const char* id,char* out,std::size_t n) noexcept {
    copiar(out,n,id);for(std::size_t i=0;i<n&&out[i];++i)if(out[i]==' ')out[i]='_';
}

struct Linha { bool ok{}; bool cabecalho{}; teste::Sink sink{}; char texto[4200]; };
bool montar(Estado& e,const char* id,const char* detalhe,Linha& l) noexcept {
    if constexpr(minimo){if(!e.sink)return false;}
    char gid[64];limpar(id,gid,sizeof gid);
    if(!e.limiter.admit(gid))return false;
    std::snprintf(l.texto,sizeof l.texto,"pid=%lu tick=%llu thread=%lu id=%s %s\n",GetCurrentProcessId(),
        static_cast<unsigned long long>(GetTickCount64()),GetCurrentThreadId(),gid,detalhe?detalhe:"");
    l.sink=e.sink;l.cabecalho=!e.sink&&!e.cabecalho;if(l.cabecalho)e.cabecalho=true;
    return l.ok=true;
}

void gravar(const Linha& l) noexcept {
    if(!l.ok)return;
    if(l.sink){l.sink(l.texto);return;}
}
}

unsigned Limiter::por_id_efetivo() noexcept {return minimo?1u:per_id;}
unsigned Limiter::total_efetivo() noexcept {return minimo?16u:total;}
bool Limiter::admit(const char* id) noexcept {
    const unsigned lim_id=por_id_efetivo(),lim_total=total_efetivo();
    if(lines_>=lim_total)return false;
    char key[sizeof(Entry::id)];copiar(key,sizeof key,id);
    for(unsigned i=0;i<used_;++i)if(std::strcmp(entries_[i].id,key)==0){
        if(entries_[i].count>=lim_id)return false;
        ++entries_[i].count;++lines_;return true;
    }
    if(used_>=total)return false;
    copiar(entries_[used_].id,sizeof(Entry::id),key);entries_[used_].count=1;++used_;++lines_;return true;
}

void gate_id(const char* s,char* out,std::size_t n) noexcept {
    if(!out||!n)return;
    std::size_t o=0;
    if(s)for(std::size_t i=0;s[i]&&o+1<n;++i){
        if(s[i]=='_'&&i>0){
            const char next=s[i+1];
            if(next>='0'&&next<='9')break;
            if(std::strncmp(s+i,"_rva_",5)==0||std::strncmp(s+i,"_win32_",7)==0)break;
        }
        out[o++]=s[i]==' '?'_':s[i];
    }
    out[o]=0;
}

void report(const char* id,const char* detalhe) noexcept {
    try{auto& e=estado();Linha l;{std::lock_guard<std::mutex> lock(e.mutex);montar(e,id,detalhe,l);}gravar(l);}catch(...){}
}
void tolerado(const char* id,const char* detalhe) noexcept {
    contador_tolerados.fetch_add(1,std::memory_order_relaxed);report(id,detalhe);
}
unsigned tolerados() noexcept {return contador_tolerados.load(std::memory_order_relaxed);}
bool blocked(Origem origem) noexcept {
    const unsigned o=unsigned(origem);return o<origens&&estado().tem[o].load(std::memory_order_relaxed);
}
void block(Origem origem,const char* id,const char* detalhe,const char* campo) noexcept {
    const unsigned o=unsigned(origem);if(o>=origens||!id)return;
    try{
        auto& e=estado();Linha l;
        {
            std::lock_guard<std::mutex> lock(e.mutex);
            char gid[sizeof(Motivo::id)];limpar(id,gid,sizeof gid);
            if(e.tem[o].load(std::memory_order_relaxed)&&std::strcmp(e.ativo[o].id,gid)==0)return;
            copiar(e.ativo[o].id,sizeof(Motivo::id),gid);copiar(e.ativo[o].campo,sizeof(Motivo::campo),campo);
            e.tem[o].store(true,std::memory_order_release);
            char texto[1200];
            std::snprintf(texto,sizeof texto,"origem=%s campo=\"%s\" %s",nome_origem[o],campo?campo:"",detalhe?detalhe:"");
            montar(e,gid,texto,l);
        }
        gravar(l);
    }catch(...){}
}
void unblock(Origem origem) noexcept {
    const unsigned o=unsigned(origem);if(o>=origens)return;
    auto& e=estado();if(!e.tem[o].load(std::memory_order_relaxed))return;
    try{
        Linha l;
        {
            std::lock_guard<std::mutex> lock(e.mutex);if(!e.tem[o].load(std::memory_order_relaxed))return;
            char texto[160];std::snprintf(texto,sizeof texto,"origem=%s motivo=%s",nome_origem[o],e.ativo[o].id);
            e.ativo[o]={};e.tem[o].store(false,std::memory_order_release);
            montar(e,"recuperado",texto,l);
        }
        gravar(l);
    }catch(...){}
}
Motivo last_reason() noexcept {
    Motivo m{};auto& e=estado();
    bool algum=false;for(unsigned o=0;o<origens;++o)algum|=e.tem[o].load(std::memory_order_acquire);
    if(!algum)return m;
    try{std::lock_guard<std::mutex> lock(e.mutex);
        for(unsigned o=0;o<origens;++o)if(e.tem[o].load(std::memory_order_relaxed)){m=e.ativo[o];break;}
    }catch(...){}
    return m;
}
void module_of(const void* address,char* out,std::size_t n) noexcept {
    HMODULE module{};wchar_t path[MAX_PATH]{};
    if(!address||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,static_cast<LPCWSTR>(address),&module)){NRLOGF(out,n,"none");return;}
    const DWORD length=GetModuleFileNameW(module,path,MAX_PATH);
    if(!length||length>=MAX_PATH){NRLOGF(out,n,"unnamed@%p",static_cast<void*>(module));return;}
    const wchar_t* name=std::wcsrchr(path,L'\\');name=name?name+1:path;
    NRLOGF(out,n,"%ls@%p",name,static_cast<void*>(module));
}
void region_of(const void* address,char* out,std::size_t n) noexcept {
    MEMORY_BASIC_INFORMATION info{};
    if(!address||!VirtualQuery(address,&info,sizeof(info))){NRLOGF(out,n,"query_failed");return;}
    NRLOGF(out,n,"alloc_%p_state_%lX_type_%lX_protect_%lX",info.AllocationBase,info.State,info.Type,info.Protect);
}
namespace teste {
void set_sink(Sink sink) noexcept {auto& e=estado();std::lock_guard<std::mutex> lock(e.mutex);e.sink=sink;}
void reset() noexcept {
    auto& e=estado();std::lock_guard<std::mutex> lock(e.mutex);
    e.limiter=Limiter{};e.cabecalho=false;
    for(unsigned o=0;o<origens;++o){e.ativo[o]={};e.tem[o].store(false);}
}
}
}
