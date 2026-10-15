#pragma once
#include "../../../runtime/nr_publico.h"

#include <windows.h>
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
FILE* f{};if(fopen_s(&f,"nr_fg_pacing.log","a")==0&&f){std::fprintf(f,"pid=%lu tick=%llu %s\n",GetCurrentProcessId(),GetTickCount64(),s);std::fclose(f);}
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
 try{

  if(NRCFG_INT(L"DlssNrOnAmd",L"NeuralFgPacing",0,ini.c_str())!=1){e.estado=3;return;}
  wchar_t t[64]{};float m=0.f,v=0.f;
  NRCFG_STR(L"DlssNrOnAmd",L"NeuralFgPacingMargem",L"0",t,64,ini.c_str());m=float(_wtof(t));
  NRCFG_STR(L"DlssNrOnAmd",L"NeuralFgPacingVariancia",L"0",t,64,ini.c_str());v=float(_wtof(t));
  e.t.margem=m<0.f?0.f:(m>5.f?5.f:m);e.t.variancia=v<0.f?0.f:(v>1.f?1.f:v);

  HMODULE fg=GetModuleHandleW(L"amd_fidelityfx_framegeneration_dx12.dll");
  if(!fg){

   std::filesystem::path pastas[3];wchar_t p[32768]{};
   if(HMODULE ld=GetModuleHandleW(L"amd_fidelityfx_loader_dx12.dll");ld&&GetModuleFileNameW(ld,p,32768))pastas[0]=std::filesystem::path(p).parent_path();
   if(GetModuleFileNameW(nullptr,p,32768))pastas[1]=std::filesystem::path(p).parent_path();
   if(!ini.empty())pastas[2]=std::filesystem::path(ini).parent_path();
   for(auto& d:pastas){if(d.empty())continue;auto f=d/L"amd_fidelityfx_framegeneration_dx12.dll";std::error_code ec;
    if(std::filesystem::is_regular_file(f,ec)){fg=LoadLibraryW(f.c_str());if(fg)break;}}
  }
  if(!fg){log("FG_PACING provedor de FG ausente");e.estado=3;return;}
  auto* alvo=reinterpret_cast<unsigned char*>(GetProcAddress(fg,"ffxCreateContext"));
  e.configurar=reinterpret_cast<Configure>(GetProcAddress(fg,"ffxConfigure"));
  if(!alvo||!e.configurar||std::memcmp(alvo,prologo,sizeof prologo)!=0){log("FG_PACING ffxCreateContext do provedor desconhecido (versao diferente): nada feito");e.estado=3;return;}

  auto* tr=static_cast<unsigned char*>(VirtualAlloc(nullptr,64,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
  if(!tr){log("FG_PACING VirtualAlloc falhou");e.estado=3;return;}
  std::memcpy(tr,alvo,19);unsigned char j[14]={0xFF,0x25,0,0,0,0};const std::uint64_t volta=reinterpret_cast<std::uint64_t>(alvo+19);std::memcpy(j+6,&volta,8);std::memcpy(tr+19,j,14);
  FlushInstructionCache(GetCurrentProcess(),tr,64);
  e.original=reinterpret_cast<Create>(tr);
  unsigned char p[19];std::memset(p,0x90,sizeof p);p[0]=0xFF;p[1]=0x25;std::memset(p+2,0,4);const std::uint64_t nosso=reinterpret_cast<std::uint64_t>(&gancho);std::memcpy(p+6,&nosso,8);
  DWORD velho=0;if(!VirtualProtect(alvo,sizeof p,PAGE_EXECUTE_READWRITE,&velho)){log("FG_PACING VirtualProtect falhou");e.estado=3;return;}
  std::memcpy(alvo,p,sizeof p);DWORD x=0;VirtualProtect(alvo,sizeof p,velho,&x);FlushInstructionCache(GetCurrentProcess(),alvo,sizeof p);
  char s[160];NRLOGF(s,sizeof s,"FG_PACING instalado margem=%.3f variancia=%.3f (proximo swapchain FI criado recebe o ajuste)",e.t.margem,e.t.variancia);log(s);
  e.estado=2;
 }catch(...){e.estado=3;}
}
}
