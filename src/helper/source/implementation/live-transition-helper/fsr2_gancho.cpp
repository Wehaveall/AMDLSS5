#include "fsr2_gancho.h"
#include "fsr2_ffx.h"
#include "../active-scheduling/fsr2_ffx_recurso_dx12.h"
#include "../active-scheduling/fsr2_descriptor.h"
#include "../compat-diag/compat.h"
#include "../../../runtime/ini_jogo.h"
#include "../../../runtime/protecao_pagina.h"
#include "../../../runtime/proxy_layout.h"
#include <windows.h>
#include <delayimp.h>
#include <psapi.h>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
namespace nrfsr2 {
namespace {
using Create=std::int32_t(*)(void* context,const void* description);
using Dispatch=std::int32_t(*)(void* context,const void* description);
using Destroy=std::int32_t(*)(void* context);
constexpr std::int32_t kErroGenerico=static_cast<std::int32_t>(0x8000000a);
std::atomic<void*> base_{};
std::atomic<Create> real_create{};
std::atomic<Dispatch> real_dispatch{};
std::atomic<Destroy> real_destroy{};
std::atomic<std::uint64_t> despachos_{},enganchados_{};

struct Contexto{void* ctx;std::uint32_t flags;};
std::mutex contextos_mutex;std::array<Contexto,8> contextos{};unsigned proximo{};
bool flags_de(void* ctx,std::uint32_t& flags) noexcept{
  std::lock_guard<std::mutex> l(contextos_mutex);for(const auto& c:contextos)if(c.ctx&&c.ctx==ctx){flags=c.flags;return true;}return false;
}
bool legivel(const void* p,std::size_t n) noexcept{
  MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(p,&m,sizeof m)||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
  const auto fim=reinterpret_cast<std::uintptr_t>(m.BaseAddress)+m.RegionSize;
  return reinterpret_cast<std::uintptr_t>(p)+n<=fim||legivel(reinterpret_cast<const std::byte*>(fim),reinterpret_cast<std::uintptr_t>(p)+n-fim);
}

struct Chamada{Dispatch original;const void* fsr2;void* ctx;const nrffx::fsr2::Leiaute* L;};
thread_local Chamada* chamada{};thread_local int dentro{};
std::int32_t trampolim(void* ctx,void* generico){
  auto* c=chamada;if(!c||c->ctx!=ctx||!c->original)return kErroGenerico;

  std::array<std::byte,0x618> copia{};const auto& L=*c->L;const std::size_t pre=L.dispatch_prefix;
  nrffx::generic_to_fsr2(static_cast<const std::byte*>(generico),c->fsr2,copia.data(),pre,L);
  if(std::memcmp(copia.data(),c->fsr2,pre)==0)return c->original(ctx,c->fsr2);
  const std::size_t n=(&L==&nrffx::fsr2::v21&&legivel(c->fsr2,copia.size()))?copia.size():pre;
  nrffx::generic_to_fsr2(static_cast<const std::byte*>(generico),c->fsr2,copia.data(),n,L);
  return c->original(ctx,copia.data());
}
void registrar(void* ctx,std::uint32_t flags,const char* origem) noexcept{
  {std::lock_guard<std::mutex> l(contextos_mutex);bool achou=false;
   for(auto& c:contextos)if(c.ctx==ctx){c.flags=flags;achou=true;}
   if(!achou)contextos[proximo++%contextos.size()]={ctx,flags};}

  if(auto* b=static_cast<std::uint8_t*>(base_.load(std::memory_order_acquire));b&&nrproxy::admitido(b)){
    *reinterpret_cast<volatile std::int32_t*>(b+0x94A84)=1;
    *reinterpret_cast<volatile std::int32_t*>(b+nrproxy::rva_flags_ffx)=(flags&nrffx::fsr2::flag_depth_inverted)?1:0;
  }
  char t[192];std::snprintf(t,sizeof t,"%s flags=0x%x (profundidade invertida=%u, MV na tela=%u)",origem,flags,
    (flags&nrffx::fsr2::flag_depth_inverted)?1u:0u,(flags&nrffx::fsr2::flag_mv_display)?1u:0u);
  nrcompat::tolerado("tolerado_fsr2_contexto",t);
}
std::int32_t gancho_create(void* ctx,const void* desc){
  const auto real=real_create.load(std::memory_order_acquire);if(!real)return kErroGenerico;
  const auto r=real(ctx,desc);
  if(r==0&&ctx&&desc){std::uint32_t flags{};std::memcpy(&flags,desc,sizeof flags);registrar(ctx,flags,"ffxFsr2ContextCreate");
    nrfsr2ffx::ao_criar(ctx,desc);}
  return r;
}

std::int32_t gancho_destroy(void* ctx){
  const auto real=real_destroy.load(std::memory_order_acquire);if(!real)return kErroGenerico;
  nrfsr2ffx::ao_destruir(ctx);
  {std::lock_guard<std::mutex> l(contextos_mutex);for(auto& c:contextos)if(c.ctx==ctx)c={};}
  return real(ctx);
}

bool flags_do_contexto(void* ctx,std::uint32_t& flags) noexcept{
  if(flags_de(ctx,flags))return true;
  std::uint32_t lido{};if(!legivel(ctx,sizeof lido))return false;std::memcpy(&lido,ctx,sizeof lido);
  if(lido&~0x1FFu)return false;
  registrar(ctx,lido,"contexto ja criado (lido do FfxFsr2Context)");flags=lido;return true;
}
std::int32_t gancho_dispatch(void* ctx,const void* desc){
  const auto real=real_dispatch.load(std::memory_order_acquire);if(!real)return kErroGenerico;
  auto* base=base_.load(std::memory_order_acquire);

  if(dentro||!desc||!legivel(desc,nrffx::fsr2::v20.dispatch_prefix))return real(ctx,desc);

  const auto* L=nrffx::leiaute_plausivel(desc,legivel(desc,nrffx::fsr2::dispatch_prefix));
  if(!L){static std::atomic<bool> avisou{false};if(!avisou.exchange(true))nrcompat::tolerado("tolerado_fsr2_leiaute_desconhecido","despacho do FSR 2 sem layout plausivel (2.0 ou 2.1-2.2): so o FSR");
    return real(ctx,desc);}
  if(L==&nrffx::fsr2::v20){static std::atomic<bool> avisou{false};if(!avisou.exchange(true))nrcompat::tolerado("tolerado_fsr2_leiaute20","FSR 2.0 (FfxResource sem name[64])");}

  if(nrfsr2ffx::pedido()){
    const std::size_t n=(L==&nrffx::fsr2::v21&&legivel(desc,0x618))?0x618:L->dispatch_prefix;
    std::int32_t r{};
    if(nrfsr2ffx::despachar(ctx,desc,*L,n,r)){despachos_.fetch_add(1,std::memory_order_relaxed);return r;}
  }
  if(!base)return real(ctx,desc);

  std::uint32_t flags{};(void)flags_do_contexto(ctx,flags);
  std::array<std::byte,nrffx::descriptor_size> generico{};
  if(!nrffx::fsr2_to_generic(desc,generico.data(),generico.size(),*L))return real(ctx,desc);
  despachos_.fetch_add(1,std::memory_order_relaxed);
  Chamada c{real,desc,ctx,L};
  struct Escopo{Chamada* antes;Escopo(Chamada* x):antes(chamada){chamada=x;++dentro;}~Escopo(){chamada=antes;--dentro;}} escopo(&c);
  std::uint32_t handled=1;
  return nr_despacho_generico(base,&trampolim,ctx,generico.data(),0,&handled);
}
bool nome_igual(const char* a,const char* b) noexcept{return a&&b&&_stricmp(a,b)==0;}

unsigned trocar(std::uintptr_t* slot,const void* real_c,const void* real_d,const void* real_x) noexcept{
  unsigned n=0;
  for(;*slot;++slot){
    void* alvo=nullptr;
    if(reinterpret_cast<void*>(*slot)==real_d)alvo=reinterpret_cast<void*>(&gancho_dispatch);
    else if(reinterpret_cast<void*>(*slot)==real_c)alvo=reinterpret_cast<void*>(&gancho_create);
    else if(real_x&&reinterpret_cast<void*>(*slot)==real_x)alvo=reinterpret_cast<void*>(&gancho_destroy);
    if(!alvo)continue;

    DWORD antes{};if(!VirtualProtect(slot,sizeof *slot,nrpagina::gravavel_faixa(slot,sizeof *slot),&antes))continue;
    InterlockedExchangePointer(reinterpret_cast<void**>(slot),alvo);DWORD x{};VirtualProtect(slot,sizeof *slot,antes,&x);++n;
  }
  return n;
}
unsigned enganchar_modulo(HMODULE m,const void* real_c,const void* real_d,const void* real_x) noexcept{
  __try{
    auto* b=reinterpret_cast<std::uint8_t*>(m);auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(b);if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return 0;
    auto* nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(b+dos->e_lfanew);if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return 0;
    unsigned n=0;
    const auto& imp=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if(imp.VirtualAddress&&imp.Size)
      for(auto* d=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(b+imp.VirtualAddress);d->Name;++d)
        if(nome_igual(reinterpret_cast<const char*>(b+d->Name),"ffx_fsr2_api_x64.dll")&&d->FirstThunk)
          n+=trocar(reinterpret_cast<std::uintptr_t*>(b+d->FirstThunk),real_c,real_d,real_x);
    const auto& dly=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
    if(dly.VirtualAddress&&dly.Size)
      for(auto* d=reinterpret_cast<ImgDelayDescr*>(b+dly.VirtualAddress);d->rvaDLLName;++d)
        if((d->grAttrs&dlattrRva)&&nome_igual(reinterpret_cast<const char*>(b+d->rvaDLLName),"ffx_fsr2_api_x64.dll")&&d->rvaIAT)
          n+=trocar(reinterpret_cast<std::uintptr_t*>(b+d->rvaIAT),real_c,real_d,real_x);
    return n;
  }__except(EXCEPTION_EXECUTE_HANDLER){return 0;}
}
bool ligado() noexcept{static const bool v=[]{double x=1.;return !nrinijogo::ler_double(L"NeuralFsr2",x)||x>.5;}();return v;}

std::atomic<bool> nossa_dll{false};
unsigned instalar(HMODULE fsr2,void* base) noexcept{

  if(GetProcAddress(fsr2,nrffx::fsr2ffx::marca_export)){
    if(!nossa_dll.exchange(true))nrcompat::tolerado("tolerado_fsr2_nossa_dll","ffx_fsr2_api_x64.dll e a nossa (FSR 2 -> FFX na propria DLL): o gancho do helper fica fora");
    return 0;
  }
  base_.store(base,std::memory_order_release);
  auto* c=reinterpret_cast<Create>(GetProcAddress(fsr2,"ffxFsr2ContextCreate"));auto* d=reinterpret_cast<Dispatch>(GetProcAddress(fsr2,"ffxFsr2ContextDispatch"));
  if(!c||!d)return 0;
  real_create.store(c,std::memory_order_release);real_dispatch.store(d,std::memory_order_release);
  auto* x=reinterpret_cast<Destroy>(GetProcAddress(fsr2,"ffxFsr2ContextDestroy"));real_destroy.store(x,std::memory_order_release);
  HMODULE mods[1024];DWORD bytes{};if(!K32EnumProcessModules(GetCurrentProcess(),mods,sizeof mods,&bytes))return 0;
  HMODULE self{};GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&gancho_dispatch),&self);
  unsigned trocados=0;
  for(DWORD i=0;i<bytes/sizeof(HMODULE)&&i<1024;++i)if(mods[i]!=fsr2&&mods[i]!=self)trocados+=enganchar_modulo(mods[i],reinterpret_cast<const void*>(c),reinterpret_cast<const void*>(d),reinterpret_cast<const void*>(x));
  if(trocados){
    const auto total=enganchados_.fetch_add(trocados,std::memory_order_relaxed)+trocados;
    char t[128];std::snprintf(t,sizeof t,"ffx_fsr2_api_x64.dll: %u importacoes trocadas (total %llu)",trocados,static_cast<unsigned long long>(total));
    nrcompat::tolerado("tolerado_fsr2_gancho",t);
  }
  return trocados;
}
}
void tentar(void* base) noexcept{

  if(despachos_.load(std::memory_order_relaxed)||nossa_dll.load(std::memory_order_relaxed))return;
  static std::atomic<unsigned> presents{0};
  const auto n=presents.fetch_add(1,std::memory_order_relaxed);
  if(n%(n<6000u?60u:600u)!=0u)return;
  const HMODULE fsr2=GetModuleHandleW(L"ffx_fsr2_api_x64.dll");if(!fsr2||!ligado())return;

  if(!nrproxy::admitido(base))return;
  (void)instalar(fsr2,base);
}
std::uint64_t despachos() noexcept{return despachos_.load(std::memory_order_relaxed);}
unsigned teste::enganchar(void* base) noexcept{
  const HMODULE fsr2=GetModuleHandleW(L"ffx_fsr2_api_x64.dll");return fsr2?instalar(fsr2,base):0u;
}
}
