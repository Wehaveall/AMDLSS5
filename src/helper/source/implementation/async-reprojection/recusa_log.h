#pragma once

#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <process.h>
#include "../../../runtime/log_fila.h"
namespace nrrecusa {
inline std::atomic<bool> segura{false};
inline constexpr unsigned seguidos_max=3;
inline std::atomic<unsigned long long> segurados{0};

inline int segurar(int job,int ultimo,int anterior,unsigned seguidos,bool chave,bool manutencao) noexcept {
    if(!chave||manutencao||job<=0||ultimo<=0||seguidos>=seguidos_max)return 0;
    if(job==ultimo)return 0;
    if(job==anterior)return job;
    if(job>ultimo&&job-ultimo<=2)return ultimo;
    return 0;
}

inline bool campos_ok(int job,int pronto,int compondo,int nativo,bool chave) noexcept {
    if(pronto==job&&compondo==job)return true;
    if(!chave||job<=0)return false;
    return (compondo==job||(nativo>0&&compondo==nativo))&&pronto>=job&&pronto-job<=2;
}
inline const char* motivo_job(int job,int ultimo,int anterior) noexcept {
    return job<=0?"sem_job":job==anterior?"job_anterior_corrida":job>ultimo?"job_ainda_nao_notado":"job_outro";
}
struct Conta {const char* motivo{};unsigned long long n{};};
inline std::mutex& trava(){static std::mutex m;return m;}
inline Conta* contas(){static Conta c[32];return c;}

inline void registrar(const char* motivo,int job,int a,int b,int c,int segurado=0) noexcept {
    try{
        unsigned long long n=0;
        {std::lock_guard<std::mutex> l(trava());auto* t=contas();unsigned i=0;
         for(;i<32;++i){if(!t[i].motivo){t[i].motivo=motivo;break;}if(t[i].motivo==motivo||std::strcmp(t[i].motivo,motivo)==0)break;}
         if(i<32)n=++t[i].n;else n=1;}
        if(n>20&&n%256!=0)return;
        nrlogfila::linha(L"nr_reproject.log","pid=%d RECUSA motivo=%s job=%d a=%d b=%d c=%d segurado=%d n=%llu segurados=%llu\n",_getpid(),motivo,job,a,b,c,segurado,n,segurados.load());
    }catch(...){}
}
inline unsigned long long contagem(const char* motivo) noexcept {
    std::lock_guard<std::mutex> l(trava());auto* t=contas();
    for(unsigned i=0;i<32&&t[i].motivo;++i)if(std::strcmp(t[i].motivo,motivo)==0)return t[i].n;
    return 0;
}
}
