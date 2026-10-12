#pragma once
#include "../../../runtime/nr_publico.h"

#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
namespace nrfgconfig {
struct Header{std::uint64_t type;Header* next;};
struct Res{void* resource;std::uint32_t tipo,formato,largura,altura,profundidade,mips,flags,uso;std::uint32_t estado;std::uint32_t pad;};
static_assert(sizeof(Res)==48,"FfxApiResource");
struct Configure20002{Header header;void* swapChain;void* presentCallback;void* presentCallbackCtx;void* fgCallback;void* fgCallbackCtx;bool enabled;bool async;std::uint8_t pad[6];Res hudless;std::uint32_t flags;bool onlyGenerated;std::uint8_t pad2[3];std::int32_t rect[4];std::uint64_t frameID;};
static_assert(offsetof(Configure20002,hudless)==64,"hudless");
inline constexpr unsigned char prologo[25]={0x4c,0x8b,0xc2,0x48,0x8b,0xd1,0x4d,0x85,0xc0,0x74,0x12,0x48,0x85,0xc9,0x74,0x0d,0x48,0x8b,0x01,0x48,0x8b,0x08,0x48,0x8b,0x01};
struct Estado{std::atomic<int> estado{0};bool log=false,sem_hudless=false;std::mutex m;char ultimo[512]{};unsigned linhas=0;unsigned long long chamadas=0,zerados=0;};
inline Estado& st(){static Estado e;return e;}
inline void escrever(const char* s){
(void)s;
}
inline std::int32_t substituta(void** ctx,Header* desc){
 if(!desc||!ctx)return 6;
 auto& e=st();
 try{
  for(Header* h=desc;h;h=h->next){
   if(h->type==0x20002u){
    auto* c=reinterpret_cast<Configure20002*>(h);
    if(e.log){
     char s[512];NRLOGF(s,sizeof s,"FG_CONFIGURE 0x20002 ctx=%p swapchain=%p enabled=%d async=%d hudless=%p fmt=%u %ux%u estado=0x%x flags=0x%x present_cb=%p fg_cb=%p so_gerado=%d rect=%d,%d,%d,%d",
      static_cast<void*>(ctx),c->swapChain,int(c->enabled),int(c->async),c->hudless.resource,c->hudless.formato,c->hudless.largura,c->hudless.altura,c->hudless.estado,c->flags,c->presentCallback,c->fgCallback,int(c->onlyGenerated),c->rect[0],c->rect[1],c->rect[2],c->rect[3]);
     std::lock_guard<std::mutex> l(e.m);

     if((std::strcmp(s,e.ultimo)!=0&&e.linhas<200)||(++e.chamadas%6000==1)){strcpy_s(e.ultimo,s);++e.linhas;escrever(s);}
    }

    if(e.sem_hudless&&!e.zerados++)escrever("FG_SEM_HUDLESS ignorado (desativado: TDR no jogo)");
   }else if(e.log&&h->type==0x30002u){

    struct Ui{Header header;Res ui;std::uint32_t flags;};auto* u=reinterpret_cast<Ui*>(h);
    char s[300];NRLOGF(s,sizeof s,"FG_CONFIGURE 0x30002 (UI do swapchain FI) ctx=%p ui=%p fmt=%u %ux%u estado=0x%x flags=0x%x",
     static_cast<void*>(ctx),u->ui.resource,u->ui.formato,u->ui.largura,u->ui.altura,u->ui.estado,u->flags);
    std::lock_guard<std::mutex> l(e.m);if(e.linhas<200&&std::strcmp(s,e.ultimo)!=0){strcpy_s(e.ultimo,s);++e.linhas;escrever(s);}
   }else if(e.log&&(h->type&0xFFFF0000u)==0x30000u){
    char s[200];NRLOGF(s,sizeof s,"FG_CONFIGURE swapchain tipo=0x%llx ctx=%p",static_cast<unsigned long long>(h->type),static_cast<void*>(ctx));
    std::lock_guard<std::mutex> l(e.m);if(e.linhas<200&&std::strcmp(s,e.ultimo)!=0){strcpy_s(e.ultimo,s);++e.linhas;escrever(s);}
   }
  }
 }catch(...){}
 void* obj=**reinterpret_cast<void***>(ctx);
 auto** vt=*reinterpret_cast<void***>(obj);
 using F=std::int32_t(*)(void*,void**,Header*);
 return reinterpret_cast<F>(vt[5])(obj,ctx,desc);
}
inline void instalar(const std::wstring& ini) noexcept {
 auto& e=st();int zero=0;if(!e.estado.compare_exchange_strong(zero,1))return;
 try{
  e.log=NRCFG_INT(L"DlssNrOnAmd",L"NeuralFgConfigLog",0,ini.c_str())==1;
  e.sem_hudless=NRCFG_INT(L"DlssNrOnAmd",L"NeuralFgSemHudless",0,ini.c_str())==1;
  if(!e.log&&!e.sem_hudless){e.estado=3;return;}
  HMODULE fg=GetModuleHandleW(L"amd_fidelityfx_framegeneration_dx12.dll");
  if(!fg){
   wchar_t p[32768]{};std::wstring pastas[3];
   if(HMODULE ld=GetModuleHandleW(L"amd_fidelityfx_loader_dx12.dll");ld&&GetModuleFileNameW(ld,p,32768)){std::wstring s(p);pastas[0]=s.substr(0,s.find_last_of(L"\\/"));}
   if(GetModuleFileNameW(nullptr,p,32768)){std::wstring s(p);pastas[1]=s.substr(0,s.find_last_of(L"\\/"));}
   if(!ini.empty())pastas[2]=ini.substr(0,ini.find_last_of(L"\\/"));
   for(auto& d:pastas){if(d.empty())continue;const std::wstring f=d+L"\\amd_fidelityfx_framegeneration_dx12.dll";
    if(GetFileAttributesW(f.c_str())!=INVALID_FILE_ATTRIBUTES){fg=LoadLibraryW(f.c_str());if(fg)break;}}
  }
  if(!fg){escrever("FG_CONFIG provedor de FG ausente");e.estado=3;return;}
  auto* alvo=reinterpret_cast<unsigned char*>(GetProcAddress(fg,"ffxConfigure"));
  if(!alvo||std::memcmp(alvo,prologo,sizeof prologo)!=0){escrever("FG_CONFIG ffxConfigure do provedor desconhecido: nada feito");e.estado=3;return;}
  unsigned char p[14]={0xFF,0x25,0,0,0,0};const std::uint64_t nosso=reinterpret_cast<std::uint64_t>(&substituta);std::memcpy(p+6,&nosso,8);
  DWORD velho=0;if(!VirtualProtect(alvo,sizeof p,PAGE_EXECUTE_READWRITE,&velho)){escrever("FG_CONFIG VirtualProtect falhou");e.estado=3;return;}
  std::memcpy(alvo,p,sizeof p);DWORD x=0;VirtualProtect(alvo,sizeof p,velho,&x);FlushInstructionCache(GetCurrentProcess(),alvo,sizeof p);
  char s[160];NRLOGF(s,sizeof s,"FG_CONFIG instalado log=%d sem_hudless=%d",int(e.log),int(e.sem_hudless));escrever(s);
  e.estado=2;
 }catch(...){e.estado=3;}
}
}
