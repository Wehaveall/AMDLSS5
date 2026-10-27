#pragma once
#include <chrono>
#include "nr_publico.h"
#include "reg_swin32_policy.h"
#include "native_ini_path.h"
#include "ini_jogo.h"
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4201)
#endif
#include <hip/hip_runtime_api.h>
#include "hip_api.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "neural_tone.h"
#include "pasta_helper.h"
#include "../source/implementation/live-transition-image/live_image.h"
#include "../../v033-port/kernels/reg_swin32_contract.h"
#include "../../v033-port/kernels/reg_layout_contract.h"
#include "reg_swin_mh_policy.h"
#include "kernel_capture.h"
#include "kernel_trace.h"
#include "kernel_timeline.h"
#include "reg1d_policy.h"
#include "regvit_policy.h"
#include "sw32x_state.h"
#include "v05_state.h"
#include "vit1d_full.h"
#include "rede1080_policy.h"
#include "reuso_state.h"
#include "reuso_proj_contract.h"
#include "reuso1d_mov.h"
#include "exposicao_fresca.h"
#include "cena_mascara.h"
#include "hist_mov.h"
#include "reproj_params.h"
#include "orcamento_vram.h"
#include <string>
#include "../../v040-port/kernels/reg_swin_mh_contract.h"
#include "../../v043-port/1d/reg1d43_contract.h"
#include <windows.h>
#include <intrin.h>
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>
#include <vector>
#include <atomic>
#include <tuple>

