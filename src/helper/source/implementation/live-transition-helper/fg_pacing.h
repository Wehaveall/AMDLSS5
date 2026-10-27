#pragma once
#include "../../../runtime/nr_publico.h"

#include <windows.h>
#include "../../../runtime/log_fila.h"
#include "fg_comum.h"
#include "../../../runtime/pasta_helper.h"
#include "../../../runtime/ini_jogo.h"
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
namespace nrfgpacing {
struct Header{std::uint64_t type;void* next;};
struct Tuning{float margem;float variancia;bool spin;std::uint32_t spin_t;bool wfso;};
struct KV{Header header;std::uint64_t key;std::uint64_t u64;void* ptr;};
using Create=std::int32_t(*)(void** context,Header* desc,const void* memCb);
using Configure=std::int32_t(*)(void** context,const Header* desc);
inline constexpr unsigned char prologo[19]={0x40,0x55,0x53,0x56,0x57,0x41,0x57,0x48,0x8d,0x6c,0x24,0xc9,0x48,0x81,0xec,0xc0,0x00,0x00,0x00};
struct Estado{std::atomic<int> estado{0};Tuning t{0.f,0.f,false,2u,false};Create original=nullptr;Configure configurar=nullptr;std::atomic<unsigned> criados{0};};
inline Estado& st(){static Estado e;return e;}
inline void log(const char* s){
(void)s;
}
inline std::int32_t gancho(void** context,Header* desc,const void* mem){
 auto& e=st();
 const std::int32_t r=e.original(context,desc,mem);
 try{
  if(r==0&&desc&&desc->type==0x30006u&&context&&*context&&e.configurar){
   Tuning t=e.t;KV kv{};kv.header={0x30008u,nullptr};kv.key=2;kv.ptr=&t;
   const std::int32_t c=e.configurar(context,&kv.header);
   char s[200];NRLOGF(s,sizeof s,"FG_PACING swapchain FI criado (%u) contexto=%p margem=%.3f variancia=%.3f ffxConfigure=%d",
    e.criados.fetch_add(1)+1,*context,t.margem,t.variancia,int(c));log(s);
  }
 }catch(...){}
 return r;
}

inline void instalar(const std::wstring& ini) noexcept {
 auto& e=st();int zero=0;if(!e.estado.compare_exchange_strong(zero,1))return;
 (void)ini;e.estado=3;
}
}
