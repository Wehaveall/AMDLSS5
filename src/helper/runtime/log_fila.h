#pragma once

#include "nr_publico.h"
#include <windows.h>
#include <share.h>
#include <atomic>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
namespace nrlogfila {
struct Linha {std::wstring arquivo;std::string texto;};
struct Estado {
    std::mutex m;
    std::mutex gravando;
    std::condition_variable cv;
    std::vector<Linha> fila;
    bool thread{},sincrono{},sincrono_lido{};
    unsigned long long descartadas{},gravadas{};
};

inline Estado& estado() noexcept {static Estado* e=new Estado;return *e;}
constexpr std::size_t teto_fila=8192;
constexpr DWORD intervalo_ms=50;
inline std::wstring completo(const wchar_t* arquivo){
    wchar_t buf[MAX_PATH*4]{};
    const DWORD n=GetFullPathNameW(arquivo,DWORD(sizeof(buf)/sizeof(buf[0])),buf,nullptr);
    return (n&&n<sizeof(buf)/sizeof(buf[0]))?std::wstring(buf,n):std::wstring(arquivo);
}

inline void gravar(std::vector<Linha>& lote,unsigned long long descartadas) noexcept {
    std::size_t i=0;
    while(i<lote.size()){
        std::size_t j=i;while(j<lote.size()&&lote[j].arquivo==lote[i].arquivo)++j;
        FILE* f=_wfsopen(lote[i].arquivo.c_str(),L"a",_SH_DENYNO);
        if(f){
            for(std::size_t k=i;k<j;++k)std::fputs(lote[k].texto.c_str(),f);
            if(descartadas){std::fprintf(f,"pid=%lu LOG_FILA descartadas=%llu\n",GetCurrentProcessId(),descartadas);descartadas=0;}
            std::fclose(f);
        }
        i=j;
    }
    estado().gravadas+=lote.size();
}
inline void esvaziar_travado(Estado& e) noexcept {
    std::vector<Linha> lote;unsigned long long d=0;
    {std::lock_guard<std::mutex> t(e.m);lote.swap(e.fila);d=e.descartadas;e.descartadas=0;}
    if(!lote.empty())gravar(lote,d);
}

inline void esvaziar() noexcept {
    try{auto& e=estado();std::lock_guard<std::mutex> g(e.gravando);esvaziar_travado(e);}catch(...){}
}

inline bool travar_na_saida(std::mutex& m) noexcept {
    for(int i=0;i<200;++i){if(m.try_lock())return true;Sleep(1);}
    return false;
}
inline void esvaziar_na_saida() noexcept {
    try{
        auto& e=estado();
        if(!travar_na_saida(e.gravando))return;
        std::vector<Linha> lote;unsigned long long d=0;
        if(travar_na_saida(e.m)){lote.swap(e.fila);d=e.descartadas;e.descartadas=0;e.m.unlock();}
        if(!lote.empty())gravar(lote,d);
        e.gravando.unlock();
    }catch(...){}
}
inline void escritor() noexcept {
    auto& e=estado();
    for(;;){
        try{
            {std::unique_lock<std::mutex> t(e.m);e.cv.wait(t,[&]{return !e.fila.empty();});}
            Sleep(intervalo_ms);
            std::lock_guard<std::mutex> g(e.gravando);esvaziar_travado(e);
        }catch(...){Sleep(intervalo_ms);}
    }
}
inline bool sincrono(Estado& e) noexcept {
    if(!e.sincrono_lido){e.sincrono_lido=true;
        wchar_t v[8]{};e.sincrono=GetEnvironmentVariableW(L"NR_LOG_FILA_SINCRONO",v,8)>0&&v[0]==L'1';}
    return e.sincrono;
}

inline void escrever(const wchar_t* arquivo,std::string texto) noexcept {
    if(texto.empty()||!arquivo)return;
    try{
        auto& e=estado();Linha l{completo(arquivo),std::move(texto)};
        bool direto=false,acordar=false;
        {std::lock_guard<std::mutex> t(e.m);
         if(sincrono(e))direto=true;
         else{
            if(!e.thread){
                try{std::thread(escritor).detach();e.thread=true;std::atexit(esvaziar_na_saida);}catch(...){e.sincrono=true;direto=true;}
            }
            if(!direto){
                if(e.fila.size()>=teto_fila){++e.descartadas;return;}
                e.fila.push_back(std::move(l));acordar=e.fila.size()==1;
            }
         }}
        if(direto){
            std::lock_guard<std::mutex> g(e.gravando);esvaziar_travado(e);
            std::vector<Linha> um;um.push_back(std::move(l));gravar(um,0);return;
        }
        if(acordar)e.cv.notify_one();
    }catch(...){}
}

inline void linha(const wchar_t* arquivo,const char* formato,...) noexcept {
    try{
        char pilha[512];va_list a;va_start(a,formato);const int n=std::vsnprintf(pilha,sizeof pilha,formato,a);va_end(a);
        if(n<0)return;
        if(std::size_t(n)<sizeof pilha){escrever(arquivo,std::string(pilha,std::size_t(n)));return;}
        std::string s(std::size_t(n)+1,'\0');va_start(a,formato);std::vsnprintf(s.data(),s.size(),formato,a);va_end(a);s.resize(std::size_t(n));
        escrever(arquivo,std::move(s));
    }catch(...){}
}
}