namespace nrregswin {
constexpr bool swin32_contract_aligned() {
 for(unsigned i=0;i<8;++i)if(nrmhcontract::swin32[i]&&std::string_view(nrmhcontract::swin32[i])!=std::string_view(nrregcontract::symbols[i]))return false;
 return true;
}
static_assert(swin32_contract_aligned(),"nrmhcontract::swin32 fora da ordem de nrregcontract::symbols");
using nrregpolicy::Dim;
using nrregpolicy::Launch;
inline nrregpolicy::Dispatcher dispatch;
inline Launch native_launch{};
inline std::atomic<char*> installed_image{};

namespace rjsync {
inline std::atomic<bool> ligado{false};
inline std::recursive_mutex& trava(){static std::recursive_mutex* m=new std::recursive_mutex;return *m;}
inline std::atomic<unsigned long long> flag_set_n{0};

inline thread_local int nivel=0;
struct Dono {
 bool on=false;
 void travar(){trava().lock();++nivel;on=true;}
 Dono()=default;Dono(const Dono&)=delete;Dono& operator=(const Dono&)=delete;
 ~Dono(){if(on&&nivel>0){--nivel;trava().unlock();}}
};
inline void soltar_tudo() noexcept {while(nivel>0){--nivel;trava().unlock();}}

inline std::atomic<unsigned long long> rosto_us{0},rosto_n{0};inline std::atomic<int> rosto_dentro{0};
struct Rosto {
 std::lock_guard<std::recursive_mutex> l;std::chrono::steady_clock::time_point t;
 Rosto():l(trava()),t(std::chrono::steady_clock::now()){rosto_dentro.fetch_add(1,std::memory_order_relaxed);}
 ~Rosto(){rosto_dentro.fetch_sub(1,std::memory_order_relaxed);rosto_n.fetch_add(1,std::memory_order_relaxed);
  rosto_us.fetch_add((unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-t).count(),std::memory_order_relaxed);}
};
}
#define NR_R6_DENTRO false
inline std::filesystem::path folder;
inline std::vector<unsigned char> module_image;
inline std::vector<unsigned char> layout_image;
inline std::once_flag initialization;
inline std::mutex install_mutex;
inline void* module{};
inline void* layout_module{};inline void* to_reg{};inline void* to_legacy{};

inline bool v041_requested=true;
inline unsigned v041_mask=0xffu;
inline std::vector<unsigned char> v041_image;
inline void* v041_module{};
inline int device=-1;
inline int(*get_device)(int*){};
inline bool requested=false;
inline bool layout_requested=true;
inline std::array<std::atomic<void*>,4> installed_hooks{};
inline std::array<std::atomic<bool>,16> dispatch_logged{};
inline std::array<std::atomic<bool>,16> swin32_refused_logged{};

inline bool potencia2(std::atomic<unsigned long long>& c) noexcept {const auto n=c.fetch_add(1,std::memory_order_relaxed)+1;return (n&(n-1))==0;}
inline std::array<std::atomic<unsigned long long>,16> swin32_refused_count{};

inline std::mutex sw32_mutex;

inline std::atomic<bool> sw32_pendente{false};
inline std::atomic<bool> sw32_chain_logged{false};

namespace mh {
inline bool requested=false;
inline std::vector<unsigned char> image;
inline std::vector<unsigned char> layout_image;
inline std::once_flag initialization;
inline std::atomic<bool> ready{false};
inline const void* tokens[3]{};
inline void* functions[3][8]{};
inline void* to_mh{};inline void* to_legacy{};
inline nrregpolicy::ModuleLaunch module_launch{};
inline bool wide=false;
inline int device=-1;
inline std::array<std::atomic<bool>,24> logged{};
inline std::array<std::atomic<bool>,3*16> refused_logged{};
inline std::array<std::atomic<unsigned long long>,3*16> refused_count{};

inline bool chain_enabled=false;
inline bool drop_restore=false;
inline std::mutex chain_mutex;
inline nrmhpolicy::Chain chain;

inline std::atomic<bool> chain_pendente{false};
inline std::atomic<bool> chain_logged{false};
inline void* module_handle{};

inline std::vector<unsigned char> r55_image[3];
inline void* r55_module[3]{};
inline bool r55_borda[3]{};
}

namespace c1d {
inline bool requested=false;
inline std::vector<unsigned char> image;
inline void* module_handle{};
inline const void* token{};
inline std::once_flag initialization;
inline std::atomic<bool> ready{false};
inline void* conv{};inline void* reduce{};inline void* workspace{};inline void* stream{};inline bool stream_known=false;
inline std::mutex mutex;inline std::atomic<bool> logged{false};inline std::atomic<bool> refused_logged{false};
inline std::atomic<unsigned long long> refused_count{0};
}

namespace c1d43 {
inline bool requested=false;inline bool fast=true;
inline bool conv_split=true;inline unsigned qkv_split_max=0;inline unsigned split_teto=64;
inline std::vector<unsigned char> image;
inline void* module_handle{};
inline const void* tokens[5]{};
inline const void* foreign[5]{};
inline std::once_flag initialization;
inline std::atomic<bool> ready{false};
inline void* functions[nr1d43policy::fn_count]{};
inline nr1d43policy::Buffers buffers;
inline int(*memset_async)(void*,int,std::size_t,void*){};
inline bool counters_zeroed=false;
inline void* stream{};inline bool stream_known=false;
inline nr1d43policy::Chain chain;
inline std::mutex mutex;
inline std::array<std::atomic<bool>,nr1d43policy::fn_count> logged{};
inline std::atomic<bool> refused_logged{false};
inline std::atomic<unsigned long long> refused_count{0};
inline int kind(const void* f) noexcept {for(int i=0;i<5;++i)if(f==tokens[i])return i;for(int i=0;i<5;++i)if(f==foreign[i])return 5+i;return -1;}
}

namespace vit {
inline bool requested=false;
inline const void* tokens[4]{};
inline std::once_flag initialization;
inline std::atomic<bool> ready{false};
inline void* functions[nrvitpolicy::fn_count]{};
inline bool attn2=false;
inline bool manter_a=false;
inline std::atomic<bool> manter_logged{false};
inline void* conv4_v041{};
inline std::atomic<bool> exato_logged{false};
inline void* to_frag{};inline void* to_tin{};
inline std::vector<unsigned char> layout_image;
inline std::mutex mutex;
inline nrvitpolicy::Tracker tracker;
inline std::atomic<bool> any_tracked{false};
inline std::array<std::atomic<bool>,nrvitpolicy::fn_count> logged{};
inline std::array<std::atomic<bool>,4> refused_logged{};
inline std::array<std::atomic<unsigned long long>,4> refused_count{};
inline std::atomic<bool> flush_logged{false};
inline int kind(const void* f) noexcept {for(int i=0;i<4;++i)if(f==tokens[i])return i;return -1;}
}

namespace rw1d {
inline bool requested=false;inline bool split=true;
inline std::vector<unsigned char> image;
inline void* module{};inline void* functions[nr1080::contract::rw_count]{};
inline bool ready=false;
inline std::array<std::atomic<bool>,nr1080::contract::rw_count> logged{};
}

namespace r55d1 {
inline bool requested=false;
inline std::vector<unsigned char> image;
inline void* module{};inline void* qkv[nr1080::contract::r55_qkv_count]{};
inline bool ready=false;
inline std::array<std::atomic<bool>,nr1080::contract::r55_qkv_count> logged{};
}

namespace vitx {
inline bool requested=false;inline unsigned mask=7u;
inline std::vector<unsigned char> image,attn_image;
inline void* conv{};inline void* ffwd{};inline void* attn{};
inline std::array<std::atomic<bool>,3> logged{};
}

namespace decup {
inline bool requested=false;inline const void* token{};inline void* function{};
inline std::atomic<bool> logged{false};inline std::atomic<bool> refused_logged{false};
}
namespace cauda {
inline bool requested=false;inline const void* token{};
inline std::vector<unsigned char> image;
inline std::once_flag initialization;inline std::atomic<bool> ready{false};
inline void* module{};inline void* head{};inline int device=-1;
inline nrregpolicy::ModuleLaunch module_launch{};
inline std::atomic<bool> logged{false};inline std::atomic<bool> refused_logged{false};
}

namespace sw32x {
inline const nr1080::Sw32x* variant{};
inline std::vector<unsigned char> image;
inline void* module{};

inline std::vector<unsigned char> images[3];
inline void* modules[3]{};
inline void* sets[3][8]{};
inline bool have[3]{};
inline int applied=-1;
}

namespace v05 {
inline bool installed=false;
inline constexpr unsigned max_n=2304;
inline std::mutex mutex;
inline nrvit1dfull::Chain chains[4]{};
inline nrvit1dfull::Shadows shadows;
inline int(*malloc_fn)(void**,std::size_t){};
inline int(*memset_async)(void*,int,std::size_t,void*){};
inline int(*range)(void**,std::size_t*,void*){};
inline int(*free_fn)(void*){};
inline unsigned long long jobs=0;
inline unsigned seen[8]{};inline unsigned seen_count=0;
}

namespace r1d {

inline bool so_cena=false;inline unsigned long long extras=0;
inline bool on=false;inline bool skip=false;inline bool valid=false;inline unsigned long long job=0,reusos=0,cheios=0;
inline unsigned n=0;inline void* stream{};inline void* cache{};inline std::size_t cap=0;inline unsigned refresco_visto=0;
inline int(*memcpy_async)(void*,const void*,std::size_t,int,void*){};

namespace mov {
inline bool on=false;inline nrr1dmov::Config cfg{};inline nrr1dmov::Estado est{};
inline std::vector<unsigned char> image;inline std::once_flag init;inline bool ready=false;
inline void* module{};inline void* k{};inline void* stats_dev{};inline unsigned* stats_host{};inline void* ev{};inline void* d{};inline std::size_t d_cap=0;
inline bool pend=false;inline bool nova=false;inline nrr1dmov::Medida ult{};
inline unsigned long long medidas=0,forcados=0,sem_mv=0,em_voo=0;
using ModuleLaunch=int(*)(void*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,void*,void**,void**);
inline ModuleLaunch mlaunch{};inline int(*memset_async)(void*,int,std::size_t,void*){};
inline int(*event_record)(void*,void*){};inline int(*event_query)(void*){};inline int(*malloc_)(void**,std::size_t){};

using Mv=nrreproj::Mv;

inline void colher() noexcept {
 if(!pend||!event_query||event_query(ev)!=0)return;
 pend=false;ult=nrr1dmov::medida_de(stats_host);nova=ult.ok;if(nova)++medidas;
}
}
}

namespace aposentados {
inline std::mutex m;
inline std::vector<std::pair<void*,int(*)(void*)>> lista;
inline void guardar(void* p,int(*soltar_fn)(void*)) noexcept {
 if(!p||!soltar_fn)return;
 try{std::lock_guard<std::mutex> t(m);lista.emplace_back(p,soltar_fn);}catch(...){}
}
inline void soltar() noexcept {
 std::vector<std::pair<void*,int(*)(void*)>> agora;
 {std::lock_guard<std::mutex> t(m);if(lista.empty())return;agora.swap(lista);}
 for(const auto& [p,f]:agora)(void)f(p);
}
}
inline void log(const char* text) noexcept {
 (void)text;
}

inline void register_hook(void* hook) noexcept {
 for(auto& slot:installed_hooks){void* empty=nullptr;if(slot.load()==hook||slot.compare_exchange_strong(empty,hook))return;}
}
inline bool known_launch(void* current,void* native) noexcept {
 if(current==native)return true;
 for(auto& slot:installed_hooks)if(slot.load()==current&&current)return true;
 return false;
}
inline bool read_exact(const std::filesystem::path& path,std::size_t size,std::vector<unsigned char>& bytes) {
 return nrmodpin::ler_bytes(path,size,bytes);
}

inline bool ler_modulo(const std::filesystem::path& path,std::size_t size,const unsigned char* sha,std::vector<unsigned char>& bytes,const unsigned char* alt=nullptr) {
 return nrneuraltone::read_module(path,size,sha,bytes,alt);
}
inline int fallback(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {

 nrhistmov::Salvo hs{};
 if((nrhistmov::modo||nrhistmov::log_on)&&args&&args[0]&&f==installed_image.load()+0x62238)
  hs=nrhistmov::antes(args[0],nrreusostate::cena,nrreusostate::quadro_cena,nrhistmov::job_atual,r1d::mov::ult.media,r1d::mov::ult.ok,&log);
 int result{};
 if(nrneuraltone::try_launch(f,g.x,g.y,g.z,b.x,b.y,b.z,args,shared,stream,result)){nrhistmov::depois(hs);return result;}
 result=native_launch(f,g,b,args,shared,stream);
 nrhistmov::depois(hs);
 return result;
}

template<class F> inline int timed_conversion(const char* name,void* stream,F&& run) {
 if(!nrtimeline::requested)return run();
 const auto m=nrtimeline::recorder.start(stream);
 const int result=run();
 nrtimeline::recorder.nested_stop(m,name,stream);
 return result;
}
inline int convert_layout(bool forward,void* data,std::uint64_t bytes,void* stream) {
 unsigned long long count=bytes;void* args[]{&data,&count};
 return timed_conversion(forward?"@conv_swin32_in":"@conv_swin32_out",stream,[&]{
  return dispatch.module_launch(forward?to_reg:to_legacy,static_cast<unsigned>(bytes/512),1,1,128,1,1,0,stream,args,nullptr);});
}

static_assert(!nrmhcontract::fast_available||(!nrmhcontract::fast_swin32[6]&&!nrmhcontract::fast_swin32[7]),
              "swin32 Fast modo 20 (+0x70) e 32 (+0xA8) leem campos que o proxy 28609E75 nao escreve");
static_assert(!nrmhcontract::fast_available||nrmhcontract::fast_varparams_bytes==176,"VarParams Fast != 176 B");

static_assert(!nrmhcontract::fast43_available||nrmhcontract::fast43_varparams_bytes==176,"VarParams Fast v0.4.3 != 176 B");
namespace fast {
inline bool requested=false;
inline unsigned version=42;
inline unsigned mask=7u;
inline unsigned sw32_mask=0xffu;
inline std::vector<unsigned char> image;
inline std::vector<unsigned char> image43;

inline std::filesystem::path pasta;
inline std::once_flag initialization;
inline void* module{};
struct Table {const char* const (*mh)[8];const char* const* swin32;const char* const* vit;bool attn2;const char* tag;};
inline Table table() noexcept {
 if(version==43)return {nrmhcontract::fast43_symbols,nrmhcontract::fast43_swin32,nrmhcontract::fast43_vit,true,"v0.4.3"};
 return {nrmhcontract::fast_symbols,nrmhcontract::fast_swin32,nrmhcontract::fast_vit,false,"v0.4.2"};
}
inline void load() noexcept {
 std::call_once(initialization,[]{
  try {
   if(!requested)return;
   auto hip=GetModuleHandleW(L"amdhip64_7.dll");if(!hip)return;
   const auto load_data=nrhip::api().module_load_data;
   if(!load_data)return;
   if(!image43.empty()){
    if(!load_data(&module,image43.data())&&module){
     version=43;
     return;
    }
    module=nullptr;log("FAST43 FALLBACK nr_reg_fast43.hsaco load rejected; tentando o v0.4.2");

    if(image.empty()&&!pasta.empty()&&(!ler_modulo(pasta/L"nr_reg_fast.hsaco",nrmhcontract::fast_module_bytes,nrmhcontract::fast_sha256,image))){image.clear();log("FAST FALLBACK nr_reg_fast.hsaco absent or digest mismatch; tudo no v0.4.1");}
   }
   version=42;
   if(image.empty())return;
   if(load_data(&module,image.data())||!module){module=nullptr;log("FAST FALLBACK nr_reg_fast.hsaco load rejected; tudo no v0.4.1");}
  }catch(...){module=nullptr;log("FAST FALLBACK load exception");}
 });
}

inline bool resolve(int(*function)(void**,void*,const char*),const char* const* names,void** out,unsigned n) noexcept {
 if(!module)return false;
 for(unsigned i=0;i<n;++i){out[i]=nullptr;if(names[i]&&(function(&out[i],module,names[i])||!out[i]))return false;}
 return true;
}
}
inline void initialize() noexcept {
 try {
  auto hip=GetModuleHandleW(L"amdhip64_7.dll");if(!hip){log("FALLBACK missing HIP runtime");return;}
  const auto properties=nrhip::api().get_device_properties;
  get_device=nrhip::api().get_device;
  const auto load=nrhip::api().module_load_data;
  const auto function=nrhip::api().module_get_function;
  const auto by_symbol=nrhip::api().get_func_by_symbol;
  const auto name=nrhip::api().kernel_name_ref;
  dispatch.module_launch=nrhip::api().module_launch_kernel;
  if(!properties||!get_device||!load||!function||!by_symbol||!name||!dispatch.module_launch){log("FALLBACK HIP API missing");return;}
  if(!nrhip::dispositivo_ok(get_device,properties,device)){log("FALLBACK device is not gfx1201 wave32");return;}
  const void* native_tokens[]{dispatch.token,dispatch.alternate_token};
  const char* native_names[]{"_Z10k_swin_varILi32ELb1EEv9VarParams","_Z10k_swin_varILi32ELb0EEv9VarParams"};
  for(unsigned i=0;i<2;++i){
   void* native_function{};
   if(by_symbol(&native_function,native_tokens[i])){log("FALLBACK native Swin32 token unresolved");return;}
   const auto native_name=name(native_function);
   if(!native_name||std::strcmp(native_name,native_names[i])){log("FALLBACK native Swin32 ABI identity mismatch");return;}
  }
  if(load(&module,module_image.data())){log("FALLBACK module load rejected");return;}
  for(unsigned i=0;i<8;++i)if(function(&dispatch.functions[i],module,nrregcontract::symbols[i])||!dispatch.functions[i]){log("FALLBACK reg symbol missing");return;}

  if(!v041_image.empty()){
   void* v041[8]{};bool ok=!load(&v041_module,v041_image.data())&&v041_module;
   for(unsigned i=0;ok&&i<8;++i)if(nrmhcontract::swin32[i])ok=!function(&v041[i],v041_module,nrmhcontract::swin32[i])&&v041[i];
   if(ok){
    char line[160]{};int n=NRLOGF(line,sizeof line,"SW32 V041 modos");
    for(unsigned i=0;i<8;++i)if(v041[i]&&(v041_mask>>i&1u)){dispatch.functions[i]=v041[i];n+=NRLOGF(line+n,sizeof line-n," %u",nrregpolicy::modes[i]);}
    log(line);
   }else log("SW32 V041 FALLBACK modulo ou simbolo recusado; todos os modos no v0.3.3");
  }
  if(fast::requested&&(fast::mask&2u)){
   fast::load();void* f[8]{};const auto t=fast::table();

   constexpr unsigned sw32_allowed=0xffu;constexpr unsigned sw32_shifted=0xc0u;
   bool applied43[8]{};
   if(fast::resolve(function,t.swin32,f,8)){
    char line[160]{};int n=NRLOGF(line,sizeof line,"SW32 FAST %s modos",t.tag);
    for(unsigned i=0;i<8;++i)if(f[i]&&(fast::sw32_mask&sw32_allowed)>>i&1u){dispatch.functions[i]=f[i];dispatch.wide[i]=true;dispatch.shifted[i]=fast::version==43&&(sw32_shifted>>i&1u);applied43[i]=fast::version==43;n+=NRLOGF(line+n,sizeof line-n," %u",nrregpolicy::modes[i]);}
    log(line);
   }else log("SW32 FAST FALLBACK modulo ou simbolo recusado; swin32 fica no v0.4.1/v0.3.3");

   if(fast::version==43){
    for(unsigned i=0;i<8;++i)sw32x::sets[0][i]=dispatch.functions[i];sw32x::have[0]=true;unsigned bits=1u;
    for(int v=1;v<=2;++v){if(sw32x::images[v].empty())continue;void* x[8]{};bool ok=!load(&sw32x::modules[v],sw32x::images[v].data())&&sw32x::modules[v];
     for(unsigned i=0;ok&&i<8;++i)if(nr1080::contract::sw32x_swin32[i])ok=!function(&x[i],sw32x::modules[v],nr1080::contract::sw32x_swin32[i])&&x[i];
     if(!ok){log("SW32X FALLBACK variante do menu recusada");continue;}
     for(unsigned i=0;i<8;++i)sw32x::sets[v][i]=nr1080::sw32x_mode(i,applied43[i])&&x[i]?x[i]:sw32x::sets[0][i];
     sw32x::have[v]=true;bits|=1u<<v;}
    nrsw32x::disponivel.store(bits,std::memory_order_release);
   }

   if(sw32x::variant&&!sw32x::image.empty()){
    void* x[8]{};bool ok=fast::version==43&&!load(&sw32x::module,sw32x::image.data())&&sw32x::module;
    for(unsigned i=0;ok&&i<8;++i)if(nr1080::contract::sw32x_swin32[i])ok=!function(&x[i],sw32x::module,nr1080::contract::sw32x_swin32[i])&&x[i];
    if(ok){
     char line[160]{};int n=NRLOGF(line,sizeof line,"SW32X %s modos",sw32x::variant->tag);
     for(unsigned i=0;i<8;++i)if(nr1080::sw32x_mode(i,applied43[i])&&x[i]){dispatch.functions[i]=x[i];n+=NRLOGF(line+n,sizeof line-n," %u",nrregpolicy::modes[i]);}
     log(line);sw32x::applied=nrsw32x::escolha.load(std::memory_order_relaxed);
    }else log(fast::version==43?"SW32X FALLBACK modulo ou simbolo recusado; swin32 fica no Fast v0.4.3":"SW32X FALLBACK swin32 fora do Fast v0.4.3; modulo novo nao usado");
   }
  }
  if(layout_requested){
   if(load(&layout_module,layout_image.data())||function(&to_reg,layout_module,nrreglayout::to_reg)||function(&to_legacy,layout_module,nrreglayout::to_legacy)||!to_reg||!to_legacy){log("FALLBACK layout module rejected");return;}
   dispatch.layout=&convert_layout;
  }

  dispatch.ready.store(true,std::memory_order_release);
  log(layout_requested?"READY gfx1201 VarParams=168 block32 layout=legacy-restored":"READY gfx1201 VarParams=168 block32 layout=direct-experimental");
 }catch(...){log("FALLBACK initialization exception");}
}
inline void initialize_mh() noexcept {
 try {
  auto hip=GetModuleHandleW(L"amdhip64_7.dll");if(!hip){log("MH FALLBACK missing HIP runtime");return;}
  const auto properties=nrhip::api().get_device_properties;
  const auto current=nrhip::api().get_device;
  const auto load=nrhip::api().module_load_data;
  const auto function=nrhip::api().module_get_function;
  const auto by_symbol=nrhip::api().get_func_by_symbol;
  const auto name=nrhip::api().kernel_name_ref;
  mh::module_launch=nrhip::api().module_launch_kernel;
  if(!properties||!current||!load||!function||!by_symbol||!name||!mh::module_launch){log("MH FALLBACK HIP API missing");return;}
  if(!get_device)get_device=current;
  if(!nrhip::dispositivo_ok(current,properties,mh::device)){log("MH FALLBACK device is not gfx1201 wave32");return;}
  const char* native_names[3]{"_Z10k_swin_varILi64ELb0EEv9VarParams","_Z10k_swin_varILi128ELb0EEv9VarParams","_Z10k_swin_varILi256ELb0EEv9VarParams"};
  for(unsigned i=0;i<3;++i){
   void* native_function{};
   if(by_symbol(&native_function,mh::tokens[i])){log("MH FALLBACK native Swin token unresolved");return;}
   const auto native_name=name(native_function);
   if(!native_name||std::strcmp(native_name,native_names[i])){log("MH FALLBACK native Swin ABI identity mismatch");return;}
  }
  void* mh_module{};void* layout{};
  if(load(&mh_module,mh::image.data())){log("MH FALLBACK module load rejected");return;}
  mh::module_handle=mh_module;
  for(unsigned w=0;w<3;++w)for(unsigned i=0;i<8;++i)
   if(function(&mh::functions[w][i],mh_module,nrmhcontract::symbols[w][i])||!mh::functions[w][i]){log("MH FALLBACK mh symbol missing");return;}
  if(load(&layout,mh::layout_image.data())||function(&mh::to_mh,layout,nrmhcontract::to_mh)||function(&mh::to_legacy,layout,nrmhcontract::to_legacy)||!mh::to_mh||!mh::to_legacy){log("MH FALLBACK layout module rejected");return;}
  if(fast::requested&&(fast::mask&1u)){
   fast::load();void* f[3][8]{};bool ok=true;const auto t=fast::table();
   for(unsigned w=0;ok&&w<3;++w)ok=fast::resolve(function,t.mh[w],f[w],8);
   if(ok){for(unsigned w=0;w<3;++w)for(unsigned i=0;i<8;++i)mh::functions[w][i]=f[w][i];mh::wide=true;
    char line[96]{};NRLOGF(line,sizeof line,"MH FAST %s 24 kernels (VarParams 176 B, cauda zerada)",t.tag);log(line);}
   else log("MH FAST FALLBACK modulo ou simbolo recusado; mh fica no v0.4.1");

   for(unsigned w=0;ok&&w<3;++w){
    if(mh::r55_image[w].empty())continue;
    const auto* r=mh::r55_borda[w]?nr1080::r55_mh_borda(w):nr1080::r55_mh(w);char line[200]{};int n=0;
    if(fast::version!=43||load(&mh::r55_module[w],mh::r55_image[w].data())||!mh::r55_module[w]){
     mh::r55_module[w]=nullptr;NRLOGF(line,sizeof line,"R55 %s FALLBACK modulo recusado ou Fast v0.4.3 ausente; fica o Fast",r->tag);log(line);continue;}
    n=NRLOGF(line,sizeof line,"R55 %s READY %ls modos",r->tag,r->file);
    for(unsigned i=0;i<8;++i){
     if(!nr1080::r55_mh_mode(nrmhcontract::modes[i],true))continue;
     void* g{};
     if(function(&g,mh::r55_module[w],t.mh[w][i])||!g){n+=NRLOGF(line+n,sizeof line-n," %u:FALLBACK",nrmhcontract::modes[i]);continue;}
     mh::functions[w][i]=g;n+=NRLOGF(line+n,sizeof line-n," %u",nrmhcontract::modes[i]);
    }
    log(line);
   }
  }

  mh::ready.store(true,std::memory_order_release);
  log("MH READY gfx1201 VarParams=168 block=W layout=mh-converted modes=0,1,2,4,5,8");
 }catch(...){log("MH FALLBACK initialization exception");}
}

inline bool on_owner_device(int owner) noexcept {
 int current=-1;
 const bool known=!get_device||!get_device(&current);
 const int dev=get_device?current:owner;
 return nrmhpolicy::may_flush(known,dev,owner);
}
inline int mh_convert(bool to_mh,void* data,std::uint64_t bytes,unsigned shift,void* stream) {
 unsigned long long count=bytes;unsigned lw=shift;void* args[]{&data,&count,&lw};
 return timed_conversion(to_mh?"@conv_mh_in":"@conv_mh_out",stream,[&]{
  return mh::module_launch(to_mh?mh::to_mh:mh::to_legacy,static_cast<unsigned>(bytes/(512ull<<shift)),1,1,256,1,1,0,stream,args,nullptr);});
}
inline int mh_fail(int error) noexcept {
 dispatch.failure.store(error,std::memory_order_release);dispatch.ready.store(false,std::memory_order_release);
 mh::ready.store(false,std::memory_order_release);vit::ready.store(false,std::memory_order_release);log("MH DISABLED launch failed; error propagated without duplicate submission");return error;
}

inline int mh_flush_pending() noexcept {
 if(!mh::chain_pendente.load(std::memory_order_acquire))return 0;
 if(!on_owner_device(mh::device))return 0;
 std::lock_guard<std::mutex> lock(mh::chain_mutex);
 nrmhpolicy::Tensor t{};
 const bool tinha=mh::chain.take(t);
 mh::chain_pendente.store(mh::chain.pending,std::memory_order_release);
 if(!tinha)return 0;
 if(nrtimeline::requested)nrtimeline::flushed=true;
 if(const int e=mh_convert(false,t.ptr,t.bytes,t.shift,t.stream))return mh_fail(e);
 return 0;
}

inline int mh_flush_park(int w) noexcept {
 if(!on_owner_device(mh::device))return 0;
 std::lock_guard<std::mutex> lock(mh::chain_mutex);
 nrmhpolicy::Tensor t{};
 if(!mh::chain.take_park(w,t))return 0;
 if(nrtimeline::requested)nrtimeline::flushed=true;
 if(const int e=mh_convert(false,t.ptr,t.bytes,t.shift,t.stream))return mh_fail(e);
 return 0;
}
inline int mh_launch(int w,const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 std::call_once(mh::initialization,initialize_mh);
 if(const int failed=dispatch.failure.load(std::memory_order_acquire))return failed;
 int current=-1;
 if(!mh::ready.load(std::memory_order_acquire)||(get_device&&(get_device(&current)||current!=mh::device))||
    b.x!=256||b.y!=1||b.z!=1||shared||!args||!args[0]){

  const bool here=on_owner_device(mh::device);
  if(here){if(const int e=mh_flush_pending())return e;}
  if(here){if(const int e=mh_flush_park(w))return e;}
  return fallback(f,g,b,args,shared,stream);
 }
 const unsigned window=64u<<w;
 nrmhpolicy::Plan p{};
 if(!nrmhpolicy::plan(window,args[0],p)){
  if(const int e=mh_flush_pending())return e;
  if(const int e=mh_flush_park(w))return e;
  unsigned mode{};std::memcpy(&mode,static_cast<const char*>(args[0])+0x28,sizeof(mode));
  if(mode<16&&potencia2(mh::refused_count[w*16+mode])){
   const char* b0=static_cast<const char*>(args[0]);int hh{},ww{};std::uint64_t in{},out{},ax30{},ax38{};std::memcpy(&hh,b0+0x18,4);std::memcpy(&ww,b0+0x1c,4);
   std::memcpy(&in,b0,8);std::memcpy(&out,b0+8,8);std::memcpy(&ax30,b0+0x30,8);std::memcpy(&ax38,b0+0x38,8);
   const std::uint64_t bytes=std::uint64_t(hh)*std::uint64_t(ww)*window;
   char line[360]{};NRLOGF(line,sizeof line,"MH ORIGINAL window=%u mode=%u (modo nao provado ou contrato recusado) vezes=%llu h=%d w=%d bytes=%llu mod=%llu in=%llx out=%llx aux30=%llx aux38=%llx",
    window,mode,mh::refused_count[w*16+mode].load(std::memory_order_relaxed),hh,ww,(unsigned long long)bytes,(unsigned long long)(bytes%(512ull<<nrmhpolicy::layout_shift(window))),(unsigned long long)in,(unsigned long long)out,(unsigned long long)ax30,(unsigned long long)ax38);log(line);
  }
  return fallback(f,g,b,args,shared,stream);
 }

 const auto fail=&mh_fail;
 std::lock_guard<std::mutex> lock(mh::chain_mutex);

 struct Espelho {~Espelho(){mh::chain_pendente.store(mh::chain.pending,std::memory_order_release);}} espelho;
 const auto step=mh::chain.begin(p,p.mh_mode,stream,mh::chain_enabled);
 if(step.flush){if(nrtimeline::requested)nrtimeline::flushed=true;if(const int e=mh_convert(false,step.flush_tensor.ptr,step.flush_tensor.bytes,step.flush_tensor.shift,step.flush_tensor.stream))return fail(e);}
 if(step.flush_park){if(nrtimeline::requested)nrtimeline::flushed=true;if(const int e=mh_convert(false,step.park_tensor.ptr,step.park_tensor.bytes,step.park_tensor.shift,step.park_tensor.stream))return fail(e);}
 if(p.convert_input&&!step.input_ready){if(const int e=mh_convert(true,p.input,p.bytes,p.shift,stream))return fail(e);}
 if(p.convert_aux){if(const int e=mh_convert(true,p.aux,p.bytes,p.shift,stream))return fail(e);}
 alignas(16) unsigned char wide_params[192]{};void* wide_args[]{wide_params};
 if(mh::wide)std::memcpy(wide_params,args[0],168);
 if(const int e=mh::module_launch(mh::functions[w][p.mode_index],g.x,g.y,g.z,window,1,1,0,stream,mh::wide?wide_args:args,nullptr))return fail(e);

 if(nrmhpolicy::restore_input(p,mh::drop_restore)){if(const int e=mh_convert(false,p.input,p.bytes,p.shift,stream))return fail(e);}
 if(p.convert_aux){if(const int e=mh_convert(false,p.aux,p.bytes,p.shift,stream))return fail(e);}

 if(p.convert_output&&!step.defer_output&&!step.park_output){if(const int e=mh_convert(false,p.output,p.bytes,p.shift,stream))return fail(e);}
 mh::chain.end(p,stream,step);
 if(step.input_ready&&!mh::chain_logged.exchange(true,std::memory_order_relaxed)){
  char line[160]{};NRLOGF(line,sizeof line,"MH CHAIN window=%u mh_mode=%u input pendente reaproveitado (sem conversao de entrada)",window,p.mh_mode);log(line);
 }
 if(nrtimeline::requested||NR_R6_DENTRO)nrtimeline::replaced=true;
 const unsigned slot=static_cast<unsigned>(w)*8u+static_cast<unsigned>(p.mode_index);
 if(!mh::logged[slot].exchange(true,std::memory_order_relaxed)){
  char line[200]{};NRLOGF(line,sizeof line,"MH DISPATCH window=%u mh_mode=%u grid=%u,%u,%u block=%u in=%d out=%d stream=%p result=0",window,p.mh_mode,g.x,g.y,g.z,window,p.convert_input,p.convert_output,stream);log(line);
 }
 return 0;
}
inline void initialize_1d() noexcept {
 try {
  std::call_once(mh::initialization,initialize_mh);
  if(!mh::ready.load(std::memory_order_acquire)||!mh::module_handle){log("1D FALLBACK mh module not ready");return;}
  auto hip=GetModuleHandleW(L"amdhip64_7.dll");if(!hip){log("1D FALLBACK missing HIP runtime");return;}
  const auto load=nrhip::api().module_load_data;
  const auto function=nrhip::api().module_get_function;
  const auto by_symbol=nrhip::api().get_func_by_symbol;
  const auto name=nrhip::api().kernel_name_ref;
  const auto alloc=nrhip::api().malloc_;
  if(!load||!function||!by_symbol||!name||!alloc){log("1D FALLBACK HIP API missing");return;}
  void* native{};const char* n{};
  if(by_symbol(&native,c1d::token)||!(n=name(native))||std::strcmp(n,"_Z11k_contract212ConvParams1d")){log("1D FALLBACK native contract2 identity mismatch");return;}
  if(c1d::image.empty()||load(&c1d::module_handle,c1d::image.data())||!c1d::module_handle){log("1D FALLBACK v040 module load rejected");return;}
  if(function(&c1d::conv,c1d::module_handle,nrmhcontract::conv1d)||function(&c1d::reduce,c1d::module_handle,nrmhcontract::reduce1d)||!c1d::conv||!c1d::reduce){log("1D FALLBACK reg1d symbols missing");return;}

  if(alloc(&c1d::workspace,nr1dpolicy::workspace_bytes)||!c1d::workspace){log("1D FALLBACK workspace allocation");return;}
  c1d::ready.store(true,std::memory_order_release);
  log("1D READY contract2 -> reg1d_conv<1,true>+reduce, workspace 4 MiB");
 }catch(...){log("1D FALLBACK initialization exception");}
}
inline int launch_1d(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 std::call_once(c1d::initialization,initialize_1d);
 if(const int failed=dispatch.failure.load(std::memory_order_acquire))return failed;
 int current=-1;
 nr1dpolicy::Plan p{};
 const bool ok=c1d::ready.load(std::memory_order_acquire)&&!(get_device&&(get_device(&current)||current!=mh::device))&&args&&args[0]&&
  nr1dpolicy::plan(args[0],{g.x,g.y,g.z},{b.x,b.y,b.z},shared,p);
 std::lock_guard<std::mutex> lock(c1d::mutex);

 if(ok&&!c1d::stream_known){c1d::stream=stream;c1d::stream_known=true;}
 if(!ok||stream!=c1d::stream){
  if(potencia2(c1d::refused_count)){char line[200]{};NRLOGF(line,sizeof line,"1D ORIGINAL contract2 fora do contrato provado (forma, stream ou modulo) vezes=%llu grid=%u,%u,%u stream=%p",c1d::refused_count.load(std::memory_order_relaxed),g.x,g.y,g.z,stream);log(line);}
  return fallback(f,g,b,args,shared,stream);
 }
 unsigned char params[nr1dpolicy::args_bytes];nr1dpolicy::build_args(args[0],c1d::workspace,p.n,params);
 void* kargs[]{params};
 if(const int e=mh::module_launch(c1d::conv,p.conv.x,p.conv.y,p.conv.z,256,1,1,0,stream,kargs,nullptr))return mh_fail(e);
 if(const int e=mh::module_launch(c1d::reduce,p.reduce.x,p.reduce.y,p.reduce.z,256,1,1,0,stream,kargs,nullptr))return mh_fail(e);
 if(nrtimeline::requested||NR_R6_DENTRO)nrtimeline::replaced=true;
 if(!c1d::logged.exchange(true,std::memory_order_relaxed)){
  char line[160]{};NRLOGF(line,sizeof line,"1D DISPATCH contract2 N=%u splits=%u conv=%u,%u,%u reduce=%u stream=%p result=0",p.n,p.splits,p.conv.x,p.conv.y,p.conv.z,p.reduce.x,stream);log(line);
 }
 return 0;
}
inline void initialize_1d43() noexcept {
 try {
  std::call_once(mh::initialization,initialize_mh);
  if(!mh::ready.load(std::memory_order_acquire)){log("1D43 FALLBACK mh module not ready");return;}
  auto hip=GetModuleHandleW(L"amdhip64_7.dll");if(!hip){log("1D43 FALLBACK missing HIP runtime");return;}
  const auto load=nrhip::api().module_load_data;
  const auto function=nrhip::api().module_get_function;
  const auto by_symbol=nrhip::api().get_func_by_symbol;
  const auto name=nrhip::api().kernel_name_ref;
  const auto alloc=nrhip::api().malloc_;
  c1d43::memset_async=nrhip::api().memset_async;
  if(!load||!function||!by_symbol||!name||!alloc||!c1d43::memset_async){log("1D43 FALLBACK HIP API missing");return;}
  const char* native_names[5]{"_Z8k_repack12RepackParams","_Z9k_expand212ExpandParams","_Z11k_contract212ConvParams1d","_Z6k_qkv29QkvParams","_Z12k_attention212AttnParams1d"};
  const char* foreign_names[5]{"_Z8k_expand12ExpandParams","_Z13k_conv_splitk12ConvParams1d","_Z5k_qkv9QkvParams","_Z11k_attention12AttnParams1d","_Z14k_dec_upsample11DecUpParams"};
  for(unsigned i=0;i<10;++i){
   void* native{};const char* n{};const void* t=i<5?c1d43::tokens[i]:c1d43::foreign[i-5];
   if(by_symbol(&native,t)||!(n=name(native))||std::strcmp(n,i<5?native_names[i]:foreign_names[i-5])){log("1D43 FALLBACK native 1D identity mismatch");return;}
  }
  if(c1d43::image.empty()||load(&c1d43::module_handle,c1d43::image.data())||!c1d43::module_handle){log("1D43 FALLBACK module load rejected");return;}
  for(unsigned i=0;i<nr1d43policy::fn_count;++i)
   if(function(&c1d43::functions[i],c1d43::module_handle,nr1d43contract::symbols[i])||!c1d43::functions[i]){log("1D43 FALLBACK symbol missing");return;}

  if(alloc(&c1d43::buffers.workspace,nr1d43policy::workspace_bytes)||!c1d43::buffers.workspace||
     alloc(&c1d43::buffers.counters,nr1d43policy::counter_bytes)||!c1d43::buffers.counters){log("1D43 FALLBACK allocation");return;}

  if(rw1d::requested&&c1d43::fast){
   bool ok=!rw1d::image.empty()&&!load(&rw1d::module,rw1d::image.data())&&rw1d::module;
   for(unsigned i=0;ok&&i<nr1080::contract::rw_count;++i)ok=!function(&rw1d::functions[i],rw1d::module,nr1080::contract::rw1d[i])&&rw1d::functions[i];
   rw1d::ready=ok;
   log(ok?(rw1d::split?"1D RW READY nr_1d_rw.hsaco conv/expand/attn (conv tambem no lugar do split)":"1D RW READY nr_1d_rw.hsaco conv/expand/attn (split fica no Daniel)"):"1D RW FALLBACK nr_1d_rw.hsaco recusado; cadeia 1D43 do Daniel");
  }
  if(r55d1::requested&&c1d43::fast){
   bool ok=!r55d1::image.empty()&&!load(&r55d1::module,r55d1::image.data())&&r55d1::module;
   for(unsigned i=0;ok&&i<nr1080::contract::r55_qkv_count;++i)ok=!function(&r55d1::qkv[i],r55d1::module,nr1080::contract::r55_qkv[i])&&r55d1::qkv[i];
   r55d1::ready=ok;
   log(ok?"R55 1D READY nr_r55_1d.hsaco 28288380: qkv_fast -> k1d_qkv_* (bit a bit)":"R55 1D FALLBACK nr_r55_1d.hsaco recusado; qkv do Daniel");
  }
  c1d43::ready.store(true,std::memory_order_release);
  log(c1d43::fast?"1D43 READY cadeia 1D reg v0.4.3 Fast (repack/expand2/conv/qkv/attn)":"1D43 READY cadeia 1D reg v0.4.3 Reference (diagnostico)");
 }catch(...){log("1D43 FALLBACK initialization exception");}
}
inline int launch_decup(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream);

inline int launch_1d43(int k,const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 std::call_once(c1d43::initialization,initialize_1d43);
 if(const int failed=dispatch.failure.load(std::memory_order_acquire))return failed;
 using namespace nr1d43policy;
 bool legacy=false;
 {
  std::lock_guard<std::mutex> lock(c1d43::mutex);
  const auto fatal=[&](const char* why){char line[200]{};NRLOGF(line,sizeof line,"1D43 FATAL %s kind=%d grid=%u,%u,%u block=%u (job envenenado)",why,k,g.x,g.y,g.z,b.x);log(line);return mh_fail(1);};
  int current=-1;
  const bool device_ok=!(get_device&&(get_device(&current)||current!=mh::device));
  const bool ready=c1d43::ready.load(std::memory_order_acquire)&&device_ok&&args&&args[0];
  if(k>=5){if(c1d43::chain.active)return fatal("kernel 1D legado no meio da cadeia");legacy=true;}
  else if(!c1d43::chain.active){
   Plan p{};
   if(k!=int(Kind::repack)||!ready||!plan_repack(args[0],{g.x,g.y,g.z},{b.x,b.y,b.z},shared,p)||!p.forward||(c1d43::stream_known&&stream!=c1d43::stream)){
    if(k==int(Kind::repack)&&potencia2(c1d43::refused_count)){int rn{},rd{},ra{},rw{};if(args&&args[0]){const auto* q=static_cast<const char*>(args[0]);std::memcpy(&ra,q+0x10,4);std::memcpy(&rw,q+0x14,4);std::memcpy(&rn,q+0x18,4);std::memcpy(&rd,q+0x1c,4);}
     char line[240]{};NRLOGF(line,sizeof line,"1D43 ORIGINAL k_repack fora do contrato vezes=%llu (grid=%u,%u,%u block=%u stream=%p N=%d dir=%d a=%d w=%d)",c1d43::refused_count.load(std::memory_order_relaxed),g.x,g.y,g.z,b.x,stream,rn,rd,ra,rw);log(line);}
    legacy=true;
   }else{
    if(!c1d43::stream_known){c1d43::stream=stream;c1d43::stream_known=true;}
    if(!c1d43::counters_zeroed){if(const int e=c1d43::memset_async(c1d43::buffers.counters,0,counter_bytes,stream))return mh_fail(e);c1d43::counters_zeroed=true;}
    void* kargs[]{p.args};
    if(const int e=mh::module_launch(c1d43::functions[p.fn],p.grid.x,p.grid.y,p.grid.z,p.block.x,p.block.y,p.block.z,0,stream,kargs,nullptr))return mh_fail(e);
    c1d43::chain.begin(p.n,stream);
    if(!c1d43::logged[p.fn].exchange(true,std::memory_order_relaxed)){char line[160]{};NRLOGF(line,sizeof line,"1D43 DISPATCH repack ida N=%u grid=%u block=%u stream=%p",p.n,g.x,b.x,stream);log(line);}
   }
  }else{
   if(!ready||stream!=c1d43::chain.stream)return fatal("dispositivo/stream fora da cadeia");
   Plan p{};const unsigned n=c1d43::chain.n;const nr1d43policy::Dim gg{g.x,g.y,g.z},bb{b.x,b.y,b.z};bool ok=false;
   switch(static_cast<Kind>(k)){
    case Kind::repack: ok=plan_repack(args[0],gg,bb,shared,p)&&!p.forward&&p.n==n;break;
    case Kind::expand: ok=plan_expand(args[0],gg,bb,shared,n,p);break;
    case Kind::contract: ok=plan_contract(args[0],gg,bb,shared,n,c1d43::buffers,c1d43::fast,p,c1d43::conv_split,c1d43::split_teto);break;
    case Kind::qkv: ok=plan_qkv(args[0],gg,bb,shared,n,c1d43::buffers,c1d43::fast,p,c1d43::qkv_split_max);break;
    case Kind::attention: ok=plan_attention(args[0],gg,bb,shared,n,c1d43::fast,p);break;
   }
   if(!ok)return fatal("lancamento fora do plano");

   void* fn=c1d43::functions[p.fn];nr1080::Pick rw{};bool use_rw=false;
   if(rw1d::ready&&c1d43::fast){
    switch(static_cast<Kind>(k)){
     case Kind::contract: use_rw=(p.fn==conv_fast||(p.fn==conv_split_fast&&rw1d::split))&&nr1080::rw_conv(n,p.args,rw);break;
     case Kind::expand: use_rw=p.fn==expand&&nr1080::rw_expand(n,rw);break;
     case Kind::attention: use_rw=p.fn==attn_fast&&nr1080::rw_attn(n,p.args,rw);break;
     default: break;
    }
    if(use_rw){fn=rw1d::functions[rw.fn];p.grid={rw.grid.x,rw.grid.y,rw.grid.z};p.block={rw.block.x,rw.block.y,rw.block.z};}
   }

   nr1080::Pick r55{};bool use_r55=false;
   if(r55d1::ready&&c1d43::fast&&static_cast<Kind>(k)==Kind::qkv&&p.fn==qkv_fast&&nr1080::r55_qkv(n,p.args,r55)){
    use_r55=true;fn=r55d1::qkv[r55.fn];p.grid={r55.grid.x,r55.grid.y,r55.grid.z};p.block={r55.block.x,r55.block.y,r55.block.z};
   }
   void* kargs[]{p.args};
   if(const int e=mh::module_launch(fn,p.grid.x,p.grid.y,p.grid.z,p.block.x,p.block.y,p.block.z,0,stream,kargs,nullptr))return mh_fail(e);
   if(static_cast<Kind>(k)==Kind::repack)c1d43::chain.end(stream);
   if(use_r55){if(!r55d1::logged[r55.fn].exchange(true,std::memory_order_relaxed)){char line[200]{};NRLOGF(line,sizeof line,"R55 1D DISPATCH fn=%s (no lugar de %s) N=%u grid=%u,%u,%u block=%u stream=%p",nr1080::contract::r55_qkv[r55.fn],nr1d43contract::symbols[p.fn],n,p.grid.x,p.grid.y,p.grid.z,p.block.x,stream);log(line);}}
   else if(use_rw){if(!rw1d::logged[rw.fn].exchange(true,std::memory_order_relaxed)){char line[200]{};NRLOGF(line,sizeof line,"1D RW DISPATCH fn=%s (no lugar de %s) N=%u grid=%u,%u,%u block=%u stream=%p",nr1080::contract::rw1d[rw.fn],nr1d43contract::symbols[p.fn],n,p.grid.x,p.grid.y,p.grid.z,p.block.x,stream);log(line);}}
   else if(!c1d43::logged[p.fn].exchange(true,std::memory_order_relaxed)){char line[200]{};NRLOGF(line,sizeof line,"1D43 DISPATCH fn=%s N=%u grid=%u,%u,%u block=%u stream=%p",nr1d43contract::symbols[p.fn],n,p.grid.x,p.grid.y,p.grid.z,p.block.x,stream);log(line);}
  }
 }
 if(legacy){
  if(k==int(nr1d43policy::Kind::contract)&&c1d::requested)return launch_1d(f,g,b,args,shared,stream);
  if(f==decup::token&&decup::requested)return launch_decup(f,g,b,args,shared,stream);
  return fallback(f,g,b,args,shared,stream);
 }
 if(nrtimeline::requested||NR_R6_DENTRO)nrtimeline::replaced=true;
 return 0;
}
inline void initialize_vit() noexcept {
 try {
  std::call_once(mh::initialization,initialize_mh);
  if(!mh::ready.load(std::memory_order_acquire)||!mh::module_handle){log("VIT FALLBACK mh module not ready");return;}
  auto hip=GetModuleHandleW(L"amdhip64_7.dll");if(!hip){log("VIT FALLBACK missing HIP runtime");return;}
  const auto load=nrhip::api().module_load_data;
  const auto function=nrhip::api().module_get_function;
  const auto by_symbol=nrhip::api().get_func_by_symbol;
  const auto name=nrhip::api().kernel_name_ref;
  if(!load||!function||!by_symbol||!name){log("VIT FALLBACK HIP API missing");return;}
  const char* native_names[4]{"_Z7k_ffwd211Ffwd2Params","_Z11k_conv_res211Conv2Params","_Z11k_qkv_attn210AttnParams","_Z16k_conv_res_views12ConvPlParams"};
  for(unsigned i=0;i<4;++i){
   void* native{};const char* n{};
   if(by_symbol(&native,vit::tokens[i])||!(n=name(native))||std::strcmp(n,native_names[i])){log("VIT FALLBACK native vit512 identity mismatch");return;}
  }
  for(unsigned i=0;i<nrvitpolicy::fn_count;++i)
   if(function(&vit::functions[i],mh::module_handle,nrmhcontract::vit[i])||!vit::functions[i]){log("VIT FALLBACK reg_vit symbol missing");return;}
  static_assert(nrvitpolicy::fn_count==7,"fast_vit tem 7 simbolos");
  if(nrvitpolicy::views_exato){vit::conv4_v041=vit::functions[nrvitpolicy::conv4];
   log(vit::conv4_v041?"VIT VIEWS EXATO READY conv<1,4> v0.4.1 no pooled parcial (NeuralRegVitViewsExato=1)":"VIT VIEWS EXATO FALLBACK simbolo v0.4.1 ausente; original");}
  if(fast::requested&&(fast::mask&4u)){
   fast::load();void* f[7]{};const auto t=fast::table();
   if(fast::resolve(function,t.vit,f,7)){for(unsigned i=0;i<7;++i)vit::functions[i]=f[i];vit::attn2=t.attn2;
    char line[128]{};NRLOGF(line,sizeof line,"VIT FAST %s 7 kernels (structs iguais; attn %s)",t.tag,t.attn2?"attn2 bloco 64 z 16":"bloco 128 z 4");log(line);}
   else log("VIT FAST FALLBACK modulo ou simbolo recusado; vit fica no v0.4.1");
  }
  void* layout{};
  if(load(&layout,vit::layout_image.data())||function(&vit::to_frag,layout,nrmhcontract::vit_to_frag)||function(&vit::to_tin,layout,nrmhcontract::vit_to_tin)||!vit::to_frag||!vit::to_tin){log("VIT FALLBACK layout module rejected");return;}

  if(vitx::requested&&vit::attn2&&fast::version==43){
   void* m{};void* ma{};
   const bool ok=!vitx::image.empty()&&!load(&m,vitx::image.data())&&m&&!function(&vitx::conv,m,nr1080::contract::vitx_conv)&&vitx::conv&&
                 !function(&vitx::ffwd,m,nr1080::contract::vitx_ffwd)&&vitx::ffwd;
   const bool ok_attn=!vitx::attn_image.empty()&&!load(&ma,vitx::attn_image.data())&&ma&&!function(&vitx::attn,ma,nr1080::contract::vitx_attn)&&vitx::attn;
   if(!ok)vitx::conv=vitx::ffwd=nullptr;
   if(!ok_attn)vitx::attn=nullptr;
   char line[160]{};NRLOGF(line,sizeof line,"VITX %s conv=%d ffwd=%d attn2=%d (mask=%u)",ok&&ok_attn?"READY":"FALLBACK parcial",vitx::conv&&(vitx::mask&1u)?1:0,vitx::ffwd&&(vitx::mask&2u)?1:0,vitx::attn&&(vitx::mask&4u)?1:0,vitx::mask);log(line);
  }else if(vitx::requested)log("VITX FALLBACK vit fora do Fast v0.4.3; kernels do Daniel");

  if(decup::requested){
   if(fast::module&&fast::version==43&&!function(&decup::function,fast::module,nr1080::contract::decup)&&decup::function)log("DECUP READY k_reg_decup<true> (v0.4.3) no lugar do k_dec_upsample, saida em frag512 rastreada");
   else {decup::function=nullptr;log("DECUP FALLBACK modulo Fast v0.4.3 ausente ou simbolo recusado; k_dec_upsample original");}
  }
  vit::ready.store(true,std::memory_order_release);
  log("VIT READY ffwd2/conv_res2/qkv_attn2/conv_res_views -> reg_vit (frag512 rastreado, conversao nas pontas)");
 }catch(...){log("VIT FALLBACK initialization exception");}
}
inline int vit_convert(bool frag,void* data,std::uint64_t bytes,void* stream) {
 unsigned long long count=bytes;void* args[]{&data,&count};
 return timed_conversion(frag?"@conv_vit_in":"@conv_vit_out",stream,[&]{
  return mh::module_launch(frag?vit::to_frag:vit::to_tin,static_cast<unsigned>((bytes+1023)/1024),1,1,256,1,1,0,stream,args,nullptr);});
}

inline int vit_flush_locked() noexcept {
 nrvitpolicy::Tensor t[nrvitpolicy::Tracker::cap];void* stream{};
 const unsigned n=vit::tracker.take_all(t,stream);vit::any_tracked.store(false,std::memory_order_release);
 if(n&&nrtimeline::requested)nrtimeline::flushed=true;
 for(unsigned i=0;i<n;++i)if(const int e=vit_convert(false,t[i].ptr,t[i].bytes,stream))return mh_fail(e);
 if(n&&!vit::flush_logged.exchange(true,std::memory_order_relaxed)){char line[96]{};NRLOGF(line,sizeof line,"VIT FLUSH %u tensor(es) de volta a tin",n);log(line);}
 return 0;
}

inline bool vit_device_ok() noexcept {int current=-1;return !(get_device&&(get_device(&current)||current!=mh::device));}
inline int vit_flush() noexcept {
 if(!vit::any_tracked.load(std::memory_order_acquire)||!vit_device_ok())return 0;
 std::lock_guard<std::mutex> lock(vit::mutex);return vit_flush_locked();
}
inline int vit_launch(int k,const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 std::call_once(vit::initialization,initialize_vit);
 if(const int failed=dispatch.failure.load(std::memory_order_acquire))return failed;
 nrvitpolicy::Plan p{};
 const bool device_ok=vit_device_ok();
 const bool ok=vit::ready.load(std::memory_order_acquire)&&device_ok&&args&&args[0]&&
  nrvitpolicy::plan(static_cast<nrvitpolicy::Kind>(k),args[0],{g.x,g.y,g.z},{b.x,b.y,b.z},shared,p,vit::attn2)&&(!p.exato||vit::conv4_v041);
 std::lock_guard<std::mutex> lock(vit::mutex);
 nrvitpolicy::Tracker::Before bf{};
 if(!ok||!vit::tracker.before(p,stream,bf)){

  if(device_ok){if(const int e=vit_flush_locked())return e;}
  if(potencia2(vit::refused_count[k])){char line[160]{};NRLOGF(line,sizeof line,"VIT ORIGINAL kind=%d grid=%u,%u,%u (fora do contrato provado) vezes=%llu",k,g.x,g.y,g.z,vit::refused_count[k].load(std::memory_order_relaxed));log(line);}
  return fallback(f,g,b,args,shared,stream);
 }

 for(unsigned i=0;i<bf.n_tin;++i)if(const int e=vit_convert(false,bf.to_tin[i].ptr,bf.to_tin[i].bytes,stream))return mh_fail(e);
 for(unsigned i=0;i<bf.n_frag;++i)if(const int e=vit_convert(true,bf.to_frag[i].ptr,bf.to_frag[i].bytes,stream))return mh_fail(e);
 void* kargs[]{p.args};

 void* fn=vit::functions[p.fn];int x=-1;
 if(p.exato){fn=vit::conv4_v041;
  if(!vit::exato_logged.exchange(true,std::memory_order_relaxed)){char line[160]{};NRLOGF(line,sizeof line,"VIT VIEWS EXATO DISPATCH conv<1,4> v0.4.1 grid=%u,%u,%u block=%u stream=%p",p.grid.x,p.grid.y,p.grid.z,p.block.x,stream);log(line);}}
 if(p.fn==nrvitpolicy::conv0&&vitx::conv&&(vitx::mask&1u)){fn=vitx::conv;x=0;}
 else if(p.fn==nrvitpolicy::ffwd0&&vitx::ffwd&&(vitx::mask&2u)){fn=vitx::ffwd;x=1;}
 else if(p.fn==nrvitpolicy::attn&&vit::attn2&&vitx::attn&&(vitx::mask&4u)){fn=vitx::attn;x=2;}
 if(const int e=mh::module_launch(fn,p.grid.x,p.grid.y,p.grid.z,p.block.x,p.block.y,p.block.z,0,stream,kargs,nullptr))return mh_fail(e);
 vit::tracker.after(p,stream,bf);vit::any_tracked.store(!vit::tracker.empty(),std::memory_order_release);
 if(nrtimeline::requested||NR_R6_DENTRO)nrtimeline::replaced=true;
 if(x>=0&&!vitx::logged[unsigned(x)].exchange(true,std::memory_order_relaxed)){
  static constexpr const char* names[3]{"k_vitx_conv","k_vitx_ffwd","k_vitx_attn2"};
  char line[160]{};NRLOGF(line,sizeof line,"VITX DISPATCH %s grid=%u,%u,%u block=%u stream=%p",names[x],p.grid.x,p.grid.y,p.grid.z,p.block.x,stream);log(line);
 }
 if(!vit::logged[p.fn].exchange(true,std::memory_order_relaxed)){
  char line[200]{};NRLOGF(line,sizeof line,"VIT DISPATCH fn=%u grid=%u,%u,%u block=%u to_frag=%u to_tin=%u tracked=%u stream=%p result=0",unsigned(p.fn),p.grid.x,p.grid.y,p.grid.z,p.block.x,bf.n_frag,bf.n_tin,vit::tracker.count,stream);log(line);
 }
 return 0;
}

inline int launch_decup(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 std::call_once(vit::initialization,initialize_vit);
 if(const int failed=dispatch.failure.load(std::memory_order_acquire))return failed;
 nr1080::Decup d{};
 const bool device_ok=vit_device_ok();
 const bool ok=vit::ready.load(std::memory_order_acquire)&&decup::function&&device_ok&&args&&args[0]&&
  nr1080::decup(args[0],{g.x,g.y,g.z},{b.x,b.y,b.z},shared,d);
 std::lock_guard<std::mutex> lock(vit::mutex);
 auto& t=vit::tracker;
 const int kin=ok?t.find(d.vit512a):-1,kout=ok?t.find(d.out):-1;
 const bool fits=ok&&(!t.count||t.stream==stream)&&(kin<0||t.held[kin].bytes==d.dense_bytes)&&(kout<0||t.held[kout].bytes==d.dense_bytes)&&
  t.count+(kin<0?1u:0u)+(kout<0?1u:0u)<=nrvitpolicy::Tracker::cap;
 if(!fits){
  if(device_ok){if(const int e=vit_flush_locked())return e;}
  if(!decup::refused_logged.exchange(true,std::memory_order_relaxed)){char line[160]{};NRLOGF(line,sizeof line,"DECUP ORIGINAL grid=%u,%u,%u block=%u (fora do contrato)",g.x,g.y,g.z,b.x);log(line);}
  return fallback(f,g,b,args,shared,stream);
 }
 if(kin<0){if(const int e=vit_convert(true,d.vit512a,d.dense_bytes,stream))return mh_fail(e);}
 void* kargs[]{d.args};
 if(const int e=mh::module_launch(decup::function,d.grid.x,d.grid.y,d.grid.z,d.block.x,d.block.y,d.block.z,0,stream,kargs,nullptr))return mh_fail(e);
 t.stream=stream;t.add(d.vit512a,d.dense_bytes);t.add(d.out,d.dense_bytes);
 t.stage=0;t.f_out=t.c1_out=t.a_out=nullptr;
 vit::any_tracked.store(true,std::memory_order_release);
 if(nrtimeline::requested||NR_R6_DENTRO)nrtimeline::replaced=true;
 if(!decup::logged.exchange(true,std::memory_order_relaxed)){char line[200]{};NRLOGF(line,sizeof line,"DECUP DISPATCH k_reg_decup<true> grid=%u,%u,%u block=%u bytes=%llu vit512a_ja_frag=%d stream=%p",d.grid.x,d.grid.y,d.grid.z,d.block.x,(unsigned long long)d.dense_bytes,kin>=0?1:0,stream);log(line);}
 return 0;
}
inline void initialize_cauda() noexcept {
 try {
  auto hip=GetModuleHandleW(L"amdhip64_7.dll");if(!hip){log("FINALHEAD FALLBACK missing HIP runtime");return;}
  const auto properties=nrhip::api().get_device_properties;
  const auto current=nrhip::api().get_device;
  const auto load=nrhip::api().module_load_data;
  const auto function=nrhip::api().module_get_function;
  const auto by_symbol=nrhip::api().get_func_by_symbol;
  const auto name=nrhip::api().kernel_name_ref;
  cauda::module_launch=nrhip::api().module_launch_kernel;
  if(!properties||!current||!load||!function||!by_symbol||!name||!cauda::module_launch){log("FINALHEAD FALLBACK HIP API missing");return;}
  if(!get_device)get_device=current;
  if(!nrhip::dispositivo_ok(current,properties,cauda::device)){log("FINALHEAD FALLBACK device is not gfx1201 wave32");return;}
  void* native{};const char* n{};
  if(by_symbol(&native,cauda::token)||!(n=name(native))||std::strcmp(n,"_Z12k_final_head10HeadParams")){log("FINALHEAD FALLBACK native k_final_head identity mismatch");return;}
  if(cauda::image.empty()||load(&cauda::module,cauda::image.data())||!cauda::module||function(&cauda::head,cauda::module,nr1080::contract::final_head)||!cauda::head){log("FINALHEAD FALLBACK nr_cauda.hsaco recusado");return;}
  cauda::ready.store(true,std::memory_order_release);
  log("FINALHEAD READY k_final_head_fast (nr_cauda.hsaco) no lugar do k_final_head, grade (gx,4,1) bloco 128");
 }catch(...){log("FINALHEAD FALLBACK initialization exception");}
}
inline int launch_final_head(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 std::call_once(cauda::initialization,initialize_cauda);
 if(const int failed=dispatch.failure.load(std::memory_order_acquire))return failed;
 nr1080::Dim grid{},block{};int current=-1;
 if(!cauda::ready.load(std::memory_order_acquire)||(get_device&&(get_device(&current)||current!=cauda::device))||!args||!args[0]||
    !nr1080::final_head(args[0],{g.x,g.y,g.z},{b.x,b.y,b.z},shared,grid,block)){
  if(!cauda::refused_logged.exchange(true,std::memory_order_relaxed)){char line[160]{};NRLOGF(line,sizeof line,"FINALHEAD ORIGINAL grid=%u,%u,%u block=%u (fora do contrato)",g.x,g.y,g.z,b.x);log(line);}
  return fallback(f,g,b,args,shared,stream);
 }
 void* kargs[]{args[0]};
 if(const int e=cauda::module_launch(cauda::head,grid.x,grid.y,grid.z,block.x,block.y,block.z,0,stream,kargs,nullptr))return mh_fail(e);
 if(nrtimeline::requested||NR_R6_DENTRO)nrtimeline::replaced=true;
 if(!cauda::logged.exchange(true,std::memory_order_relaxed)){char line[160]{};NRLOGF(line,sizeof line,"FINALHEAD DISPATCH k_final_head_fast grid=%u,%u,%u block=%u stream=%p",grid.x,grid.y,grid.z,block.x,stream);log(line);}
 return 0;
}
inline int capture_forward(const void* f,nrcapture::Dim3 g,nrcapture::Dim3 b,void** args,std::size_t shared,void* stream) {
 return fallback(f,{g.x,g.y,g.z},{b.x,b.y,b.z},args,shared,stream);
}

inline bool sw32_owner_device() noexcept {return on_owner_device(device);}
inline int sw32_flush() noexcept {
 if(!sw32_pendente.load(std::memory_order_acquire))return 0;
 std::lock_guard<std::mutex> lock(sw32_mutex);
 if(!dispatch.pending)return 0;
 if(!on_owner_device(device))return 0;
 if(nrtimeline::requested)nrtimeline::flushed=true;
 const int e=dispatch.flush();
 sw32_pendente.store(dispatch.pending,std::memory_order_release);
 return e;
}

inline void sw32x_sincronizar() noexcept {
 const int v=nrsw32x::escolha.load(std::memory_order_relaxed);
 if(v==sw32x::applied||v<0||v>2)return;
 if(!sw32x::have[v]||!sw32x::have[0]){sw32x::applied=v;return;}
 for(unsigned i=0;i<8;++i)dispatch.functions[i]=sw32x::sets[v][i];
 char line[96]{};NRLOGF(line,sizeof line,"SW32X troca ao vivo -> %s",v==0?"Fast v0.4.3":v==1?"A (bit a bit)":"E (rapida)");log(line);
 sw32x::applied=v;
}

inline int launch_1d_next(int k,const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 if(mh::requested){
  if(c1d43::requested)return launch_1d43(k,f,g,b,args,shared,stream);
  if(c1d::requested&&f==c1d::token)return launch_1d(f,g,b,args,shared,stream);
 }
 return fallback(f,g,b,args,shared,stream);
}
inline void* v05_shadow(void* p,const nrvit1dfull::Geometry& geo,void* stream) noexcept {
 return v05::shadows.get(p,geo.n2,v05::jobs,
  [&]{std::size_t size{};void* base{};if(v05::range&&!v05::range(&base,&size,p)&&base==p)return nrvit1dfull::scaled_bytes(size,geo);return nrvit1dfull::shadow_bytes(geo);},
  [&](std::uint64_t n)->void*{void* m{};if(v05::malloc_fn(&m,std::size_t(n))||!m)return nullptr;return m;},
  [&](void* m,std::uint64_t n){return v05::memset_async(m,0,std::size_t(n),stream)==0;},
  [&](void* m){if(v05::free_fn)v05::free_fn(m);});
}
inline int v05_launch(int k,const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 nrvit1dfull::Launch L{};bool transformed=false;
 {
  std::lock_guard<std::mutex> lock(v05::mutex);
  const bool repack=k==0&&args&&args[0];
  const std::int32_t dir=repack?nrvit1dfull::i32(args[0],0x1c):-1;
  nrvit1dfull::Chain* c=nullptr;
  for(auto& x:v05::chains)if(x.active&&x.stream==stream){c=&x;break;}
  if(c&&dir==1)c->active=false,c=nullptr;
  if(!c&&dir==1&&nrv05::ligado.load(std::memory_order_relaxed)){
   nrvit1dfull::Geometry geo{};
   if(nrvit1dfull::plan(args[0],v05::max_n,geo)){
    for(auto& x:v05::chains)if(!x.active){c=&x;break;}
    if(c){
     c->active=true;c->geo=geo;c->stream=stream;++v05::jobs;
    }else log("V05 sem cadeia livre (mais de 4 streams): grade de antes neste job");
   }
  }
  if(c){
   if(!nrvit1dfull::transform(static_cast<nrvit1dfull::Kind>(k),args[0],{g.x,g.y,g.z},c->geo,[&](void* p){return v05_shadow(p,c->geo,stream);},L)){
    c->active=false;char line[160]{};NRLOGF(line,sizeof line,"V05 FATAL lancamento fora da grade kind=%d grid=%u,%u,%u (job envenenado)",k,g.x,g.y,g.z);log(line);
    return mh_fail(1);
   }
   transformed=true;
   if(dir==0){
    c->active=false;
    const unsigned key=(c->geo.a2<<16)|c->geo.b2;bool seen=false;
    for(unsigned i=0;i<v05::seen_count;++i)seen=seen||v05::seen[i]==key;
    if(!seen&&v05::seen_count<8){
     v05::seen[v05::seen_count++]=key;
     char line[240]{};NRLOGF(line,sizeof line,"V05 grade nivel 6 %ux%u (N %u) -> %ux%u real %u N %u stream=%p jobs=%llu sombras=%u (%.1f MB, reaproveitadas %llu)",c->geo.b,c->geo.a,c->geo.n,c->geo.b2,c->geo.a2,c->geo.real2,c->geo.n2,stream,v05::jobs,v05::shadows.count,double(v05::shadows.total())/1048576.0,v05::shadows.evictions);log(line);
    }
   }
  }
 }
 if(!transformed){if(r1d::on&&k==0)r1d::skip=false;return launch_1d_next(k,f,g,b,args,shared,stream);}
 void* kargs[]{L.args};
 if(r1d::on&&r1d::so_cena&&!nrreusostate::cena){if(k==0&&nrvit1dfull::i32(L.args,0x1c)==1)++r1d::extras;r1d::skip=false;}
 else if(r1d::on){
  const std::int32_t dir1=k==0?nrvit1dfull::i32(L.args,0x1c):-1;
  if(k==0&&dir1==1){
   ++r1d::job;
   const unsigned nn=unsigned(nrvit1dfull::i32(L.args,0x18));
   const unsigned rf=nrreusostate::refresco.load(std::memory_order_relaxed);
   if(rf!=r1d::refresco_visto){r1d::refresco_visto=rf;r1d::valid=false;}
   bool forcar=false;
   if(r1d::mov::ready){r1d::mov::colher();forcar=nrr1dmov::cheio(r1d::mov::est,r1d::mov::nova?r1d::mov::ult:nrr1dmov::Medida{},r1d::mov::cfg);r1d::mov::nova=false;}
   r1d::skip=r1d::valid&&nn==r1d::n&&stream==r1d::stream&&(r1d::job&1ull)==0;
   if(r1d::skip&&forcar){r1d::skip=false;++r1d::mov::forcados;}
  }else if(k!=0&&r1d::skip)return 0;
  if(k==0&&dir1==0){
   void* tok{};std::memcpy(&tok,L.args+0,8);const unsigned nn=unsigned(nrvit1dfull::i32(L.args,0x18));
   std::size_t bytes=std::size_t(nn)*1024u;
   if(v05::range&&tok){void* base{};std::size_t size{};if(!v05::range(&base,&size,tok)&&base){const std::size_t room=size-std::size_t(static_cast<char*>(tok)-static_cast<char*>(base));if(room<bytes)bytes=room;}}
   if(r1d::skip&&tok&&r1d::cache&&bytes<=r1d::cap){
    if(r1d::memcpy_async(tok,r1d::cache,bytes,3,stream)){r1d::valid=false;log("REUSO1D copia cache->tokens falhou");}
    ++r1d::reusos;
   }
   const int e=launch_1d_next(k,f,{L.grid.x,L.grid.y,L.grid.z},b,kargs,shared,stream);
   if(!r1d::skip&&!e&&tok){

    if(r1d::cap<bytes){void* m{};if(r1d::cache)aposentados::guardar(r1d::cache,v05::free_fn);r1d::cache=nullptr;r1d::cap=0;
     if(!v05::malloc_fn(&m,bytes)&&m){r1d::cache=m;r1d::cap=bytes;}}
    r1d::valid=r1d::cache&&!r1d::memcpy_async(r1d::cache,tok,bytes,3,stream);r1d::n=nn;r1d::stream=stream;++r1d::cheios;
   }
   r1d::skip=false;
   if(((r1d::reusos+r1d::cheios)&255u)==1u){char line[240]{};NRLOGF(line,sizeof line,"REUSO1D jobs=%llu reusos=%llu cheios=%llu extras=%llu N=%u forcados_mov=%llu medidas=%llu ult_media=%.2f ult_frac=%.1f bytes=%llu",r1d::job,r1d::reusos,r1d::cheios,r1d::extras,nn,r1d::mov::forcados,r1d::mov::medidas,double(r1d::mov::ult.media),double(r1d::mov::ult.frac),(unsigned long long)bytes);log(line);}
   return e;
  }
 }
 return launch_1d_next(k,f,{L.grid.x,L.grid.y,L.grid.z},b,kargs,shared,stream);
}

inline bool vit_pode_manter(const void* f,void** args,void* stream) noexcept {
 if(!vit::manter_a||!args||!args[0])return false;
 unsigned bytes=0;unsigned char a[72]{};
 if(cauda::requested&&f==cauda::token)bytes=24;
 else if(decup::requested&&f==decup::token)bytes=40;
 else {static constexpr unsigned tam[5]{32,32,72,40,40};const int k=c1d43::kind(f);if(k>=0&&k<5)bytes=tam[k];}
 if(!bytes)return false;
 std::memcpy(a,args[0],bytes);
 if(bytes==40&&f==decup::token){std::memset(a+0x08,0,8);std::memset(a+0x10,0,8);}
 std::lock_guard<std::mutex> lock(vit::mutex);
 if(!vit::tracker.count||vit::tracker.stream!=stream||nrvitpolicy::refs_tracked(a,bytes,vit::tracker))return false;
 if(!vit::manter_logged.exchange(true,std::memory_order_relaxed)){char line[160]{};NRLOGF(line,sizeof line,"VIT MANTER A: %u tensor(es) seguem em frag ate o decup (NeuralRegVitManterA=1) stream=%p",vit::tracker.count,stream);log(line);}
 return true;
}
inline int launch_untimed(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {

 if(const int failed=dispatch.failure.load(std::memory_order_acquire))return failed;
 const bool sw32_token=f==dispatch.token||f==dispatch.alternate_token;

 if(dispatch.chain&&!sw32_token){if(const int e=sw32_flush())return e;}
 const int vit_kind=vit::requested?vit::kind(f):-1;

 if(vit::requested&&vit_kind<0&&!vit_pode_manter(f,args,stream)){if(const int e=vit_flush())return e;}
 if(nrcapture::requested){
  if(mh::chain_enabled){if(const int e=mh_flush_pending())return e;}
  int captured_result=0;
  if(nrcapture::maybe(f,{g.x,g.y,g.z},{b.x,b.y,b.z},args,shared,stream,&capture_forward,captured_result))return captured_result;
 }
 if(mh::requested){
  for(int w=0;w<3;++w)if(f==mh::tokens[w])return mh_launch(w,f,g,b,args,shared,stream);

  if(mh::chain_enabled){if(const int e=mh_flush_pending())return e;}
  if(v05::installed){const int kv=c1d43::kind(f);if(kv>=0&&kv<5)return v05_launch(kv,f,g,b,args,shared,stream);}
  if(c1d43::requested){const int k43=c1d43::kind(f);if(k43>=0)return launch_1d43(k43,f,g,b,args,shared,stream);}
  if(c1d::requested&&f==c1d::token)return launch_1d(f,g,b,args,shared,stream);
  if(vit_kind>=0)return vit_launch(vit_kind,f,g,b,args,shared,stream);
 }
 else if(v05::installed){const int kv=c1d43::kind(f);if(kv>=0&&kv<5)return v05_launch(kv,f,g,b,args,shared,stream);}

 if(decup::requested&&f==decup::token)return launch_decup(f,g,b,args,shared,stream);
 if(cauda::requested&&f==cauda::token)return launch_final_head(f,g,b,args,shared,stream);
 if(requested&&(f==dispatch.token||f==dispatch.alternate_token)){
  std::call_once(initialization,initialize);
  int current=-1;
  if(dispatch.ready.load(std::memory_order_acquire)&&(!get_device||get_device(&current)||current!=device)){

   return fallback(f,g,b,args,shared,stream);
  }
 }
 const bool ready=dispatch.ready.load(std::memory_order_acquire);
 if(ready&&(f==dispatch.token||f==dispatch.alternate_token))sw32x_sincronizar();
 unsigned mode{};int index=-1;
 if(ready&&(f==dispatch.token||f==dispatch.alternate_token)&&b.x==256&&b.y==1&&b.z==1&&!shared&&args&&args[0]){
  std::memcpy(&mode,static_cast<const char*>(args[0])+0x28,sizeof(mode));index=nrregpolicy::mode_index(mode);
  nrregpolicy::LayoutPlan plan{};
  if(dispatch.layout&&!nrregpolicy::layout_plan(mode,args[0],plan))index=-1;

  if(index>=0&&!nrregpolicy::admitted(mode,args[0])){
   int h{},w{};std::memcpy(&h,static_cast<const char*>(args[0])+0x18,4);std::memcpy(&w,static_cast<const char*>(args[0])+0x1c,4);
   const auto slot=static_cast<unsigned>(index)+(f==dispatch.alternate_token?8u:0u);
   if(potencia2(swin32_refused_count[slot])){
    char line[200]{};NRLOGF(line,sizeof line,"ORIGINAL old_token=%s mode=%u shape=%dx%d (forma nao provada) vezes=%llu",f==dispatch.token?"68088_true":"68090_false",mode,h,w,swin32_refused_count[slot].load(std::memory_order_relaxed));log(line);
   }
   index=-1;
  }
 }

 if(!sw32_token&&!sw32_pendente.load(std::memory_order_acquire)){
  if(const int failed=dispatch.failure.load(std::memory_order_acquire))return failed;
  return fallback(f,g,b,args,shared,stream);
 }
 int result{};bool chained{};
 {
  std::lock_guard<std::mutex> lock(sw32_mutex);
  const bool had=dispatch.pending;const void* held=dispatch.held.ptr;const unsigned flushes=dispatch.flushes;
  chained=had&&index>=0&&args&&args[0]&&*static_cast<void* const*>(args[0])==held;
  result=dispatch.launch(f,g,b,args,shared,stream);
  sw32_pendente.store(dispatch.pending,std::memory_order_release);
  if(nrtimeline::requested&&dispatch.flushes!=flushes)nrtimeline::flushed=true;
 }
 if(!result&&chained&&!sw32_chain_logged.exchange(true,std::memory_order_relaxed)){
  char line[128]{};NRLOGF(line,sizeof line,"CHAIN swin32 mode=%u entrada pendente reaproveitada (sem conversao de entrada)",mode);log(line);
 }
 if(!result&&index>=0&&(nrtimeline::requested||NR_R6_DENTRO))nrtimeline::replaced=true;
 if(!result&&index>=0){
  const auto slot=static_cast<unsigned>(index)+(f==dispatch.alternate_token?8u:0u);
  if(!dispatch_logged[slot].exchange(true,std::memory_order_relaxed)){
   char line[256]{};NRLOGF(line,sizeof line,"DISPATCH old_token=%s mode=%u grid=%u,%u,%u block=32,1,1 stream=%p result=0",f==dispatch.token?"68088_true":"68090_false",mode,g.x,g.y,g.z,stream);log(line);
  }
 }
 if(ready&&!dispatch.ready.load(std::memory_order_acquire))log("DISABLED reg launch failed; error propagated without duplicate submission");
 return result;
}

inline int launch_timed(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 auto& t=nrtimeline::recorder;
 std::string name=t.name(f);
 t.observe(name);
 const bool mh_token=f==mh::tokens[0]||f==mh::tokens[1]||f==mh::tokens[2];
 const bool swin=f==dispatch.token||f==dispatch.alternate_token||mh_token;
 if(swin&&args&&args[0]){unsigned mode{};std::memcpy(&mode,static_cast<const char*>(args[0])+0x28,sizeof(mode));name+="#m"+std::to_string(mode);}
 nrtimeline::replaced=false;nrtimeline::flushed=false;
 const auto m=t.start(stream);
 nrtimeline::inside=true;
 const int result=launch_untimed(f,g,b,args,shared,stream);
 nrtimeline::inside=false;
 if(nrtimeline::replaced)name+="->novo";
 if(nrtimeline::flushed)name+="+flush";
 t.stop(m,std::move(name),stream);
 return result;
}

inline int launch_inner(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 if(nrtimeline::requested)return launch_timed(f,g,b,args,shared,stream);
 return launch_untimed(f,g,b,args,shared,stream);
}
inline int launch_parametros(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {

 if(args&&args[0]){
  if(nrpadapara::modo){const char* img=installed_image.load();if(img&&f==img+0x62228){std::lock_guard<std::mutex> lock(sw32_mutex);nrpadapara::no_import(args[0]);}}
  if(f==dispatch.token||f==dispatch.alternate_token)nrsementefixa::aplicar(args[0]);
  else if(nrexpofresca::ligado()){const char* img=installed_image.load();
   if(img&&f==img+0x62228)nrexpofresca::no_import(args[0]);else if(img&&f==img+0x62238)nrexpofresca::no_reproject(args[0]);}
 }

 return launch_inner(f,g,b,args,shared,stream);
}

inline void r1d_medir(const void* params,void* stream) noexcept {
 namespace M=r1d::mov;
 std::call_once(M::init,[]{
  const auto hip=GetModuleHandleW(L"amdhip64_7.dll");if(!hip){log("REUSO1D MOV FALLBACK amdhip64_7.dll ausente");return;}
  auto load=nrhip::api().module_load_data;
  auto function=nrhip::api().module_get_function;
  auto host_malloc=reinterpret_cast<int(*)(void**,std::size_t,unsigned)>(nrhip::api().host_malloc);
  auto event_create=reinterpret_cast<int(*)(void**,unsigned)>(nrhip::api().event_create_with_flags);
  M::mlaunch=reinterpret_cast<M::ModuleLaunch>(nrhip::api().module_launch_kernel);
  M::memset_async=reinterpret_cast<decltype(M::memset_async)>(nrhip::api().memset_async);
  M::event_record=reinterpret_cast<decltype(M::event_record)>(nrhip::api().event_record);
  M::event_query=reinterpret_cast<decltype(M::event_query)>(nrhip::api().event_query);
  M::malloc_=reinterpret_cast<decltype(M::malloc_)>(nrhip::api().malloc_);
  bool ok=load&&function&&host_malloc&&event_create&&M::mlaunch&&M::memset_async&&M::event_record&&M::event_query&&M::malloc_&&r1d::memcpy_async;
  ok=ok&&!load(&M::module,M::image.data())&&M::module&&!function(&M::k,M::module,"rp_celulas")&&M::k;
  void* h{};ok=ok&&!host_malloc(&h,64,0)&&h;if(ok){M::stats_host=static_cast<unsigned*>(h);std::memset(h,0,64);}
  ok=ok&&!M::malloc_(&M::stats_dev,64)&&M::stats_dev&&!event_create(&M::ev,2u)&&M::ev;
  M::ready=ok;log(ok?"REUSO1D MOV READY (rp_celulas por job, celulas de 16 px)":"REUSO1D MOV FALLBACK modulo/API recusado: reuso 1D sem corte por movimento");
 });
 if(!M::ready)return;
 M::colher();
 if(M::pend){++M::em_voo;return;}
 M::Mv m{};nrreproj::copiar(params,m);
 m.dep=nullptr;m.dpitch=m.dfmt=m.dw=m.dh=m.dinv=0;
 const int bpp=m.fmt==1||m.fmt==2?8:4;
 const bool ok=m.mv&&m.fmt>=0&&m.fmt<=2&&m.mvw>0&&m.mvh>0&&m.mvw<=16384&&m.mvh<=16384&&m.pitch>=m.mvw*bpp&&m.netw>0&&m.neth>0&&m.netw<=16384&&m.neth<=16384&&std::isfinite(m.sx)&&std::isfinite(m.sy);
 if(!ok){++M::sem_mv;return;}
 int f=16,hh=(m.neth+15)/16,hw=(m.netw+15)/16,modo=0;float lim=M::cfg.limiar_px,dv=2.f;
 const std::size_t need=std::size_t(hh)*std::size_t(hw)*8u;

 if(M::d_cap<need){void* nd{};if(M::malloc_(&nd,need)||!nd)return;aposentados::guardar(M::d,nrhip::api().free_);M::d=nd;M::d_cap=need;}
 void* dd=M::d;void* st=M::stats_dev;void* a[]{&m,&hh,&hw,&f,&modo,&lim,&dv,&dd,&st};
 if(M::memset_async(st,0,32,stream)||M::mlaunch(M::k,unsigned(hw+63)/64u,unsigned(hh),1,64,1,1,0,stream,a,nullptr))return;
 if(r1d::memcpy_async(M::stats_host,st,32,2,stream)||M::event_record(M::ev,stream))return;
 M::pend=true;
}
inline int launch_r6(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream);
inline int launch_corpo(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream);
inline int launch_handshake(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream){return launch_corpo(f,g,b,args,shared,stream);}
inline int launch(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 if(rjsync::ligado.load(std::memory_order_relaxed)){
  const char* img=installed_image.load();
  if(img&&(f==img+0x62260||f==img+0x62268||f==img+0x62270)){
   int e{};{rjsync::Dono l;l.travar();e=launch_handshake(f,g,b,args,shared,stream);}
   if(f==img+0x62270)rjsync::flag_set_n.fetch_add(1,std::memory_order_release);
   return e;
  }
 }
 return launch_corpo(f,g,b,args,shared,stream);
}
inline int launch_corpo(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {

 if(nrcena::quer_mv.load(std::memory_order_relaxed)&&nrreusostate::cena&&args&&args[0]&&f==installed_image.load()+0x62238)nrcena::guardar(args[0]);
 const bool medir=r1d::mov::on&&args&&args[0]&&f==installed_image.load()+0x62238;
 const int e=launch_parametros(f,g,b,args,shared,stream);
 if(medir&&!e)r1d_medir(args[0],stream);
 return e;
}
inline int launch_r6(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream){return launch(f,g,b,args,shared,stream);}

namespace orcmalloc {
inline constexpr unsigned slot_malloc=0x86be0u,slot_free=0x86ba8u;
using Malloc=int(*)(void**,std::size_t);using Free=int(*)(void*);
inline Malloc native{};inline Free native_free{};
inline std::atomic<bool> pool_on{false};
inline int hook(void** p,std::size_t n) noexcept {
 if(pool_on.load(std::memory_order_acquire)&&nrorc::rede_rosto_tl>0&&p&&n){
  auto& P=nrorc_vram::pool_rede();
  {std::lock_guard<std::mutex> l(P.m);if(void* b=P.p.servir(n)){*p=b;return 0;}}
  const int r=native(p,n);
  if(!r&&*p){const bool tarde=nrorc::estado().guarda.selado.load(std::memory_order_acquire);
   {std::lock_guard<std::mutex> l(P.m);P.p.registrar(*p,n);if(tarde)++P.reais_pos_selo;}
   nrorc_vram::registrar(nrorc::Api::HipMallocProxy,n,"proxy_pool",nrorc::Origem::RedeRosto);}
  return r;}
 const int r=native(p,n);if(!r&&n){nrorc_vram::registrar(nrorc::Api::HipMallocProxy,n,"proxy");nrorc_vram::vivo_criar(*p,n,nrorc::origem_tl);}return r;}

inline int hook_free(void* p) noexcept {
 if(p&&pool_on.load(std::memory_order_acquire)){auto& P=nrorc_vram::pool_rede();std::lock_guard<std::mutex> l(P.m);if(P.p.devolver(p))return 0;}
 nrorc_vram::vivo_soltar(p);
 return native_free(p);}
inline bool trocar(void** slot,void* novo) noexcept {
 DWORD old{},unused{};if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old))return false;
 InterlockedExchangePointer(reinterpret_cast<void* volatile*>(slot),novo);VirtualProtect(slot,sizeof(void*),old,&unused);return true;}
}
inline void orc_contar(char* image) noexcept {
 static std::atomic<bool> feito{false};if(!image||nrorc::modo()==0||feito.exchange(true))return;
 if(v05::malloc_fn)v05::malloc_fn=&nrorc_vram::hip_malloc_contado;
 if(v05::free_fn)v05::free_fn=&nrorc_vram::hip_free_contado;
 char v[4]{};
 if(GetEnvironmentVariableA("NR_MEMORY_DIAGNOSTICS",v,sizeof v)==1&&v[0]=='1'){log("ORCAMENTO hipMalloc do proxy nao contado aqui (e sem pool da rede do rosto): NR_MEMORY_DIAGNOSTICS=1 (o nr_memory.log conta)");return;}
 const auto hip=GetModuleHandleW(L"amdhip64_7.dll");const auto expected=hip?GetProcAddress(hip,"hipMalloc"):nullptr;
 const auto expected_free=hip?GetProcAddress(hip,"hipFree"):nullptr;
 const auto slot=reinterpret_cast<void**>(image+orcmalloc::slot_malloc);const auto slot_f=reinterpret_cast<void**>(image+orcmalloc::slot_free);
 if(!expected||*slot!=reinterpret_cast<void*>(expected)){log("ORCAMENTO hipMalloc do proxy nao contado: slot do IAT diferente do export");return;}

 const bool quer_pool=nrorc::modo()==1;
 const bool free_ok=expected_free&&*slot_f==reinterpret_cast<void*>(expected_free);
 if(quer_pool&&!free_ok)log("ORCAMENTO pool da rede do rosto recusado: slot do hipFree (0x86ba8) diferente do export; so a contagem");
 orcmalloc::native=reinterpret_cast<orcmalloc::Malloc>(expected);

 bool free_ligado=false;
 if(free_ok){orcmalloc::native_free=reinterpret_cast<orcmalloc::Free>(expected_free);
  if(!orcmalloc::trocar(slot_f,reinterpret_cast<void*>(&orcmalloc::hook_free))){log(quer_pool?"ORCAMENTO pool da rede do rosto recusado: VirtualProtect do hipFree":"ORCAMENTO hipFree do proxy nao descontado: VirtualProtect");}
  else{free_ligado=true;
   if(quer_pool){orcmalloc::pool_on.store(true,std::memory_order_release);{auto& P=nrorc_vram::pool_rede();std::lock_guard<std::mutex> l(P.m);P.ligado=true;}

    if(v05::range&&v05::range!=&nrorc_vram::range_pool){nrorc_vram::range_real=v05::range;v05::range=&nrorc_vram::range_pool;}}}}
 if(!free_ligado&&!quer_pool)log("ORCAMENTO hipFree do proxy nao descontado (slot 0x86ba8 diferente do export): rosto_mb soma so os hipMalloc");
 if(!orcmalloc::trocar(slot,reinterpret_cast<void*>(&orcmalloc::hook))){orcmalloc::pool_on.store(false,std::memory_order_release);log("ORCAMENTO hipMalloc do proxy nao contado: VirtualProtect");return;}
 log(orcmalloc::pool_on.load()?"ORCAMENTO hipMalloc do proxy contado e pool da rede do rosto ligado (hipMalloc 0x86be0 + hipFree 0x86ba8 conferidos com o export)":
     "ORCAMENTO hipMalloc do proxy contado (NeuralRostoJogadorOrcamento)");
}
inline bool install(char* image) noexcept {
 try {
  std::lock_guard<std::mutex> lock(install_mutex);
  if(installed_image.load())return installed_image.load()==image;

  static_assert(std::string_view(nrliveimage::file_sha256)=="fc5004298a735e207cfa80a8c66ad4a9bd8b5d469ec149a2633ca73d45a3ccce");

  const auto proxy_path=nrpasta::modulo_de(image);
  if(!image||nrpasta::helper().empty()||proxy_path.empty())return false;
  folder=nrpasta::helper();
  const auto ini=nrinipath::resolve(image,reinterpret_cast<const void*>(&install));
  if(!ini.empty())nrcapture::configure(ini,folder);
  if(!ini.empty())nrtimeline::configure(ini,folder);
  requested=!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32",0,ini.c_str())==1;

  {const bool t=ini.empty()||NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegFormasTodas",1,ini.c_str())!=0;nrregpolicy::todas_formas=t;nrvitpolicy::todas_formas=t;
   nrvitpolicy::views_todas=!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegVitViews",0,ini.c_str())==1;
   if(nrvitpolicy::views_todas)log("VIT VIEWS: conv com pooled em qualquer H/2 x W/2 (NeuralRegVitViews=1)");
   nrvitpolicy::views_exato=!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegVitViewsExato",0,ini.c_str())==1;
   if(nrvitpolicy::views_exato)log("VIT VIEWS EXATO pedido: conv<1,4> v0.4.1 no pooled parcial (NeuralRegVitViewsExato=1)");
   vit::manter_a=!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegVitManterA",0,ini.c_str())==1;
   if(vit::manter_a)log("VIT MANTER A pedido: vit512a em frag do fim do encoder ao decup (NeuralRegVitManterA=1)");
   log(t?"FORMAS todas (NeuralRegFormasTodas=1)":"FORMAS so as provadas (NeuralRegFormasTodas=0)");}
  layout_requested=ini.empty()||NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32Layout",1,ini.c_str())!=0;
  wchar_t flag[2]{};
  mh::requested=!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwinMh",0,ini.c_str())==1;
  c1d::requested=mh::requested&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralReg1d",1,ini.c_str())==1;

  {const int v=!ini.empty()?int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralReg1d43",1,ini.c_str())):0;c1d43::requested=mh::requested&&(v==1||v==2);c1d43::fast=v!=2;}
  c1d43::conv_split=NRCFG_INT(L"DlssNrOnAmd",L"NeuralReg1d43ConvSplit",1,ini.c_str())!=0;
  c1d43::split_teto=static_cast<unsigned>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralReg1d43SplitTeto",64,ini.c_str()));
  c1d43::qkv_split_max=static_cast<unsigned>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralReg1d43QkvSplitMax",0,ini.c_str()));

  wchar_t stages[2]{};
  const bool stages_env=GetEnvironmentVariableW(L"DLSSNR_STAGES",stages,2)!=0;

  const bool trace_k=nrkerneltrace::trace_ligado(long(NRCFG_INT(L"DlssNrOnAmd",L"NeuralKernelTrace",0,ini.c_str())),NR_LOG_ATIVO!=0);
  const bool trace_c=nrkerneltrace::contrato_ligado(long(NRCFG_INT(L"DlssNrOnAmd",L"NeuralContractTrace",0,ini.c_str())),NR_LOG_ATIVO!=0,NR_SEM_GRAVACAO!=0);
  c1d43::requested=c1d43::requested&&!nrcapture::requested&&!trace_k&&!trace_c&&!stages_env;
  vit::requested=mh::requested&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegVit",1,ini.c_str())==1&&!nrcapture::requested&&
   !trace_k&&!trace_c&&
   !stages_env;

  mh::chain_enabled=nrmhpolicy::chain_allowed(mh::requested,NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwinMhChain",1,ini.c_str())==1,
   trace_k,trace_c,stages_env);

  dispatch.chain=nrmhpolicy::chain_allowed(requested&&layout_requested,NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32Chain",1,ini.c_str())==1,
   trace_k,trace_c,stages_env)&&!nrcapture::requested;
  if(GetEnvironmentVariableW(L"DLSSNR_NO_REG",flag,2)||GetEnvironmentVariableW(L"DLSSNR_SLOW_PREPOST",flag,2))requested=mh::requested=mh::chain_enabled=vit::requested=dispatch.chain=c1d43::requested=false;

  if(!nrliveimage::disk_file_admitted(proxy_path.c_str())){log("REJECTED proxy disk digest");return false;}
  if(requested&&(!ler_modulo(folder/L"nr_reg_swin32.hsaco",nrregcontract::module_bytes,nrregcontract::sha256,module_image))){requested=false;module_image.clear();log("FALLBACK module absent or digest mismatch");}
  v041_requested=requested&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32V041",1,ini.c_str())==1;
  v041_mask=static_cast<unsigned>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32V041Mask",255,ini.c_str()))&0xffu;
  if(v041_requested&&(!ler_modulo(folder/L"nr_reg_swin_mh.hsaco",nrmhcontract::module_bytes,nrmhcontract::sha256,v041_image))){v041_requested=false;v041_image.clear();log("SW32 V041 FALLBACK nr_reg_swin_mh.hsaco absent or digest mismatch");}
  fast::requested=nrmhcontract::fast_available&&(requested||mh::requested)&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegFast",1,ini.c_str())==1;
  fast::mask=static_cast<unsigned>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegFastMask",7,ini.c_str()))&7u;
  fast::sw32_mask=static_cast<unsigned>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegFastSw32Mask",255,ini.c_str()))&0xffu;
  const bool want43=fast::requested&&nrmhcontract::fast43_available&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegFastVersao",43,ini.c_str())==43;
  bool have43=false;
  if(want43){

   static_assert(nr1080::contract::fast43_mhpeep_bytes==nrmhcontract::fast43_module_bytes,"mh peep muda o tamanho do fast43");

   bool ovfl43=false;
   if(!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegMhOvfl",0,ini.c_str())==1){
    ovfl43=ler_modulo(folder/L"nr_reg_fast43_ovfl.hsaco",nrmhcontract::fast43_module_bytes,nr1080::contract::fast43_mhovfl_sha256,fast::image43);
    if(!ovfl43){fast::image43.clear();log("FAST43 OVFL FALLBACK nr_reg_fast43_ovfl.hsaco absent or digest mismatch; fica o nr_reg_fast43.hsaco");}
   }
   const bool read43=ovfl43||read_exact(folder/L"nr_reg_fast43.hsaco",nrmhcontract::fast43_module_bytes,fast::image43);
   const bool peep=!ovfl43&&read43&&nrliveimage::detail::sha256_matches(fast::image43.data(),fast::image43.size(),nr1080::contract::fast43_mhpeep_sha256);
   have43=ovfl43||peep||(read43&&nrliveimage::detail::sha256_matches(fast::image43.data(),fast::image43.size(),nrmhcontract::fast43_sha256));
   if(!have43){fast::image43.clear();log("FAST43 FALLBACK nr_reg_fast43.hsaco absent or digest mismatch; v0.4.2");}
   else log(ovfl43?"FAST43 modulo 2842B012 (mh peep + FP16_OVFL, NeuralRegMhOvfl=1)":peep?"FAST43 modulo B276C6EF (mh peep, bit a bit)":"FAST43 modulo 543CFEF7 (Daniel, sem o mh peep)");

   const int borda_chave=ini.empty()?0:int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralR55MhBorda",0,ini.c_str()));
   const int borda_mascara=ini.empty()?7:int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralR55MhBordaMask",7,ini.c_str()));
   for(unsigned w=0;w<3;++w){
    mh::r55_image[w].clear();mh::r55_borda[w]=false;const auto* r=nr1080::r55_mh(w);
    if(ini.empty()||NRCFG_INT(L"DlssNrOnAmd",r->key,0,ini.c_str())!=1)continue;
    char line[160]{};

    if(have43&&nr1080::r55_borda_pedida(borda_chave,borda_mascara,w)){
     const auto* rb=nr1080::r55_mh_borda(w);
     if(rb->bytes&&ler_modulo(folder/rb->file,rb->bytes,rb->sha256,mh::r55_image[w])){
      mh::r55_borda[w]=true;NRLOGF(line,sizeof line,"R55 %s pedido (%ls: janelas parciais so com as telhas validas)",rb->tag,rb->file);log(line);continue;}
     mh::r55_image[w].clear();NRLOGF(line,sizeof line,"R55 %s FALLBACK %ls absent or digest mismatch; fica o %ls",rb->tag,rb->file,r->file);log(line);
    }
    if(!have43||!r->bytes||!ler_modulo(folder/r->file,r->bytes,r->sha256,mh::r55_image[w])){
     mh::r55_image[w].clear();NRLOGF(line,sizeof line,"R55 %s FALLBACK %ls absent or digest mismatch; fica o Fast",r->tag,r->file);}
    else NRLOGF(line,sizeof line,"R55 %s pedido (%ls)",r->tag,r->file);
    log(line);
   }
  }

  fast::image.clear();fast::pasta=folder;
  if(fast::requested&&!have43&&(!ler_modulo(folder/L"nr_reg_fast.hsaco",nrmhcontract::fast_module_bytes,nrmhcontract::fast_sha256,fast::image))){
   fast::image.clear();
   fast::requested=false;log("FAST FALLBACK nr_reg_fast.hsaco absent or digest mismatch; tudo no v0.4.1");
  }
  if(fast::requested){char line[120]{};NRLOGF(line,sizeof line,"FAST %s pedido mask=%u sw32_mask=%u",have43?"v0.4.3":"v0.4.2",fast::mask,fast::sw32_mask);log(line);}
  {
   int key=!ini.empty()?int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32X",0,ini.c_str())):0;

   nr1080::r55_sw32=false;
   if(!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralR55Sw32",0,ini.c_str())==1){
    const auto* r=nr1080::sw32x_r55();std::vector<unsigned char> t;
    if(!ler_modulo(folder/r->file,r->bytes,r->sha256,t))log("R55 SW32 FALLBACK nr_r55_sw32.hsaco absent or digest mismatch; swin32 fica como antes");
    else{nr1080::r55_sw32=true;if(key==1)key=2;char line[120]{};NRLOGF(line,sizeof line,"R55 SW32 pedido: variante E = %s; NeuralRegSwin32X efetivo=%d",r->tag,key);log(line);}}

   nr1080::sw32x_ovfl=!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32Ovfl",0,ini.c_str())==1;
   if(nr1080::sw32x_ovfl){const auto* ao=nr1080::sw32x_ao();std::vector<unsigned char> t;
    if(!ler_modulo(folder/ao->file,ao->bytes,ao->sha256,t)){nr1080::sw32x_ovfl=false;log("SW32X OVFL FALLBACK nr_reg_swin32x_ao.hsaco absent or digest mismatch; a variante A fica sem FP16_OVFL");}
    else log("SW32X OVFL pedido: a variante A usa o modulo A + FP16_OVFL");}
   sw32x::variant=nr1080::sw32x(key);sw32x::image.clear();
   nrsw32x::escolha.store(key==1||key==2?key:0,std::memory_order_relaxed);

   for(int v=1;v<=2;++v){sw32x::images[v].clear();const auto* var=nr1080::sw32x(v);
    if(var&&requested&&have43&&(fast::mask&2u)&&ler_modulo(folder/var->file,var->bytes,var->sha256,sw32x::images[v])){
     char line[160]{};NRLOGF(line,sizeof line,"SW32X disponivel no menu: %s",var->tag);log(line);}
    else sw32x::images[v].clear();}
   if(sw32x::variant&&!(requested&&have43&&(fast::mask&2u))){sw32x::variant=nullptr;log("SW32X FALLBACK swin32 Fast v0.4.3 nao pedido/ausente; modulo novo nao usado");}
   else if(sw32x::variant&&(!ler_modulo(folder/sw32x::variant->file,sw32x::variant->bytes,sw32x::variant->sha256,sw32x::image))){
    char line[160]{};NRLOGF(line,sizeof line,"SW32X FALLBACK %ls absent or digest mismatch; swin32 fica no Fast v0.4.3",sw32x::variant->file);log(line);
    sw32x::variant=nullptr;sw32x::image.clear();
   }
   else if(sw32x::variant){char line[160]{};NRLOGF(line,sizeof line,"SW32X pedido NeuralRegSwin32X=%d %s (%ls)",key,sw32x::variant->tag,sw32x::variant->file);log(line);}
   else if(key)log("SW32X valor desconhecido em NeuralRegSwin32X; swin32 fica no Fast v0.4.3");
  }
  if(requested&&layout_requested&&(!ler_modulo(folder/L"nr_reg_layout.hsaco",nrreglayout::module_bytes,nrreglayout::sha256,layout_image))){requested=false;layout_image.clear();log("FALLBACK layout module absent or digest mismatch");}
  dispatch.chain=dispatch.chain&&requested;
  dispatch.park_pretin=dispatch.chain&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32PreTin",1,ini.c_str())==1;
  dispatch.drop_restore=dispatch.chain&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32DropRestore",1,ini.c_str())==1;
  dispatch.park_stage=dispatch.chain&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32Park",1,ini.c_str())==1;

  if(mh::requested&&!v041_image.empty())mh::image=v041_image;
  else if(mh::requested&&(!ler_modulo(folder/L"nr_reg_swin_mh.hsaco",nrmhcontract::module_bytes,nrmhcontract::sha256,mh::image))){mh::requested=false;mh::image.clear();log("MH FALLBACK module absent or digest mismatch");}
  if(mh::requested&&(!ler_modulo(folder/L"nr_mh_layout.hsaco",nrmhcontract::layout_bytes,nrmhcontract::layout_sha256,mh::layout_image))){mh::requested=false;mh::image.clear();mh::layout_image.clear();log("MH FALLBACK layout module absent or digest mismatch");}
  c1d43::requested=c1d43::requested&&mh::requested;
  if(c1d43::requested&&(!ler_modulo(folder/L"nr_reg_1d43.hsaco",nr1d43contract::module_bytes,nr1d43contract::sha256,c1d43::image))){c1d43::requested=false;c1d43::image.clear();log("1D43 FALLBACK nr_reg_1d43.hsaco absent or digest mismatch");}
  c1d::requested=c1d::requested&&mh::requested;
  if(c1d::requested&&(!ler_modulo(folder/L"nr_reg_v040.hsaco",nrmhcontract::v040_module_bytes,nrmhcontract::v040_sha256,c1d::image))){c1d::requested=false;c1d::image.clear();log("1D FALLBACK nr_reg_v040.hsaco absent or digest mismatch");}
  mh::chain_enabled&=mh::requested;
  mh::drop_restore=nrmhpolicy::drop_restore_allowed(mh::chain_enabled&&!nrcapture::requested,NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwinMhDropRestore",1,ini.c_str())==1);
  mh::chain.park_enabled=mh::chain_enabled&&!nrcapture::requested&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwinMhPark",1,ini.c_str())==1;
  mh::chain.decoder_enabled=NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwinMhChainDecoder",1,ini.c_str())==1;
  vit::requested=vit::requested&&mh::requested;
  if(vit::requested&&(!ler_modulo(folder/L"nr_vit_layout.hsaco",nrmhcontract::vit_layout_bytes,nrmhcontract::vit_layout_sha256,vit::layout_image))){vit::requested=false;vit::layout_image.clear();log("VIT FALLBACK layout module absent or digest mismatch");}
  vit::tracker.drop_final=vit::requested&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegVitDropFinal",1,ini.c_str())==1;

  nrpadapara::modo=static_cast<int>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralPerfilAparaPad",0,ini.c_str()));
  nrpadapara::margem=static_cast<int>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralPerfilAparaMargem",0,ini.c_str()));
  if(nrpadapara::modo<0||nrpadapara::modo>2)nrpadapara::modo=0;
  nrpadapara::modo_x=nrpadapara::modo&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralPerfilAparaPadX",0,ini.c_str())==1;
  if(nrpadapara::modo){nrpadapara::logger=log;char line[128]{};NRLOGF(line,sizeof line,"APARA PAD pedido modo=%d x=%d margem=%d",nrpadapara::modo,nrpadapara::modo_x,nrpadapara::margem);log(line);}

  {
   using namespace nr1080::contract;
   const auto key=[&](const wchar_t* k,int d){return int(NRCFG_INT(L"DlssNrOnAmd",k,d,ini.c_str()));};
   rw1d::requested=c1d43::requested&&c1d43::fast&&key(L"NeuralReg1dRw",1)!=0;
   rw1d::split=key(L"NeuralReg1dRwSplit",1)!=0;
   if(rw1d::requested&&(!ler_modulo(folder/L"nr_1d_rw.hsaco",rw1d_bytes,rw1d_sha256,rw1d::image,rw1d_sha256_alt))){rw1d::requested=false;rw1d::image.clear();log("1D RW FALLBACK nr_1d_rw.hsaco absent or digest mismatch");}
   r55d1::requested=c1d43::requested&&c1d43::fast&&key(L"NeuralR55D1",0)==1;
   if(r55d1::requested&&(!ler_modulo(folder/L"nr_r55_1d.hsaco",r55_1d_bytes,r55_1d_sha256,r55d1::image))){r55d1::requested=false;r55d1::image.clear();log("R55 1D FALLBACK nr_r55_1d.hsaco absent or digest mismatch; qkv do Daniel");}
   else if(r55d1::requested)log("R55 1D pedido: NeuralR55D1=1 (qkv_fast -> k1d_qkv_*)");
   vitx::requested=vit::requested&&key(L"NeuralRegVitx",1)!=0;
   vitx::mask=static_cast<unsigned>(key(L"NeuralRegVitxMask",7))&7u;
   if(vitx::requested){
    if(!ler_modulo(folder/L"nr_vitx_rel.hsaco",vitx_bytes,vitx_sha256,vitx::image,vitx_sha256_alt)){vitx::image.clear();log("VITX FALLBACK nr_vitx_rel.hsaco absent or digest mismatch (conv/ffwd do Daniel)");}

    if(ler_modulo(folder/L"nr_vitx_attn.hsaco",vitx_attn_nopulo_bytes,vitx_attn_nopulo_sha256,vitx::attn_image))log("VITX attn2 sem pulo (7CC6C197)");
    else if(ler_modulo(folder/L"nr_vitx_attn.hsaco",vitx_attn_bytes,vitx_attn_sha256,vitx::attn_image))log("VITX attn2 com pulo (61C41528)");
    else {vitx::attn_image.clear();log("VITX FALLBACK nr_vitx_attn.hsaco absent or digest mismatch (attn2 do Daniel)");}
    vitx::requested=!vitx::image.empty()||!vitx::attn_image.empty();
   }
   decup::requested=vit::requested&&have43&&key(L"NeuralRegDecup",1)!=0;
   cauda::requested=mh::requested&&key(L"NeuralRegFinalHead",1)!=0&&!nrcapture::requested;
   if(cauda::requested&&(!ler_modulo(folder/L"nr_cauda.hsaco",cauda_bytes,cauda_sha256,cauda::image,cauda_sha256_alt))){cauda::requested=false;cauda::image.clear();log("FINALHEAD FALLBACK nr_cauda.hsaco absent or digest mismatch");}
   char line[200]{};NRLOGF(line,sizeof line,"REDE1080 pedido: 1d_rw=%d (split=%d) vitx=%d (mask=%u) decup=%d final_head=%d",rw1d::requested?1:0,rw1d::split?1:0,vitx::requested?1:0,vitx::mask,decup::requested?1:0,cauda::requested?1:0);log(line);
  }
  const auto hip=GetModuleHandleW(L"amdhip64_7.dll");
  const auto expected=hip?GetProcAddress(hip,"hipLaunchKernel"):nullptr;
  const auto slot=reinterpret_cast<void**>(image+0x86bd8);
  if(!expected||*slot!=reinterpret_cast<void*>(expected)){log("REJECTED unknown launch hook");return false;}
  DWORD old{},unused{};if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old))return false;
  native_launch=reinterpret_cast<Launch>(expected);dispatch.original=&fallback;dispatch.owner_device=&sw32_owner_device;dispatch.token=image+0x68088;dispatch.alternate_token=image+0x68090;

  mh::tokens[0]=image+0x68098;mh::tokens[1]=image+0x680a0;mh::tokens[2]=image+0x680a8;
  c1d::token=image+0x621e0;

  c1d43::tokens[0]=image+0x62210;c1d43::tokens[1]=image+0x621d8;c1d43::tokens[2]=image+0x621e0;c1d43::tokens[3]=image+0x621e8;c1d43::tokens[4]=image+0x621f0;
  c1d43::foreign[0]=image+0x621b8;c1d43::foreign[1]=image+0x621c0;c1d43::foreign[2]=image+0x621c8;c1d43::foreign[3]=image+0x621d0;c1d43::foreign[4]=image+0x62218;
  decup::token=image+0x62218;cauda::token=image+0x62208;

  if(!ini.empty()){
   nrhistmov::modo=std::clamp<int>(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralHistMov",0,ini.c_str())),0,1);
   nrhistmov::log_on=NRCFG_INT(L"DlssNrOnAmd",L"NeuralHistMovLog",0,ini.c_str())!=0;

   float t{};const bool tem=nrinijogo::ler_float_em(ini.c_str(),L"NeuralHistMovMax",t,64)&&std::isfinite(t);
   nrhistmov::teto_auto=!tem;
   if(tem)nrhistmov::teto=std::clamp(t,1.f,nrhistmov::teto_max);
   if(nrhistmov::modo||nrhistmov::log_on){char line[200]{};
    if(tem)NRLOGF(line,sizeof line,"HISTMOV pedido: modo=%d teto=%.1f%s log=%d",nrhistmov::modo,double(nrhistmov::teto),t>nrhistmov::teto_max?" (NeuralHistMovMax acima de 16: preso em 16)":"",nrhistmov::log_on?1:0);
    else NRLOGF(line,sizeof line,"HISTMOV pedido: modo=%d teto=auto (NeuralHistMovMax ausente = a cadencia efetiva, presa em 16) log=%d",nrhistmov::modo,nrhistmov::log_on?1:0);
    log(line);}
  }

  {
   const int v=!ini.empty()?int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralV05Tokens",0,ini.c_str())):0;
   if(!nrv05::menu_escreveu.load())nrv05::ligado.store(v==1);
   v05::malloc_fn=nrorc::modo()>0?static_cast<int(*)(void**,std::size_t)>(&nrorc_vram::hip_malloc_contado):nrhip::api().malloc_;
   v05::memset_async=nrhip::api().memset_async;
   v05::range=nrhip::api().mem_get_address_range;
   v05::free_fn=nrorc::modo()>0?static_cast<int(*)(void*)>(&nrorc_vram::hip_free_contado):nrhip::api().free_;
   v05::installed=v05::malloc_fn&&v05::memset_async&&!nrcapture::requested;
   r1d::memcpy_async=reinterpret_cast<decltype(r1d::memcpy_async)>(nrhip::api().memcpy_async);
   r1d::on=v05::installed&&r1d::memcpy_async&&!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralReuso1D",0,ini.c_str())==1;
   if(r1d::on)log("REUSO1D ligado: saida do nivel global 1D reusada em 1 de cada 2 jobs (NeuralReuso1D=1; so com a grade v0.5)");
   r1d::so_cena=r1d::on&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralReuso1DSoCena",0,ini.c_str())==1;
   if(r1d::so_cena)log("REUSO1D so na cena (NeuralReuso1DSoCena=1): passos extras rodam o 1D inteiro, fora do cache");
   if(r1d::on){
    auto rd=[&](const wchar_t* key,float def){float v{};return nrinijogo::ler_float_em(ini.c_str(),key,v,64)?v:def;};
    r1d::mov::cfg.limiar_px=std::clamp(rd(L"NeuralReuso1DLimiarPx",0.f),0.f,1000.f);
    r1d::mov::cfg.fracao=std::clamp(rd(L"NeuralReuso1DFracao",10.f),0.f,100.f);
    r1d::mov::cfg.saida=std::clamp(rd(L"NeuralReuso1DSaida",0.5f),0.f,1.f);
    r1d::mov::cfg.calmos=static_cast<unsigned>(std::clamp<int>(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralReuso1DCalmos",3,ini.c_str())),1,64));
    if(r1d::mov::cfg.limiar_px>0.f){
     if(!ler_modulo(folder/L"nr_reuso_proj.hsaco",nrreusoproj::module_bytes,nrreusoproj::module_sha256,r1d::mov::image)){
      r1d::mov::image.clear();log("REUSO1D MOV FALLBACK nr_reuso_proj.hsaco ausente ou digest diferente: reuso 1D sem corte por movimento");}
     else{r1d::mov::on=true;char line[200]{};NRLOGF(line,sizeof line,"REUSO1D MOV pedido: limiar=%.2f px fracao=%.1f%% saida=%.2f calmos=%u",double(r1d::mov::cfg.limiar_px),double(r1d::mov::cfg.fracao),double(r1d::mov::cfg.saida),r1d::mov::cfg.calmos);log(line);}
    }
   }
   char line[160]{};NRLOGF(line,sizeof line,"V05 %s NeuralV05Tokens=%d (menu ao vivo; grade do nivel global da v0.5.0)",v05::installed?"INSTALLED":"OFF (captura ou API)",v);log(line);
  }
  vit::tokens[0]=image+0x621a0;vit::tokens[1]=image+0x621a8;vit::tokens[2]=image+0x621b0;vit::tokens[3]=image+0x62200;
  InterlockedExchangePointer(reinterpret_cast<void* volatile*>(slot),reinterpret_cast<void*>(&launch));
  VirtualProtect(slot,sizeof(void*),old,&unused);register_hook(reinterpret_cast<void*>(&launch));installed_image.store(image);
  log(requested?(dispatch.chain?(dispatch.park_pretin?(dispatch.drop_restore?(dispatch.park_stage?"INSTALLED awaiting first Swin32 launch chain=on pre_tin=reg drop_restore=on park=on":"INSTALLED awaiting first Swin32 launch chain=on pre_tin=reg drop_restore=on"):"INSTALLED awaiting first Swin32 launch chain=on pre_tin=reg"):"INSTALLED awaiting first Swin32 launch chain=on"):"INSTALLED awaiting first Swin32 launch chain=off"):"INSTALLED Swin32 original path (disabled or module rejected)");
  log(c1d::requested?"1D INSTALLED awaiting first contract2 launch":"1D INSTALLED original path (NeuralReg1d off or mh module rejected)");
  log(c1d43::requested?(c1d43::fast?"1D43 INSTALLED awaiting first k_repack (Fast)":"1D43 INSTALLED awaiting first k_repack (Reference)"):"1D43 INSTALLED off (NeuralReg1d43=0, capture/trace, or module rejected)");
  log(vit::requested?"VIT INSTALLED awaiting first vit512 launch":"VIT INSTALLED original path (NeuralRegVit off, capture/trace on, or module rejected)");
  log(mh::requested?(mh::chain_enabled?(mh::drop_restore?(mh::chain.park_enabled?"MH INSTALLED awaiting first Swin64/128/256 launch chain=on drop_restore=on park=on":"MH INSTALLED awaiting first Swin64/128/256 launch chain=on drop_restore=on"):"MH INSTALLED awaiting first Swin64/128/256 launch chain=on"):"MH INSTALLED awaiting first Swin64/128/256 launch chain=off"):"MH INSTALLED original path (NeuralRegSwinMh off or module rejected)");
  return true;
 }catch(...){log("REJECTED install exception");return false;}
}
}
