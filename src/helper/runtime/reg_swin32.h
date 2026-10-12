#pragma once
#include "nr_publico.h"
#include "reg_swin32_policy.h"
#include "native_ini_path.h"
#include "ini_jogo.h"
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4201)
#endif
#include <hip/hip_runtime_api.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "neural_tone.h"
#include "../source/implementation/live-transition-image/live_image.h"
#include "../../v033-port/kernels/reg_swin32_contract.h"
#include "../../v033-port/kernels/reg_layout_contract.h"
#include "reg_swin_mh_policy.h"
#include "kernel_capture.h"
#include "kernel_timeline.h"
#include "reg1d_policy.h"
#include "regvit_policy.h"
#include "sw32x_state.h"
#include "v05_state.h"
#include "vit1d_full.h"
#include "rede1080_policy.h"
#include "reuso_fundo.h"
#include "reuso_state.h"
#include "reuso_proj_contract.h"
#include "reuso1d_mov.h"
#include "exposicao_fresca.h"
#include "cena_mascara.h"
#include "hist_mov.h"
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
inline std::atomic<bool> chain_logged{false};
inline void* module_handle{};

inline std::vector<unsigned char> r55_image[3];
inline void* r55_module[3]{};
inline bool r55_borda[3]{};
}

namespace mhp {
inline bool requested=false;
inline std::vector<unsigned char> image;
inline void* module{};inline void* fn[3][3]{};
inline int modo=0;
inline unsigned mascara=4;
inline bool ready=false;
inline bool r55[3][8]{};
inline void* sync[3]{};inline unsigned epoch[3]{};

inline constexpr unsigned max_streams=4;
inline void* sync_s[3][max_streams]{};inline void* sync_stream[3][max_streams]{};inline bool sync_used[3][max_streams]{};inline unsigned epoch_s[3][max_streams]{};

inline unsigned grade_max=0;
inline int monitor=1;inline unsigned* host_err{};inline unsigned last_err[3][max_streams][3]{};
inline int(*memcpy_async)(void*,const void*,std::size_t,int,void*){};
inline unsigned long long monitor_checks=0,monitor_errors=0;
inline constexpr unsigned cap_items=65536;
struct Layer{unsigned char vp[176];unsigned gx,gy,mode,first;};
static_assert(sizeof(Layer)==192,"mhp Layer 192");
struct Args{void* sync;unsigned n,total,epoch,spin;unsigned pad[2];Layer L[8];};
static_assert(sizeof(Args)==32+192*8,"mhp Args");
struct Run{int w=-1;void* stream{};unsigned n=0,items=0,maxwin=0;int h=0,wd=0;const void* last_out{};void* f[8]{};Layer L[8];};
inline Run run;
inline unsigned long long runs=0,layers=0;inline unsigned logged=0;
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

inline void colher() noexcept {
 if(!pend||!event_query||event_query(ev)!=0)return;
 pend=false;ult=nrr1dmov::medida_de(stats_host);nova=ult.ok;if(nova)++medidas;
}
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
 std::ifstream f(path,std::ios::binary|std::ios::ate);
 if(!f||f.tellg()!=static_cast<std::streamoff>(size))return false;
 bytes.resize(size);f.seekg(0);return bool(f.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size())));
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
static_assert(!nrmhcontract::fast51_available||(nrmhcontract::fast51_varparams_bytes==192&&nrregpolicy::varparams_wide_bytes>=192),"VarParams Fast v0.5.1 = 192 B no buffer largo");
namespace fast {
inline bool requested=false;
inline unsigned version=42;
inline unsigned mask=7u;
inline unsigned sw32_mask=0xffu;
inline std::vector<unsigned char> image;
inline std::vector<unsigned char> image43;

inline std::vector<unsigned char> image51;
inline void* module51{};
inline unsigned v051_sw32_mask=0x5Fu,v051_mh_mask=0x30u,v051_1d_mask=0u;
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
   const auto load_data=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipModuleLoadData"));
   if(!load_data)return;
   if(!image43.empty()){
    if(!load_data(&module,image43.data())&&module){
     version=43;
     if(!image51.empty()&&(load_data(&module51,image51.data())||!module51)){module51=nullptr;log("FAST51 FALLBACK nr_reg_fast51.hsaco load rejected; tudo no v0.4.3");}
     return;
    }
    module=nullptr;log("FAST43 FALLBACK nr_reg_fast43.hsaco load rejected; tentando o v0.4.2");
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

inline bool resolve51(int(*function)(void**,void*,const char*),const char* const* names,void** out,unsigned n,unsigned bits) noexcept {
 if(!module51||!bits)return false;
 for(unsigned i=0;i<n;++i){out[i]=nullptr;if((bits>>i&1u)&&names[i]&&(function(&out[i],module51,names[i])||!out[i]))return false;}
 return true;
}
}
inline void initialize() noexcept {
 try {
  auto hip=GetModuleHandleW(L"amdhip64_7.dll");if(!hip){log("FALLBACK missing HIP runtime");return;}
  const auto properties=reinterpret_cast<int(*)(hipDeviceProp_t*,int)>(GetProcAddress(hip,"hipGetDevicePropertiesR0600"));
  get_device=reinterpret_cast<decltype(get_device)>(GetProcAddress(hip,"hipGetDevice"));
  const auto load=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipModuleLoadData"));
  const auto function=reinterpret_cast<int(*)(void**,void*,const char*)>(GetProcAddress(hip,"hipModuleGetFunction"));
  const auto by_symbol=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipGetFuncBySymbol"));
  const auto name=reinterpret_cast<const char*(*)(void*)>(GetProcAddress(hip,"hipKernelNameRef"));
  dispatch.module_launch=reinterpret_cast<nrregpolicy::ModuleLaunch>(GetProcAddress(hip,"hipModuleLaunchKernel"));
  if(!properties||!get_device||!load||!function||!by_symbol||!name||!dispatch.module_launch){log("FALLBACK HIP API missing");return;}
  hipDeviceProp_t props{};
  if(get_device(&device)||properties(&props,device)||std::strncmp(props.gcnArchName,"gfx1201",7)||(props.gcnArchName[7]&&props.gcnArchName[7]!=':')||props.warpSize!=32){log("FALLBACK device is not gfx1201 wave32");return;}
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

   bool r55_ativo[8]{};
   if(sw32x::variant&&!sw32x::image.empty()){
    void* x[8]{};bool ok=fast::version==43&&!load(&sw32x::module,sw32x::image.data())&&sw32x::module;
    for(unsigned i=0;ok&&i<8;++i)if(nr1080::contract::sw32x_swin32[i])ok=!function(&x[i],sw32x::module,nr1080::contract::sw32x_swin32[i])&&x[i];
    if(ok){
     char line[160]{};int n=NRLOGF(line,sizeof line,"SW32X %s modos",sw32x::variant->tag);
     for(unsigned i=0;i<8;++i)if(nr1080::sw32x_mode(i,applied43[i])&&x[i]){dispatch.functions[i]=x[i];r55_ativo[i]=sw32x::variant==nr1080::sw32x_r55();n+=NRLOGF(line+n,sizeof line-n," %u",nrregpolicy::modes[i]);}
     log(line);sw32x::applied=nrsw32x::escolha.load(std::memory_order_relaxed);
    }else log(fast::version==43?"SW32X FALLBACK modulo ou simbolo recusado; swin32 fica no Fast v0.4.3":"SW32X FALLBACK swin32 fora do Fast v0.4.3; modulo novo nao usado");
   }

   if(fast::module51&&fast::version==43){
    void* x[8]{};unsigned bits=fast::v051_sw32_mask;for(unsigned i=0;i<8;++i)if(!applied43[i])bits&=~(1u<<i);
    if(fast::resolve51(function,nrmhcontract::fast51_swin32,x,8,bits)){
     char line[200]{};int n=NRLOGF(line,sizeof line,"SW32 V051 modos");
     for(unsigned i=0;i<8;++i)if(x[i]){
      const bool r55_fica=nr1080::r55_sw32&&nr1080::contract::sw32x_swin32[i]&&nr1080::r55_vence_sw32(i);
      if(!(r55_fica&&r55_ativo[i]))dispatch.functions[i]=x[i];
      for(int v=0;v<=2;++v)if(sw32x::have[v]&&!(v==2&&r55_fica))sw32x::sets[v][i]=x[i];
      n+=NRLOGF(line+n,sizeof line-n,r55_fica?" %u:R55":" %u",nrregpolicy::modes[i]);}
     log(line);
    }else if(bits)log("SW32 V051 FALLBACK simbolo recusado; swin32 fica como estava");
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
  const auto properties=reinterpret_cast<int(*)(hipDeviceProp_t*,int)>(GetProcAddress(hip,"hipGetDevicePropertiesR0600"));
  const auto current=reinterpret_cast<int(*)(int*)>(GetProcAddress(hip,"hipGetDevice"));
  const auto load=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipModuleLoadData"));
  const auto function=reinterpret_cast<int(*)(void**,void*,const char*)>(GetProcAddress(hip,"hipModuleGetFunction"));
  const auto by_symbol=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipGetFuncBySymbol"));
  const auto name=reinterpret_cast<const char*(*)(void*)>(GetProcAddress(hip,"hipKernelNameRef"));
  mh::module_launch=reinterpret_cast<nrregpolicy::ModuleLaunch>(GetProcAddress(hip,"hipModuleLaunchKernel"));
  if(!properties||!current||!load||!function||!by_symbol||!name||!mh::module_launch){log("MH FALLBACK HIP API missing");return;}
  if(!get_device)get_device=current;
  hipDeviceProp_t props{};
  if(current(&mh::device)||properties(&props,mh::device)||std::strncmp(props.gcnArchName,"gfx1201",7)||(props.gcnArchName[7]&&props.gcnArchName[7]!=':')||props.warpSize!=32){log("MH FALLBACK device is not gfx1201 wave32");return;}
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

   void* v51[3][8]{};
   if(ok&&fast::module51&&fast::version==43){
    void* x[3][8]{};bool ok51=true;
    for(unsigned w=0;ok51&&w<3;++w)ok51=fast::resolve51(function,nrmhcontract::fast51_symbols[w],x[w],8,fast::v051_mh_mask);
    if(ok51&&fast::v051_mh_mask){
     char line[128]{};int n=NRLOGF(line,sizeof line,"MH V051 modos");
     for(unsigned i=0;i<8;++i)if(x[0][i]){for(unsigned w=0;w<3;++w){mh::functions[w][i]=x[w][i];v51[w][i]=x[w][i];}n+=NRLOGF(line+n,sizeof line-n," %u",nrmhcontract::modes[i]);}
     log(line);
    }else if(fast::v051_mh_mask)log("MH V051 FALLBACK simbolo recusado; mh fica no v0.4.3");
   }

   for(unsigned w=0;ok&&w<3;++w){
    if(mh::r55_image[w].empty())continue;
    const auto* r=mh::r55_borda[w]?nr1080::r55_mh_borda(w):nr1080::r55_mh(w);char line[200]{};int n=0;
    if(fast::version!=43||load(&mh::r55_module[w],mh::r55_image[w].data())||!mh::r55_module[w]){
     mh::r55_module[w]=nullptr;NRLOGF(line,sizeof line,"R55 %s FALLBACK modulo recusado ou Fast v0.4.3 ausente; fica o Fast",r->tag);log(line);continue;}
    n=NRLOGF(line,sizeof line,"R55 %s READY %ls modos",r->tag,r->file);
    for(unsigned i=0;i<8;++i){
     if(!nr1080::r55_mh_mode(nrmhcontract::modes[i],true))continue;
     if(v51[w][i]&&!nr1080::r55_vence_mh(w,i)){n+=NRLOGF(line+n,sizeof line-n," %u:V051",nrmhcontract::modes[i]);continue;}
     void* g{};
     if(function(&g,mh::r55_module[w],t.mh[w][i])||!g){n+=NRLOGF(line+n,sizeof line-n," %u:FALLBACK",nrmhcontract::modes[i]);continue;}
     mh::functions[w][i]=g;mhp::r55[w][i]=true;n+=NRLOGF(line+n,sizeof line-n," %u",nrmhcontract::modes[i]);
    }
    log(line);
   }
  }

  if(mhp::requested&&!mhp::image.empty()){
   const auto hmalloc=reinterpret_cast<int(*)(void**,std::size_t)>(GetProcAddress(hip,"hipMalloc"));
   const auto hmemset=reinterpret_cast<int(*)(void*,int,std::size_t)>(GetProcAddress(hip,"hipMemset"));
   const auto* m=nr1080::r55_mhp();bool ok=hmalloc&&hmemset&&!load(&mhp::module,mhp::image.data())&&mhp::module;
   for(unsigned w=0;ok&&w<3;++w)for(unsigned v=0;ok&&v<3;++v)ok=!function(&mhp::fn[w][v],mhp::module,m->names[w][v])&&mhp::fn[w][v];
   const std::size_t bytes=4ull*(16ull+mhp::cap_items);
   for(unsigned w=0;ok&&w<3;++w)ok=!hmalloc(&mhp::sync[w],bytes)&&mhp::sync[w]&&!hmemset(mhp::sync[w],0,bytes);
   for(unsigned w=0;ok&&w<3;++w)for(unsigned k=0;ok&&k<mhp::max_streams;++k)
    ok=!hmalloc(&mhp::sync_s[w][k],bytes)&&mhp::sync_s[w][k]&&!hmemset(mhp::sync_s[w][k],0,bytes);
   if(ok&&mhp::monitor){
    const auto hhost=reinterpret_cast<int(*)(void**,std::size_t,unsigned)>(GetProcAddress(hip,"hipHostMalloc"));
    mhp::memcpy_async=reinterpret_cast<decltype(mhp::memcpy_async)>(GetProcAddress(hip,"hipMemcpyAsync"));
    void* h{};if(hhost&&mhp::memcpy_async&&!hhost(&h,4*3*3*mhp::max_streams,0)&&h){mhp::host_err=static_cast<unsigned*>(h);std::memset(h,0,4*3*3*mhp::max_streams);log("R55 MHP monitor ativo (erro [2] de cada contador a cada 64 corridas; NeuralR55MhPersistMonitor=0 desliga)");}
    else{mhp::monitor=0;log("R55 MHP monitor indisponivel (hipHostMalloc/hipMemcpyAsync)");}
   }

   const auto hsync=reinterpret_cast<int(*)()>(GetProcAddress(hip,"hipDeviceSynchronize"));
   ok=ok&&hsync&&!hsync();
   bool any=false;for(unsigned w=0;w<3;++w)for(unsigned i=0;i<8;++i)any|=mhp::r55[w][i];
   mhp::ready=ok&&any;
   log(mhp::ready?"R55 MHP READY nr_r55_mhp.hsaco (corrida persistente do mh, NeuralR55MhPersist=1)":
       any?"R55 MHP FALLBACK modulo/simbolo/memoria recusado; um lancamento por bloco":"R55 MHP FALLBACK nenhum mh R55 ativo (NeuralR55Mh64/128/256); um lancamento por bloco");
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

inline int mhp_flush_locked() noexcept {
 auto& r=mhp::run;
 if(!r.n)return 0;
 const unsigned window=64u<<r.w;int e=0;
 if(r.n==1){
  alignas(16) unsigned char vp[192]{};std::memcpy(vp,r.L[0].vp,176);void* a[]{vp};
  e=mh::module_launch(r.f[0],r.L[0].gx,r.L[0].gy,1,window,1,1,0,r.stream,a,nullptr);
 }else{
  static mhp::Args A;
  std::memset(&A,0,sizeof A);

  int k=-1;for(unsigned i=0;i<mhp::max_streams;++i)if(mhp::sync_used[r.w][i]&&mhp::sync_stream[r.w][i]==r.stream){k=int(i);break;}
  if(k<0)for(unsigned i=0;i<mhp::max_streams;++i)if(!mhp::sync_used[r.w][i]&&mhp::sync_s[r.w][i]){mhp::sync_used[r.w][i]=true;mhp::sync_stream[r.w][i]=r.stream;k=int(i);
   char line[160]{};NRLOGF(line,sizeof line,"R55 MHP stream %p ganhou contador proprio W=%u vaga=%d",r.stream,window,k);log(line);break;}
  if(k<0){
   for(unsigned i=0;i<r.n&&!e;++i){alignas(16) unsigned char vp[192]{};std::memcpy(vp,r.L[i].vp,176);void* a1[]{vp};
    e=mh::module_launch(r.f[i],r.L[i].gx,r.L[i].gy,1,window,1,1,0,r.stream,a1,nullptr);}
   r.n=0;r.items=0;r.maxwin=0;r.last_out=nullptr;
   return e?mh_fail(e):0;
  }
  A.sync=mhp::sync_s[r.w][k];A.n=r.n;A.total=r.items;
  if(++mhp::epoch_s[r.w][k]==0)mhp::epoch_s[r.w][k]=1;
  A.epoch=mhp::epoch_s[r.w][k];A.spin=1u<<22;
  if(mhp::monitor&&mhp::host_err&&((mhp::runs&7u)==0)){
   ++mhp::monitor_checks;

   for(unsigned w2=0;w2<3;++w2)for(unsigned k2=0;k2<mhp::max_streams;++k2){const unsigned* v=mhp::host_err+(w2*mhp::max_streams+k2)*3;
    if(v[0]!=mhp::last_err[w2][k2][0]||v[1]!=mhp::last_err[w2][k2][1]||v[2]!=mhp::last_err[w2][k2][2]){
     for(int q=0;q<3;++q)mhp::last_err[w2][k2][q]=v[q];++mhp::monitor_errors;char line[200]{};
     NRLOGF(line,sizeof line,"R55 MHP DETECT W=%u vaga=%u estouro=%u pos_barreira=%u args_velhos=0x%x corridas=%llu",64u<<w2,k2,v[0],v[1],v[2],mhp::runs);log(line);}}
   mhp::memcpy_async(mhp::host_err+(r.w*mhp::max_streams+unsigned(k))*3,static_cast<unsigned char*>(A.sync)+8,12,2,r.stream);
  }
  if(mhp::monitor&&((mhp::runs&4095u)==0)&&mhp::runs){char line[160]{};NRLOGF(line,sizeof line,"R55 MHP monitor corridas=%llu conferencias=%llu estouros=%llu",mhp::runs,mhp::monitor_checks,mhp::monitor_errors);log(line);}
  for(unsigned i=0;i<r.n;++i){A.L[i]=r.L[i];std::memcpy(A.L[i].vp+0xa8,&A.epoch,4);}
  void* a[]{&A};
  unsigned modos=0;for(unsigned i=0;i<r.n;++i)modos|=1u<<r.L[i].mode;
  const unsigned grade=(mhp::grade_max&&mhp::grade_max<r.maxwin)?mhp::grade_max:r.maxwin;
  e=mh::module_launch(mhp::fn[r.w][nr1080::mhp_variante(modos)],grade,1,1,window,1,1,0,r.stream,a,nullptr);
  ++mhp::runs;mhp::layers+=r.n;
  if(mhp::logged<8){
   ++mhp::logged;char line[256]{};int nl=NRLOGF(line,sizeof line,"R55 MHP corrida W=%u blocos=%u itens=%u grade=%u modos",window,r.n,r.items,r.maxwin);
   for(unsigned i=0;i<r.n&&nl>0&&nl<200;++i)nl+=NRLOGF(line+nl,sizeof line-nl," %u(%ux%u)",r.L[i].mode,r.L[i].gx,r.L[i].gy);
   log(line);
   for(unsigned i=0;i<r.n;++i){std::uint64_t pi{},po{},p30{},p38{};std::memcpy(&pi,r.L[i].vp,8);std::memcpy(&po,r.L[i].vp+8,8);std::memcpy(&p30,r.L[i].vp+0x30,8);std::memcpy(&p38,r.L[i].vp+0x38,8);
    char l2[200]{};NRLOGF(l2,sizeof l2,"R55 MHP   camada %u modo %u in=%llx out=%llx aux30=%llx aux38=%llx",i,r.L[i].mode,(unsigned long long)pi,(unsigned long long)po,(unsigned long long)p30,(unsigned long long)p38);log(l2);}
  }
 }
 r.n=0;r.items=0;r.maxwin=0;r.last_out=nullptr;
 return e?mh_fail(e):0;
}
inline int mhp_flush() noexcept {
 if(!mhp::ready)return 0;
 std::lock_guard<std::mutex> lock(mh::chain_mutex);
 return mhp_flush_locked();
}

inline int mh_flush_pending() noexcept {
 if(!on_owner_device(mh::device))return 0;
 std::lock_guard<std::mutex> lock(mh::chain_mutex);
 nrmhpolicy::Tensor t{};
 if(!mh::chain.take(t))return 0;
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
  if(here){if(const int e=mhp_flush())return e;}
  if(here){if(const int e=mh_flush_pending())return e;}
  if(here){if(const int e=mh_flush_park(w))return e;}
  return fallback(f,g,b,args,shared,stream);
 }
 const unsigned window=64u<<w;
 nrmhpolicy::Plan p{};
 if(!nrmhpolicy::plan(window,args[0],p)){
  if(const int e=mhp_flush())return e;
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
 const auto step=mh::chain.begin(p,p.mh_mode,stream,mh::chain_enabled);

 const bool mhp_pre=step.flush||step.flush_park||(p.convert_input&&!step.input_ready)||p.convert_aux;
 const bool mhp_post=nrmhpolicy::restore_input(p,mh::drop_restore)||p.convert_aux||(p.convert_output&&!step.defer_output&&!step.park_output);
 const unsigned mhp_items=g.x*g.y;
 const bool mhp_can=mhp::ready&&mhp::r55[w][p.mode_index]&&g.z==1&&nr1080::mhp_mode(p.mh_mode)&&mhp_items<=mhp::cap_items&&
                    ((mhp::mascara>>w)&1u)&&nr1080::mhp_vale(w,mhp_items,mhp::modo);
 if(mhp::run.n&&(!mhp_can||mhp_pre)){if(const int e=mhp_flush_locked())return e;}
 if(step.flush){if(nrtimeline::requested)nrtimeline::flushed=true;if(const int e=mh_convert(false,step.flush_tensor.ptr,step.flush_tensor.bytes,step.flush_tensor.shift,step.flush_tensor.stream))return fail(e);}
 if(step.flush_park){if(nrtimeline::requested)nrtimeline::flushed=true;if(const int e=mh_convert(false,step.park_tensor.ptr,step.park_tensor.bytes,step.park_tensor.shift,step.park_tensor.stream))return fail(e);}
 if(p.convert_input&&!step.input_ready){if(const int e=mh_convert(true,p.input,p.bytes,p.shift,stream))return fail(e);}
 if(p.convert_aux){if(const int e=mh_convert(true,p.aux,p.bytes,p.shift,stream))return fail(e);}
 alignas(16) unsigned char wide_params[192]{};void* wide_args[]{wide_params};
 if(mh::wide)std::memcpy(wide_params,args[0],168);
 if(mhp_can){
  auto& r=mhp::run;
  const char* vp=static_cast<const char*>(args[0]);const void* in{};const void* out{};int hh{},ww{};
  std::memcpy(&in,vp,8);std::memcpy(&out,vp+8,8);std::memcpy(&hh,vp+0x18,4);std::memcpy(&ww,vp+0x1c,4);
  const nr1080::MhpTail t{r.w,r.stream,r.h,r.wd,r.last_out,r.n,r.items};
  if(r.n&&!nr1080::mhp_join(t,w,stream,hh,ww,in,p.mh_mode,mhp_items,mhp::cap_items)){if(const int e=mhp_flush_locked())return e;}
  auto& L=r.L[r.n];std::memset(&L,0,sizeof L);std::memcpy(L.vp,args[0],168);
  L.gx=g.x;L.gy=g.y;L.mode=p.mh_mode;L.first=r.items;r.f[r.n]=mh::functions[w][p.mode_index];
  r.w=w;r.stream=stream;r.h=hh;r.wd=ww;r.last_out=out;r.items+=mhp_items;r.maxwin=r.maxwin>mhp_items?r.maxwin:mhp_items;++r.n;
  if(mhp_post||nr1080::mhp_closes(p.mh_mode,r.n)){if(const int e=mhp_flush_locked())return e;}
 }else if(const int e=mh::module_launch(mh::functions[w][p.mode_index],g.x,g.y,g.z,window,1,1,0,stream,mh::wide?wide_args:args,nullptr))return fail(e);

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
  const auto load=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipModuleLoadData"));
  const auto function=reinterpret_cast<int(*)(void**,void*,const char*)>(GetProcAddress(hip,"hipModuleGetFunction"));
  const auto by_symbol=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipGetFuncBySymbol"));
  const auto name=reinterpret_cast<const char*(*)(void*)>(GetProcAddress(hip,"hipKernelNameRef"));
  const auto alloc=reinterpret_cast<int(*)(void**,std::size_t)>(GetProcAddress(hip,"hipMalloc"));
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
  const auto load=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipModuleLoadData"));
  const auto function=reinterpret_cast<int(*)(void**,void*,const char*)>(GetProcAddress(hip,"hipModuleGetFunction"));
  const auto by_symbol=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipGetFuncBySymbol"));
  const auto name=reinterpret_cast<const char*(*)(void*)>(GetProcAddress(hip,"hipKernelNameRef"));
  const auto alloc=reinterpret_cast<int(*)(void**,std::size_t)>(GetProcAddress(hip,"hipMalloc"));
  c1d43::memset_async=reinterpret_cast<int(*)(void*,int,std::size_t,void*)>(GetProcAddress(hip,"hipMemsetAsync"));
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

  if(fast::requested&&!fast::image51.empty()){
   fast::load();
   if(fast::module51&&fast::v051_1d_mask){
    const unsigned idx[2]{5,6};char line[128]{};int n=NRLOGF(line,sizeof line,"1D43 V051");
    for(unsigned k=0;k<2;++k){void* f{};if((fast::v051_1d_mask>>k&1u)&&!function(&f,fast::module51,nr1d43contract::symbols[idx[k]])&&f){c1d43::functions[idx[k]]=f;n+=NRLOGF(line+n,sizeof line-n," %s",k?"attn":"qkv");}}
    log(line);
   }
  }

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
  const auto load=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipModuleLoadData"));
  const auto function=reinterpret_cast<int(*)(void**,void*,const char*)>(GetProcAddress(hip,"hipModuleGetFunction"));
  const auto by_symbol=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipGetFuncBySymbol"));
  const auto name=reinterpret_cast<const char*(*)(void*)>(GetProcAddress(hip,"hipKernelNameRef"));
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
  const auto properties=reinterpret_cast<int(*)(hipDeviceProp_t*,int)>(GetProcAddress(hip,"hipGetDevicePropertiesR0600"));
  const auto current=reinterpret_cast<int(*)(int*)>(GetProcAddress(hip,"hipGetDevice"));
  const auto load=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipModuleLoadData"));
  const auto function=reinterpret_cast<int(*)(void**,void*,const char*)>(GetProcAddress(hip,"hipModuleGetFunction"));
  const auto by_symbol=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipGetFuncBySymbol"));
  const auto name=reinterpret_cast<const char*(*)(void*)>(GetProcAddress(hip,"hipKernelNameRef"));
  cauda::module_launch=reinterpret_cast<nrregpolicy::ModuleLaunch>(GetProcAddress(hip,"hipModuleLaunchKernel"));
  if(!properties||!current||!load||!function||!by_symbol||!name||!cauda::module_launch){log("FINALHEAD FALLBACK HIP API missing");return;}
  if(!get_device)get_device=current;
  hipDeviceProp_t props{};
  if(current(&cauda::device)||properties(&props,cauda::device)||std::strncmp(props.gcnArchName,"gfx1201",7)||(props.gcnArchName[7]&&props.gcnArchName[7]!=':')||props.warpSize!=32){log("FINALHEAD FALLBACK device is not gfx1201 wave32");return;}
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
 std::lock_guard<std::mutex> lock(sw32_mutex);
 if(!dispatch.pending)return 0;
 if(!on_owner_device(device))return 0;
 if(nrtimeline::requested)nrtimeline::flushed=true;
 return dispatch.flush();
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
    if(r1d::cap<bytes){void* m{};if(r1d::cache&&v05::free_fn)v05::free_fn(r1d::cache);r1d::cache=nullptr;r1d::cap=0;
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

 if(mhp::ready&&f!=mh::tokens[0]&&f!=mh::tokens[1]&&f!=mh::tokens[2]){if(const int e=mhp_flush())return e;}

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
 int result{};bool chained{};
 {
  std::lock_guard<std::mutex> lock(sw32_mutex);
  const bool had=dispatch.pending;const void* held=dispatch.held.ptr;const unsigned flushes=dispatch.flushes;
  chained=had&&index>=0&&args&&args[0]&&*static_cast<void* const*>(args[0])==held;
  result=dispatch.launch(f,g,b,args,shared,stream);
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

namespace hostgap {
inline bool enabled=false;
inline std::mutex mutex;
inline long long frequency{};
inline int(*query)(void*){};
inline int(*by_symbol)(void**,const void*){};
inline const char*(*name_ref)(void*){};
struct Pair {std::string key;unsigned long long n{},probed{},idle{};long long ticks{};};
inline std::vector<Pair> pairs;
inline std::vector<std::pair<const void*,std::string>> names;
inline std::string last;inline long long last_exit{};inline unsigned long long launches{};
inline std::string kernel_name(const void* f) {
 for(const auto& p:names)if(p.first==f)return p.second;
 std::string n;void* fn{};const char* text{};
 if(by_symbol&&name_ref&&!by_symbol(&fn,f)&&fn&&(text=name_ref(fn))!=nullptr)n=text;else{char b[40]{};NRLOGF(b,sizeof b,"token@%p",f);n=b;}
 names.emplace_back(f,n);return n;
}
inline void report_locked() {
 std::sort(pairs.begin(),pairs.end(),[](const Pair& a,const Pair& b){return a.ticks>b.ticks;});
 char line[512]{};NRLOGF(line,sizeof line,"HOSTGAP launches=%llu top pares por CPU entre lancamentos:",launches);log(line);
 for(std::size_t i=0;i<pairs.size();++i){
  const auto& p=pairs[i];
  if(i>=12&&!p.probed)continue;
  NRLOGF(line,sizeof line,"HOSTGAP  n=%llu cpu_us_medio=%.1f idle=%llu/%llu  %s",p.n,p.n?1e6*double(p.ticks)/double(frequency)/double(p.n):0.0,p.idle,p.probed,p.key.c_str());log(line);
 }
}
inline void configure(const std::wstring& ini) noexcept {
 enabled=!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralHostGapLog",0,ini.c_str())==1;
 if(!enabled)return;
 const auto hip=GetModuleHandleW(L"amdhip64_7.dll");
 query=hip?reinterpret_cast<int(*)(void*)>(GetProcAddress(hip,"hipStreamQuery")):nullptr;
 by_symbol=hip?reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipGetFuncBySymbol")):nullptr;
 name_ref=hip?reinterpret_cast<const char*(*)(void*)>(GetProcAddress(hip,"hipKernelNameRef")):nullptr;
 LARGE_INTEGER f{};QueryPerformanceFrequency(&f);frequency=f.QuadPart;
}
}
inline int launch_inner(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 if(nrtimeline::requested)return launch_timed(f,g,b,args,shared,stream);
 return launch_untimed(f,g,b,args,shared,stream);
}
inline int launch_gap(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 LARGE_INTEGER a{};QueryPerformanceCounter(&a);
 std::string name;
 {std::lock_guard<std::mutex> lock(hostgap::mutex);try{name=hostgap::kernel_name(f);}catch(...){}}
 const bool swin=f==dispatch.token||f==dispatch.alternate_token;
 if(swin&&args&&args[0]){unsigned mode{};std::memcpy(&mode,static_cast<const char*>(args[0])+0x28,sizeof(mode));name+="#m"+std::to_string(mode);}
 const bool probe=swin||name.find("k_export")!=std::string::npos||name.find("k_mean")!=std::string::npos||name.find("k_flag_set")!=std::string::npos;
 int idle=-1;if(probe&&hostgap::query)idle=hostgap::query(stream)==0?1:0;
 {
  std::lock_guard<std::mutex> lock(hostgap::mutex);
  if(hostgap::last_exit)try{
   const std::string key=hostgap::last+" -> "+name;
   hostgap::Pair* p=nullptr;for(auto& x:hostgap::pairs)if(x.key==key){p=&x;break;}
   if(!p){hostgap::pairs.push_back({key});p=&hostgap::pairs.back();}
   ++p->n;p->ticks+=a.QuadPart-hostgap::last_exit;if(idle>=0){++p->probed;p->idle+=unsigned(idle);}
  }catch(...){}
 }
 const int result=launch_inner(f,g,b,args,shared,stream);
 LARGE_INTEGER e{};QueryPerformanceCounter(&e);
 {std::lock_guard<std::mutex> lock(hostgap::mutex);hostgap::last=std::move(name);hostgap::last_exit=e.QuadPart;if(++hostgap::launches%40000u==0)try{hostgap::report_locked();}catch(...){}}
 return result;
}
inline int launch_normal(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 if(hostgap::enabled)return launch_gap(f,g,b,args,shared,stream);
 return launch_inner(f,g,b,args,shared,stream);
}

namespace reuso {
inline nrreuso::Maquina maq;
inline std::mutex mutex;
inline int corte=4;inline int janela=2;inline bool copia=true;inline bool prova=false;
inline int(*memcpy_async)(void*,const void*,std::size_t,int,void*){};
inline int(*memcpy_sync)(void*,const void*,std::size_t,int){};
inline int(*malloc_)(void**,std::size_t){};
inline int(*stream_sync)(void*){};
inline int(*by_symbol)(void**,const void*){};
inline const char*(*name_ref)(void*){};
inline void* cache{};inline std::uint64_t cache_cap{};
inline void* aux_guardado{};inline std::uint64_t bytes_guardado{};
inline std::uint64_t forma4{};
inline unsigned nomes_logados{};
inline std::string nome(const void* f) {
 void* fn{};const char* t{};
 if(by_symbol&&name_ref&&!by_symbol(&fn,f)&&fn&&(t=name_ref(fn))!=nullptr)return t;
 char b[40]{};NRLOGF(b,sizeof b,"token@%p",f);return b;
}
inline bool pulavel(const void* f) noexcept {
 for(const void* t:vit::tokens)if(f==t)return true;
 for(const void* t:c1d43::tokens)if(f==t)return true;
 for(const void* t:c1d43::foreign)if(f==t)return true;
 if(f==c1d::token||f==cauda::token||f==decup::token)return true;
 if(corte<=3&&f==mh::tokens[2])return true;
 if(corte==2&&f==mh::tokens[1])return true;
 return false;
}

namespace proj {
inline int modo=0;inline int celula=0;inline float limiar_px=0.f,fracao=10.f,n2_px=0.f;inline bool cadeia=true;
inline float div_px=2.f,div_fracao=0.f;inline unsigned max_quadros=8;
inline std::uint64_t q_ref=0;
inline unsigned k_job=1;inline unsigned long long k_soma{},k_n{};inline unsigned k_max{};
inline std::vector<unsigned char> image;inline std::once_flag init;inline bool ready=false;inline bool failed=false;
inline void* module{};inline void* k_celulas{};inline void* k_warp{};
using ModuleLaunch=int(*)(void*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,void*,void**,void**);
inline ModuleLaunch mlaunch{};inline int(*memset_async)(void*,int,std::size_t,void*){};
inline int(*host_malloc)(void**,std::size_t,unsigned){};inline int(*event_create)(void**,unsigned){};
inline int(*event_record)(void*,void*){};inline int(*event_query)(void*){};

struct Mv {const void* mv;int pitch,fmt,mvw,mvh;float sx,sy;int neth,netw;const void* dep;int dpitch,dfmt,dw,dh,dinv;};
static_assert(sizeof(Mv)==72,"Mv 72 B");
inline Mv mv{};inline unsigned mv_job=~0u;
inline void* d{};inline std::uint64_t d_cap{};inline void* stats_dev{};inline unsigned* stats_host{};inline void* ev{};
inline bool d_ok=false;
inline bool stats_pend=false;

inline float ult_media=0.f,ult_frac=0.f,ult_max=0.f,ult_div=0.f;inline bool ult_ok=false;
inline unsigned warps{},copias_sem_mv{},jobs_medidos{};inline unsigned logs{};
inline float real(const std::wstring& ini,const wchar_t* k,float def) noexcept {
 float v{};return nrinijogo::ler_float_em(ini.c_str(),k,v,64)?v:def;
}
inline void configure(const std::wstring& ini) noexcept;
inline bool initialize(void* hip) noexcept {
 auto load=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(static_cast<HMODULE>(hip),"hipModuleLoadData"));
 auto function=reinterpret_cast<int(*)(void**,void*,const char*)>(GetProcAddress(static_cast<HMODULE>(hip),"hipModuleGetFunction"));
 mlaunch=reinterpret_cast<ModuleLaunch>(GetProcAddress(static_cast<HMODULE>(hip),"hipModuleLaunchKernel"));
 memset_async=reinterpret_cast<decltype(memset_async)>(GetProcAddress(static_cast<HMODULE>(hip),"hipMemsetAsync"));
 host_malloc=reinterpret_cast<decltype(host_malloc)>(GetProcAddress(static_cast<HMODULE>(hip),"hipHostMalloc"));
 if(!host_malloc)host_malloc=reinterpret_cast<decltype(host_malloc)>(GetProcAddress(static_cast<HMODULE>(hip),"hipExtHostAlloc"));
 event_create=reinterpret_cast<decltype(event_create)>(GetProcAddress(static_cast<HMODULE>(hip),"hipEventCreateWithFlags"));
 event_record=reinterpret_cast<decltype(event_record)>(GetProcAddress(static_cast<HMODULE>(hip),"hipEventRecord"));
 event_query=reinterpret_cast<decltype(event_query)>(GetProcAddress(static_cast<HMODULE>(hip),"hipEventQuery"));
 if(!load||!function||!mlaunch||!memset_async||!host_malloc||!event_create||!event_record||!event_query)return false;
 if(load(&module,image.data())||!module||function(&k_celulas,module,"rp_celulas")||!k_celulas||function(&k_warp,module,"rp_warp")||!k_warp)return false;
 void* h{};if(host_malloc(&h,64,0)||!h)return false;stats_host=static_cast<unsigned*>(h);std::memset(h,0,64);
 if(event_create(&ev,2u)||!ev)return false;
 return true;
}
}
inline void configure(const std::wstring& ini) noexcept {
 if(ini.empty())return;
 const int n=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralReusoFundo",0,ini.c_str()));
 corte=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralReusoFundoCorte",4,ini.c_str()));
 copia=NRCFG_INT(L"DlssNrOnAmd",L"NeuralReusoFundoCopia",1,ini.c_str())!=0;
 prova=NRCFG_INT(L"DlssNrOnAmd",L"NeuralReusoFundoProva",0,ini.c_str())!=0;
 janela=nrreuso::janela_fronteira(corte);
 if(n<2||janela<0){if(n)log("REUSO recusado: NeuralReusoFundo < 2 ou corte diferente de 2/3/4");return;}
 const auto hip=GetModuleHandleW(L"amdhip64_7.dll");
 if(!hip){log("REUSO recusado: amdhip64_7.dll ausente");return;}
 memcpy_async=reinterpret_cast<decltype(memcpy_async)>(GetProcAddress(hip,"hipMemcpyAsync"));
 memcpy_sync=reinterpret_cast<decltype(memcpy_sync)>(GetProcAddress(hip,"hipMemcpy"));
 malloc_=reinterpret_cast<decltype(malloc_)>(GetProcAddress(hip,"hipMalloc"));
 stream_sync=reinterpret_cast<decltype(stream_sync)>(GetProcAddress(hip,"hipStreamSynchronize"));
 by_symbol=reinterpret_cast<decltype(by_symbol)>(GetProcAddress(hip,"hipGetFuncBySymbol"));
 name_ref=reinterpret_cast<decltype(name_ref)>(GetProcAddress(hip,"hipKernelNameRef"));
 if(!memcpy_async||!memcpy_sync||!malloc_||!stream_sync){log("REUSO recusado: API HIP ausente");return;}
 maq.pausa_ms=NRCFG_INT(L"DlssNrOnAmd",L"NeuralReusoFundoPausaMs",250,ini.c_str());
 maq.n=static_cast<unsigned>(n);
 proj::configure(ini);
 char line[200]{};NRLOGF(line,sizeof line,"REUSO ligado: N=%d corte=%d (%s) copia=%d prova=%d pausa_ms=%u",n,corte,corte==4?"pula vit+1D":corte==3?"pula swin256+vit+1D":"pula swin128+swin256+vit+1D",copia?1:0,prova?1:0,maq.pausa_ms);log(line);
}
inline void proj::configure(const std::wstring& ini) noexcept {
 modo=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralReusoFundoReproj",0,ini.c_str()));
 if(modo!=1&&modo!=2){modo=0;return;}
 if(!copia){modo=0;log("REUSO PROJ recusado: precisa de NeuralReusoFundoCopia=1");return;}
 limiar_px=std::clamp(real(ini,L"NeuralReusoFundoReprojLimiarPx",0.f),0.f,1000.f);
 fracao=std::clamp(real(ini,L"NeuralReusoFundoReprojFracao",10.f),0.f,100.f);
 n2_px=std::clamp(real(ini,L"NeuralReusoFundoReprojN2Px",0.f),0.f,1000.f);
 cadeia=NRCFG_INT(L"DlssNrOnAmd",L"NeuralReusoFundoReprojCadeia",1,ini.c_str())!=0;
 {const int c=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralReusoFundoReprojCelula",0,ini.c_str()));celula=c==1||c==2?c:0;}
 div_px=std::clamp(real(ini,L"NeuralReusoFundoReprojDivPx",2.f),0.f,1000.f);
 div_fracao=std::clamp(real(ini,L"NeuralReusoFundoReprojDivFracao",0.f),0.f,100.f);
 max_quadros=unsigned(std::clamp<int>(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralReusoFundoReprojQuadros",8,ini.c_str())),0,32));
 if(!read_exact(folder/L"nr_reuso_proj.hsaco",nrreusoproj::module_bytes,image)||!nrliveimage::detail::sha256_matches(image.data(),image.size(),nrreusoproj::module_sha256)){
  modo=0;image.clear();log("REUSO PROJ FALLBACK nr_reuso_proj.hsaco ausente ou digest diferente: reuso sem reprojecao");return;}
 char line[220]{};NRLOGF(line,sizeof line,"REUSO PROJ pedido: modo=%d (%s) celula=%s limiar_px=%.2f fracao=%.1f%% n2_px=%.2f cadeia=%d div_px=%.2f div_fracao=%.1f%% quadros_max=%u",modo,modo==1?"bilinear":"vizinho",celula==2?"silhueta":celula?"maior":"media",limiar_px,fracao,n2_px,cadeia?1:0,div_px,div_fracao,max_quadros);log(line);
}

inline void provar(const char* quando,void* aux,std::uint64_t bytes,void* stream,bool reuso_job) {
 if(!prova)return;
 try{
  std::vector<unsigned char> h(static_cast<std::size_t>(bytes));
  if(stream_sync(stream)||memcpy_sync(h.data(),aux,h.size(),2)){log("REUSO PROVA falha na copia D2H");return;}
  char line[256]{};NRLOGF(line,sizeof line,"REUSO PROVA job=%u %s reuso=%d aux=%p bytes=%llu hash=%016llx",maq.job,quando,reuso_job?1:0,aux,
   static_cast<unsigned long long>(bytes),static_cast<unsigned long long>(nrreuso::fnv(h.data(),h.size())));log(line);
 }catch(...){}
}
}
inline int launch_reuso(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {

 if(args&&args[0]){
  if(nrpadapara::modo){const char* img=installed_image.load();if(img&&f==img+0x62228){std::lock_guard<std::mutex> lock(sw32_mutex);nrpadapara::no_import(args[0]);}}
  if(f==dispatch.token||f==dispatch.alternate_token)nrsementefixa::aplicar(args[0]);
  else if(nrexpofresca::ligado()){const char* img=installed_image.load();
   if(img&&f==img+0x62228)nrexpofresca::no_import(args[0]);else if(img&&f==img+0x62238)nrexpofresca::no_reproject(args[0]);}
 }

 if(!reuso::maq.n||!nrreusostate::cena)return launch_normal(f,g,b,args,shared,stream);
 const bool fronteira=f==mh::tokens[reuso::janela]&&args&&args[0];
 unsigned modo=~0u;if(fronteira)std::memcpy(&modo,static_cast<const char*>(args[0])+0x28,4);
 const bool pulavel=reuso::pulavel(f);
 std::unique_lock<std::mutex> lock(reuso::mutex);

 if(reuso::proj::modo&&args&&args[0]&&f==installed_image.load()+0x62238){
  const char* p=static_cast<const char*>(args[0]);reuso::proj::Mv m{};
  std::memcpy(&m.mv,p+0x10,8);std::memcpy(&m.pitch,p+0x18,4);std::memcpy(&m.fmt,p+0x1c,4);std::memcpy(&m.mvw,p+0x20,4);std::memcpy(&m.mvh,p+0x24,4);
  std::memcpy(&m.sx,p+0x58,4);std::memcpy(&m.sy,p+0x5c,4);std::memcpy(&m.neth,p+0x68,4);std::memcpy(&m.netw,p+0x6c,4);
  std::memcpy(&m.dep,p+0x30,8);std::memcpy(&m.dpitch,p+0x38,4);std::memcpy(&m.dfmt,p+0x3c,4);std::memcpy(&m.dw,p+0x40,4);std::memcpy(&m.dh,p+0x44,4);std::memcpy(&m.dinv,p+0x48,4);
  if(m.dep&&(m.dw<=0||m.dh<=0||m.dw>16384||m.dh>16384||m.dpitch<m.dw*4))m.dep=nullptr;
  const int bpp=m.fmt==1||m.fmt==2?8:4;
  const bool ok=m.mv&&m.fmt>=0&&m.fmt<=2&&m.mvw>0&&m.mvh>0&&m.mvw<=16384&&m.mvh<=16384&&m.pitch>=m.mvw*bpp&&m.netw>0&&m.neth>0&&m.netw<=16384&&m.neth<=16384&&std::isfinite(m.sx)&&std::isfinite(m.sy);
  if(ok){reuso::proj::mv=m;reuso::proj::mv_job=reuso::maq.job+1;}
  if(reuso::proj::logs<4){++reuso::proj::logs;char line[260]{};NRLOGF(line,sizeof line,"REUSO PROJ vetores job=%u %s mv=%p pitch=%d fmt=%d %dx%d escala=%.4f,%.4f rede=%dx%d",reuso::maq.job+1,ok?"ok":"RECUSADOS",m.mv,m.pitch,m.fmt,m.mvw,m.mvh,m.sx,m.sy,m.netw,m.neth);log(line);}
 }
 bool proj_forcar=false;unsigned proj_nmax=0;int proj_hh=0,proj_hw=0;
 if(reuso::proj::modo&&fronteira&&modo==4){
  std::call_once(reuso::proj::init,[]{reuso::proj::ready=!reuso::proj::image.empty()&&reuso::proj::initialize(GetModuleHandleW(L"amdhip64_7.dll"));
   log(reuso::proj::ready?"REUSO PROJ READY nr_reuso_proj.hsaco (rp_celulas, rp_warp)":"REUSO PROJ FALLBACK modulo recusado/API HIP ausente: reuso sem reprojecao");});
  if(reuso::proj::ready&&reuso::proj::stats_pend&&reuso::proj::event_query(reuso::proj::ev)==0){
   const unsigned* P=reuso::proj::stats_host;reuso::proj::stats_pend=false;
   if(P[3]){reuso::proj::ult_media=float(P[0])/16.f/float(P[3]);reuso::proj::ult_frac=100.f*float(P[1])/float(P[3]);reuso::proj::ult_max=float(P[2])/16.f;reuso::proj::ult_div=100.f*float(P[4])/float(P[3]);reuso::proj::ult_ok=true;}
  }
  if(reuso::proj::mv_job!=reuso::maq.job+1)reuso::proj::ult_ok=false;
  proj_forcar=(reuso::proj::limiar_px>0.f&&reuso::proj::ult_ok&&reuso::proj::ult_frac>reuso::proj::fracao)||
             (reuso::proj::div_fracao>0.f&&reuso::proj::ult_ok&&reuso::proj::ult_div>reuso::proj::div_fracao);
  proj_nmax=(reuso::proj::n2_px>0.f&&(!reuso::proj::ult_ok||reuso::proj::ult_media>=reuso::proj::n2_px))?2u:0u;
  int h4{},w4{};std::memcpy(&h4,static_cast<const char*>(args[0])+0x18,4);std::memcpy(&w4,static_cast<const char*>(args[0])+0x1c,4);
  proj_hh=h4/2;proj_hw=w4/2;
 }
 const bool entre_antes=reuso::maq.entre;
 const unsigned refrescos_antes=reuso::maq.refrescos,pausas_antes=reuso::maq.pausas;

 if(fronteira&&modo==4){
  int h4{},w4{};std::memcpy(&h4,static_cast<const char*>(args[0])+0x18,4);std::memcpy(&w4,static_cast<const char*>(args[0])+0x1c,4);
  const std::uint64_t forma=(std::uint64_t(std::uint32_t(h4))<<32)|std::uint32_t(w4);
  if(forma!=reuso::forma4){
   if(reuso::forma4){char line[160]{};NRLOGF(line,sizeof line,"REUSO TAMANHO job=%u: %dx%d (antes %ux%u): rede inteira",reuso::maq.job+1,w4,h4,unsigned(reuso::forma4&0xffffffffu),unsigned(reuso::forma4>>32));log(line);}
   reuso::forma4=forma;reuso::maq.cache_ok=false;
  }
 }
 const auto acao=reuso::maq.antes(fronteira,modo,pulavel,nrreusostate::refresco.load(std::memory_order_relaxed),fronteira&&modo==4?GetTickCount64():0,proj_forcar,proj_nmax);

 if(reuso::proj::modo&&fronteira&&modo==4){
  reuso::proj::d_ok=false;

  const int f8=8<<reuso::janela;auto cobre=[f8](int celulas,int px){return celulas*f8>=px&&celulas*f8-px<256;};
  const auto& m=reuso::proj::mv;
  if(reuso::proj::ready&&reuso::proj::mv_job==reuso::maq.job&&proj_hh>0&&proj_hw>0&&cobre(proj_hh,m.neth)&&cobre(proj_hw,m.netw)){
   const std::uint64_t need=std::uint64_t(proj_hh)*std::uint64_t(proj_hw)*8u;
   bool ok=true;
   if(reuso::proj::d_cap<need){void* novo{};if(reuso::malloc_(&novo,static_cast<std::size_t>(need))||!novo)ok=false;else{reuso::proj::d=novo;reuso::proj::d_cap=need;}}
   if(ok&&!reuso::proj::stats_dev){void* sd{};if(reuso::malloc_(&sd,128)||!sd)ok=false;else reuso::proj::stats_dev=sd;}

   {const std::uint64_t qn=nrreusostate::quadro_cena,qr=reuso::proj::q_ref;unsigned k=1;
    if(reuso::proj::max_quadros&&qn&&qr&&qn>qr)k=unsigned((std::min<std::uint64_t>)(qn-qr,reuso::proj::max_quadros));
    reuso::proj::k_job=k;if(acao==nrreuso::Acao::lancar_e_pular_depois){reuso::proj::k_soma+=k;++reuso::proj::k_n;reuso::proj::k_max=(std::max)(reuso::proj::k_max,k);}}
   if(ok){

    const bool medir=!reuso::proj::stats_pend;
    reuso::proj::Mv mm=m;mm.sx*=float(reuso::proj::k_job);mm.sy*=float(reuso::proj::k_job);
    int hh=proj_hh,hw=proj_hw,ff=f8,cel=reuso::proj::celula;float lim=reuso::proj::limiar_px>0.f?reuso::proj::limiar_px:2.f,dv=reuso::proj::div_px;
    void* dd=reuso::proj::d;void* st=static_cast<char*>(reuso::proj::stats_dev)+(medir?0:64);void* a[]{&mm,&hh,&hw,&ff,&cel,&lim,&dv,&dd,&st};
    ok=!reuso::proj::memset_async(st,0,32,stream)&&!reuso::proj::mlaunch(reuso::proj::k_celulas,unsigned(hw+63)/64u,unsigned(hh),1,64,1,1,0,stream,a,nullptr);
    if(ok)reuso::proj::d_ok=true;
    if(ok&&medir&&!reuso::memcpy_async(reuso::proj::stats_host,st,32,2,stream)&&!reuso::proj::event_record(reuso::proj::ev,stream)){reuso::proj::stats_pend=true;++reuso::proj::jobs_medidos;}
   }
   if(!ok&&reuso::proj::logs<12){++reuso::proj::logs;log("REUSO PROJ falha ao lancar rp_celulas: job sem reprojecao");}
  }else if(reuso::proj::ready&&reuso::proj::mv_job==reuso::maq.job&&reuso::proj::logs<12){
   ++reuso::proj::logs;char line[200]{};NRLOGF(line,sizeof line,"REUSO PROJ grade inesperada: fundo %dx%d celula %d px, rede %dx%d: sem reprojecao",proj_hw,proj_hh,f8,m.netw,m.neth);log(line);
  }
  if((reuso::maq.job&63u)==0){char line[260]{};NRLOGF(line,sizeof line,"REUSO PROJ job=%u reuso=%u medidos=%u warps=%u sem_mv=%u forcados=%u mov media=%.2f px frac=%.1f%% max=%.1f px div=%.1f%% (ok=%d) quadros k medio=%.2f max=%u",reuso::maq.job,reuso::maq.jobs_reuso,reuso::proj::jobs_medidos,reuso::proj::warps,reuso::proj::copias_sem_mv,reuso::maq.forcados,reuso::proj::ult_media,reuso::proj::ult_frac,reuso::proj::ult_max,reuso::proj::ult_div,reuso::proj::ult_ok?1:0,reuso::proj::k_n?double(reuso::proj::k_soma)/double(reuso::proj::k_n):0.,reuso::proj::k_max);log(line);}
 }
 if(reuso::maq.refrescos!=refrescos_antes&&reuso::maq.refrescos<=64){char line[120]{};NRLOGF(line,sizeof line,"REUSO REFRESCO job=%u (corte de camera / reset do FFX): rede inteira em 2 jobs",reuso::maq.job);log(line);}
 if(reuso::maq.pausas!=pausas_antes&&reuso::maq.pausas<=16){char line[120]{};NRLOGF(line,sizeof line,"REUSO PAUSA job=%u: rede inteira",reuso::maq.job);log(line);}
 if(reuso::prova&&entre_antes&&reuso::nomes_logados<400&&!(fronteira&&modo==8)){
  ++reuso::nomes_logados;
  char line[320]{};NRLOGF(line,sizeof line,"REUSO FAIXA job=%u %s %s",reuso::maq.job,acao==nrreuso::Acao::pular?"PULADO":acao==nrreuso::Acao::abortar_e_lancar?"ABORTO":"lancado",reuso::nome(f).c_str());log(line);
 }
 using nrreuso::Acao;
 switch(acao){
 case Acao::pular:return 0;
 case Acao::lancar_e_pular_depois:{
  lock.unlock();
  const int r=launch_normal(f,g,b,args,shared,stream);
  lock.lock();if(!r)reuso::maq.comecar_pulo();
  return r;}
 case Acao::abortar_e_lancar:{
  char line[256]{};NRLOGF(line,sizeof line,"REUSO ABORTO job=%u kernel fora da faixa conhecida: %s (cache invalidado)",reuso::maq.job,reuso::nome(f).c_str());log(line);
  break;}
 case Acao::guardar_e_lancar:
 case Acao::restaurar_e_lancar:{

  const char* p=static_cast<const char*>(args[0]);
  void* aux{};int h{},w{};std::memcpy(&aux,p+0x30,8);std::memcpy(&h,p+0x18,4);std::memcpy(&w,p+0x1c,4);
  const std::uint64_t bytes=(h>0&&w>0)?std::uint64_t(h)*std::uint64_t(w)*(64u<<reuso::janela)/2:0;
  if(!aux||!bytes){reuso::maq.cache_ok=false;log("REUSO modo 8 sem auxiliar valido: cache invalidado");break;}
  if(acao==Acao::guardar_e_lancar){
   bool ok=true;
   if(reuso::copia){
    if(reuso::cache_cap<bytes){void* novo{};if(reuso::malloc_(&novo,static_cast<std::size_t>(bytes))||!novo){ok=false;log("REUSO hipMalloc falhou");}else{reuso::cache=novo;reuso::cache_cap=bytes;}}
    if(ok&&reuso::memcpy_async(reuso::cache,aux,static_cast<std::size_t>(bytes),3,stream)){ok=false;log("REUSO copia aux->cache falhou");}
   }
   reuso::aux_guardado=aux;reuso::bytes_guardado=bytes;reuso::maq.cache_ok=ok;reuso::proj::q_ref=nrreusostate::quadro_cena;
   reuso::provar("cheio",aux,bytes,stream,false);
  }else{
   bool ok=bytes==reuso::bytes_guardado&&(reuso::copia||aux==reuso::aux_guardado);
   bool feito=false;

   const int groups=int((128u<<reuso::janela)/16u);
   if(ok&&reuso::proj::modo&&reuso::proj::ready&&reuso::proj::d_ok&&reuso::proj::mv_job==reuso::maq.job&&h/2>0&&w/2>0&&
      std::uint64_t(h/2)*std::uint64_t(w/2)*8u<=reuso::proj::d_cap&&std::uint64_t(h/2)*std::uint64_t(w/2)*16u*std::uint64_t(groups)==bytes){
    const void* src=reuso::cache;void* dst=aux;void* dd=reuso::proj::d;int hh=h/2,hw=w/2,gg=groups,bil=reuso::proj::modo==1?1:0;
    void* a[]{&src,&dst,&dd,&hh,&hw,&gg,&bil};
    if(!reuso::proj::mlaunch(reuso::proj::k_warp,unsigned(hw+63)/64u,unsigned(hh),unsigned(gg),64,1,1,0,stream,a,nullptr)){
     feito=true;++reuso::proj::warps;

     if(reuso::proj::cadeia&&reuso::maq.n>=3){if(reuso::memcpy_async(reuso::cache,aux,static_cast<std::size_t>(bytes),3,stream)){reuso::maq.cache_ok=false;log("REUSO PROJ copia aux->cache falhou");}else reuso::proj::q_ref=nrreusostate::quadro_cena;}
    }else if(reuso::proj::logs<12){++reuso::proj::logs;log("REUSO PROJ falha ao lancar rp_warp: copia simples");}
   }else if(reuso::proj::modo)++reuso::proj::copias_sem_mv;
   if(ok&&!feito&&reuso::copia&&reuso::memcpy_async(aux,reuso::cache,static_cast<std::size_t>(bytes),3,stream)){ok=false;log("REUSO copia cache->aux falhou");}
   if(!ok){reuso::maq.cache_ok=false;log("REUSO job de reuso com auxiliar diferente do guardado: cache invalidado");}
   reuso::provar("reuso",aux,bytes,stream,true);
  }
  if((reuso::maq.job&255u)==0&&acao==Acao::guardar_e_lancar){
   char line[200]{};NRLOGF(line,sizeof line,"REUSO jobs=%u reuso=%u pulados=%llu abortos=%u refrescos=%u pausas=%u",reuso::maq.job,reuso::maq.jobs_reuso,reuso::maq.pulados,reuso::maq.abortos,reuso::maq.refrescos,reuso::maq.pausas);log(line);
  }
  break;}
 default:break;
 }
 lock.unlock();
 return launch_normal(f,g,b,args,shared,stream);
}

inline void r1d_medir(const void* params,void* stream) noexcept {
 namespace M=r1d::mov;
 std::call_once(M::init,[]{
  const auto hip=GetModuleHandleW(L"amdhip64_7.dll");if(!hip){log("REUSO1D MOV FALLBACK amdhip64_7.dll ausente");return;}
  auto load=reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipModuleLoadData"));
  auto function=reinterpret_cast<int(*)(void**,void*,const char*)>(GetProcAddress(hip,"hipModuleGetFunction"));
  auto host_malloc=reinterpret_cast<int(*)(void**,std::size_t,unsigned)>(GetProcAddress(hip,"hipHostMalloc"));
  auto event_create=reinterpret_cast<int(*)(void**,unsigned)>(GetProcAddress(hip,"hipEventCreateWithFlags"));
  M::mlaunch=reinterpret_cast<M::ModuleLaunch>(GetProcAddress(hip,"hipModuleLaunchKernel"));
  M::memset_async=reinterpret_cast<decltype(M::memset_async)>(GetProcAddress(hip,"hipMemsetAsync"));
  M::event_record=reinterpret_cast<decltype(M::event_record)>(GetProcAddress(hip,"hipEventRecord"));
  M::event_query=reinterpret_cast<decltype(M::event_query)>(GetProcAddress(hip,"hipEventQuery"));
  M::malloc_=reinterpret_cast<decltype(M::malloc_)>(GetProcAddress(hip,"hipMalloc"));
  bool ok=load&&function&&host_malloc&&event_create&&M::mlaunch&&M::memset_async&&M::event_record&&M::event_query&&M::malloc_&&r1d::memcpy_async;
  ok=ok&&!load(&M::module,M::image.data())&&M::module&&!function(&M::k,M::module,"rp_celulas")&&M::k;
  void* h{};ok=ok&&!host_malloc(&h,64,0)&&h;if(ok){M::stats_host=static_cast<unsigned*>(h);std::memset(h,0,64);}
  ok=ok&&!M::malloc_(&M::stats_dev,64)&&M::stats_dev&&!event_create(&M::ev,2u)&&M::ev;
  M::ready=ok;log(ok?"REUSO1D MOV READY (rp_celulas por job, celulas de 16 px)":"REUSO1D MOV FALLBACK modulo/API recusado: reuso 1D sem corte por movimento");
 });
 if(!M::ready)return;
 M::colher();
 if(M::pend){++M::em_voo;return;}
 const char* p=static_cast<const char*>(params);reuso::proj::Mv m{};
 std::memcpy(&m.mv,p+0x10,8);std::memcpy(&m.pitch,p+0x18,4);std::memcpy(&m.fmt,p+0x1c,4);std::memcpy(&m.mvw,p+0x20,4);std::memcpy(&m.mvh,p+0x24,4);
 std::memcpy(&m.sx,p+0x58,4);std::memcpy(&m.sy,p+0x5c,4);std::memcpy(&m.neth,p+0x68,4);std::memcpy(&m.netw,p+0x6c,4);m.dep=nullptr;
 const int bpp=m.fmt==1||m.fmt==2?8:4;
 const bool ok=m.mv&&m.fmt>=0&&m.fmt<=2&&m.mvw>0&&m.mvh>0&&m.mvw<=16384&&m.mvh<=16384&&m.pitch>=m.mvw*bpp&&m.netw>0&&m.neth>0&&m.netw<=16384&&m.neth<=16384&&std::isfinite(m.sx)&&std::isfinite(m.sy);
 if(!ok){++M::sem_mv;return;}
 int f=16,hh=(m.neth+15)/16,hw=(m.netw+15)/16,modo=0;float lim=M::cfg.limiar_px,dv=2.f;
 const std::size_t need=std::size_t(hh)*std::size_t(hw)*8u;
 if(M::d_cap<need){void* nd{};if(M::malloc_(&nd,need)||!nd)return;M::d=nd;M::d_cap=need;}
 void* dd=M::d;void* st=M::stats_dev;void* a[]{&m,&hh,&hw,&f,&modo,&lim,&dv,&dd,&st};
 if(M::memset_async(st,0,32,stream)||M::mlaunch(M::k,unsigned(hw+63)/64u,unsigned(hh),1,64,1,1,0,stream,a,nullptr))return;
 if(r1d::memcpy_async(M::stats_host,st,32,2,stream)||M::event_record(M::ev,stream))return;
 M::pend=true;
}

namespace streamq {
inline int cada=0;
inline int(*query)(void*){};
inline std::atomic<unsigned long long> lancamentos{0},consultas{0};
inline void configure(const std::wstring& ini) noexcept {
 cada=ini.empty()?0:std::clamp<int>(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralStreamQuery",0,ini.c_str())),-1,4096);
 if(!cada)return;
 const auto hip=GetModuleHandleW(L"amdhip64_7.dll");
 query=hip?reinterpret_cast<int(*)(void*)>(GetProcAddress(hip,"hipStreamQuery")):nullptr;
 if(!query){cada=0;log("STREAMQ FALLBACK hipStreamQuery ausente");return;}
 char l[128]{};NRLOGF(l,sizeof l,"STREAMQ pedido: NeuralStreamQuery=%d (%s)",cada,cada<0?"depois do k_export":"a cada N lancamentos");log(l);
}
inline void depois(const void* f,void* stream,const char* image) noexcept {
 const auto n=lancamentos.fetch_add(1,std::memory_order_relaxed)+1;
 if(cada>0?(n%unsigned(cada))!=0:(!image||f!=image+0x62230))return;
 query(stream);
 const auto c=consultas.fetch_add(1,std::memory_order_relaxed)+1;
 if((c&(c-1))==0&&c>=64){char l[128]{};NRLOGF(l,sizeof l,"STREAMQ consultas=%llu lancamentos=%llu",static_cast<unsigned long long>(c),static_cast<unsigned long long>(n));log(l);}
}
}
inline int launch_r6(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream);
inline int launch_corpo(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream);
inline int launch_handshake(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream){return launch_corpo(f,g,b,args,shared,stream);}
inline int launch(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {
 if(rjsync::ligado.load(std::memory_order_relaxed)){
  const char* img=installed_image.load();
  if(img&&(f==img+0x62260||f==img+0x62268||f==img+0x62270)){
   int e{};{std::lock_guard<std::recursive_mutex> l(rjsync::trava());e=launch_handshake(f,g,b,args,shared,stream);}
   if(f==img+0x62270)rjsync::flag_set_n.fetch_add(1,std::memory_order_release);
   return e;
  }
 }
 return launch_corpo(f,g,b,args,shared,stream);
}
inline int launch_corpo(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream) {

 if(nrcena::quer_mv.load(std::memory_order_relaxed)&&nrreusostate::cena&&args&&args[0]&&f==installed_image.load()+0x62238)nrcena::guardar(args[0]);
 const bool medir=r1d::mov::on&&args&&args[0]&&f==installed_image.load()+0x62238;
 const int e=launch_reuso(f,g,b,args,shared,stream);
 if(medir&&!e)r1d_medir(args[0],stream);
 if(streamq::cada&&!e)streamq::depois(f,stream,installed_image.load());
 return e;
}
inline int launch_r6(const void* f,Dim g,Dim b,void** args,std::size_t shared,void* stream){return launch(f,g,b,args,shared,stream);}

namespace hostcopy {
inline constexpr unsigned slot_copy=0x86be8u;
using Copy=int(*)(void*,const void*,std::size_t,int);
inline Copy native{};
inline char* base{};
inline std::mutex mutex;
struct Site {std::uintptr_t rva{};unsigned long long calls{},bytes{},ticks{};};
inline std::array<Site,8> sites{};inline unsigned long long total_calls{};inline long long frequency{};
inline void report_locked() noexcept {
 char line[768]{};int n=NRLOGF(line,sizeof line,"HOSTCOPY calls=%llu",total_calls);
 for(const auto& s:sites)if(s.calls&&n>0&&n<static_cast<int>(sizeof line))
  n+=NRLOGF(line+n,sizeof line-n," [rva=0x%llx n=%llu bytes=%llu cpu_ms=%.3f]",static_cast<unsigned long long>(s.rva),s.calls,s.bytes,frequency?1000.0*double(s.ticks)/double(frequency):0.0);
 log(line);
}
inline int hook(void* dst,const void* src,std::size_t bytes,int kind) noexcept {
 const auto ret=reinterpret_cast<char*>(_ReturnAddress());
 LARGE_INTEGER a{},b{};QueryPerformanceCounter(&a);
 const int result=native(dst,src,bytes,kind);
 QueryPerformanceCounter(&b);
 std::lock_guard<std::mutex> lock(mutex);
 const auto rva=static_cast<std::uintptr_t>(ret-base);
 Site* s=nullptr;for(auto& x:sites)if(x.calls&&x.rva==rva){s=&x;break;}
 if(!s)for(auto& x:sites)if(!x.calls){s=&x;s->rva=rva;break;}
 if(s){++s->calls;s->bytes+=bytes;s->ticks+=static_cast<unsigned long long>(b.QuadPart-a.QuadPart);}
 if((++total_calls&255u)==0)report_locked();
 return result;
}
inline void install(char* image,const std::wstring& ini) noexcept {
 if(ini.empty()||NRCFG_INT(L"DlssNrOnAmd",L"NeuralHostCopyLog",0,ini.c_str())!=1)return;
 const auto hip=GetModuleHandleW(L"amdhip64_7.dll");
 const auto expected=hip?GetProcAddress(hip,"hipMemcpy"):nullptr;
 const auto slot=reinterpret_cast<void**>(image+slot_copy);
 if(!expected||*slot!=reinterpret_cast<void*>(expected)){log("HOSTCOPY REJECTED hipMemcpy slot desconhecido");return;}
 LARGE_INTEGER f{};QueryPerformanceFrequency(&f);frequency=f.QuadPart;
 native=reinterpret_cast<Copy>(expected);base=image;
 DWORD old{},unused{};if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old)){log("HOSTCOPY REJECTED VirtualProtect");return;}
 InterlockedExchangePointer(reinterpret_cast<void* volatile*>(slot),reinterpret_cast<void*>(&hook));
 VirtualProtect(slot,sizeof(void*),old,&unused);
 log("HOSTCOPY INSTALLED (diagnostico, NeuralHostCopyLog=1)");
}
}
inline bool install(char* image) noexcept {
 try {
  std::lock_guard<std::mutex> lock(install_mutex);
  if(installed_image.load())return installed_image.load()==image;

  static_assert(std::string_view(nrliveimage::file_sha256)=="28609e75a8917e47f370a67745a359d6605bcf6f1eb10153c45e94fa80e040dc");
  wchar_t self_path[32768]{},proxy_path[32768]{};HMODULE self{};
  if(!image||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&install),&self)||!GetModuleFileNameW(self,self_path,32768)||!GetModuleFileNameW(reinterpret_cast<HMODULE>(image),proxy_path,32768))return false;
  folder=std::filesystem::path(self_path).parent_path();
  const auto ini=nrinipath::resolve(image,reinterpret_cast<const void*>(&install));
  if(!ini.empty())nrcapture::configure(ini,folder);
  if(!ini.empty())nrtimeline::configure(ini,folder);
  hostgap::configure(ini);
  streamq::configure(ini);
  reuso::configure(ini);
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
  c1d43::requested=c1d43::requested&&!nrcapture::requested&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralKernelTrace",0,ini.c_str())==0&&
   NRCFG_INT(L"DlssNrOnAmd",L"NeuralContractTrace",0,ini.c_str())==0&&!stages_env;
  vit::requested=mh::requested&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegVit",1,ini.c_str())==1&&!nrcapture::requested&&
   NRCFG_INT(L"DlssNrOnAmd",L"NeuralKernelTrace",0,ini.c_str())==0&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralContractTrace",0,ini.c_str())==0&&
   !stages_env;

  mh::chain_enabled=nrmhpolicy::chain_allowed(mh::requested,NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwinMhChain",1,ini.c_str())==1,
   NRCFG_INT(L"DlssNrOnAmd",L"NeuralKernelTrace",0,ini.c_str())!=0,NRCFG_INT(L"DlssNrOnAmd",L"NeuralContractTrace",0,ini.c_str())!=0,stages_env);

  dispatch.chain=nrmhpolicy::chain_allowed(requested&&layout_requested,NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32Chain",1,ini.c_str())==1,
   NRCFG_INT(L"DlssNrOnAmd",L"NeuralKernelTrace",0,ini.c_str())!=0,NRCFG_INT(L"DlssNrOnAmd",L"NeuralContractTrace",0,ini.c_str())!=0,stages_env)&&!nrcapture::requested;
  if(GetEnvironmentVariableW(L"DLSSNR_NO_REG",flag,2)||GetEnvironmentVariableW(L"DLSSNR_SLOW_PREPOST",flag,2))requested=mh::requested=mh::chain_enabled=vit::requested=dispatch.chain=c1d43::requested=false;
  std::vector<unsigned char> proxy;
  if(!read_exact(proxy_path,nrliveimage::file_size,proxy)||!nrliveimage::inspect_disk_snapshot(proxy.data(),proxy.size())){log("REJECTED proxy disk digest");return false;}
  if(requested&&(!read_exact(folder/L"nr_reg_swin32.hsaco",nrregcontract::module_bytes,module_image)||!nrliveimage::detail::sha256_matches(module_image.data(),module_image.size(),nrregcontract::sha256))){requested=false;module_image.clear();log("FALLBACK module absent or digest mismatch");}
  v041_requested=requested&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32V041",1,ini.c_str())==1;
  v041_mask=static_cast<unsigned>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32V041Mask",255,ini.c_str()))&0xffu;
  if(v041_requested&&(!read_exact(folder/L"nr_reg_swin_mh.hsaco",nrmhcontract::module_bytes,v041_image)||!nrliveimage::detail::sha256_matches(v041_image.data(),v041_image.size(),nrmhcontract::sha256))){v041_requested=false;v041_image.clear();log("SW32 V041 FALLBACK nr_reg_swin_mh.hsaco absent or digest mismatch");}
  fast::requested=nrmhcontract::fast_available&&(requested||mh::requested)&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegFast",1,ini.c_str())==1;
  fast::mask=static_cast<unsigned>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegFastMask",7,ini.c_str()))&7u;
  fast::sw32_mask=static_cast<unsigned>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegFastSw32Mask",255,ini.c_str()))&0xffu;
  const bool want43=fast::requested&&nrmhcontract::fast43_available&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegFastVersao",43,ini.c_str())==43;
  bool have43=false;
  if(want43){

   static_assert(nr1080::contract::fast43_mhpeep_bytes==nrmhcontract::fast43_module_bytes,"mh peep muda o tamanho do fast43");

   bool ovfl43=false;
   if(!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegMhOvfl",0,ini.c_str())==1){
    ovfl43=read_exact(folder/L"nr_reg_fast43_ovfl.hsaco",nrmhcontract::fast43_module_bytes,fast::image43)&&nrliveimage::detail::sha256_matches(fast::image43.data(),fast::image43.size(),nr1080::contract::fast43_mhovfl_sha256);
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
     if(rb->bytes&&read_exact(folder/rb->file,rb->bytes,mh::r55_image[w])&&nrliveimage::detail::sha256_matches(mh::r55_image[w].data(),mh::r55_image[w].size(),rb->sha256)){
      mh::r55_borda[w]=true;NRLOGF(line,sizeof line,"R55 %s pedido (%ls: janelas parciais so com as telhas validas)",rb->tag,rb->file);log(line);continue;}
     mh::r55_image[w].clear();NRLOGF(line,sizeof line,"R55 %s FALLBACK %ls absent or digest mismatch; fica o %ls",rb->tag,rb->file,r->file);log(line);
    }
    if(!have43||!r->bytes||!read_exact(folder/r->file,r->bytes,mh::r55_image[w])||!nrliveimage::detail::sha256_matches(mh::r55_image[w].data(),mh::r55_image[w].size(),r->sha256)){
     mh::r55_image[w].clear();NRLOGF(line,sizeof line,"R55 %s FALLBACK %ls absent or digest mismatch; fica o Fast",r->tag,r->file);}
    else NRLOGF(line,sizeof line,"R55 %s pedido (%ls)",r->tag,r->file);
    log(line);
   }

   mhp::image.clear();mhp::requested=false;
   mhp::modo=ini.empty()?0:int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralR55MhPersist",0,ini.c_str()));

   mhp::mascara=ini.empty()?4u:static_cast<unsigned>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralR55MhPersistMask",4,ini.c_str()))&7u;
   mhp::monitor=ini.empty()?1:int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralR55MhPersistMonitor",1,ini.c_str()));
   mhp::grade_max=ini.empty()?0u:static_cast<unsigned>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralR55MhPersistGrade",0,ini.c_str()));
   if(mhp::modo==1||mhp::modo==2){
    const auto* m=nr1080::r55_mhp();
    if(nrtimeline::requested||nrcapture::requested)log("R55 MHP FALLBACK cronometro/captura de kernel ligado; um lancamento por bloco");
    else if(!have43||!m->bytes||!read_exact(folder/m->file,m->bytes,mhp::image)||!nrliveimage::detail::sha256_matches(mhp::image.data(),mhp::image.size(),m->sha256)){
     mhp::image.clear();log("R55 MHP FALLBACK nr_r55_mhp.hsaco absent or digest mismatch; um lancamento por bloco");}
    else{mhp::requested=true;log(mhp::modo==2?"R55 MHP pedido (nr_r55_mhp.hsaco, NeuralR55MhPersist=2: todo nivel)":"R55 MHP pedido (nr_r55_mhp.hsaco, NeuralR55MhPersist=1: so nas formas medidas com ganho)");}
   }
  }

  fast::image51.clear();
  if(have43&&nrmhcontract::fast51_available&&!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegV051",0,ini.c_str())==1){
   fast::v051_sw32_mask=static_cast<unsigned>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegV051Sw32Mask",0x5F,ini.c_str()))&0xffu;
   fast::v051_mh_mask=static_cast<unsigned>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegV051MhMask",0x30,ini.c_str()))&0xffu;
   fast::v051_1d_mask=static_cast<unsigned>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegV0511dMask",0,ini.c_str()))&3u;
   if(read_exact(folder/L"nr_reg_fast51.hsaco",nrmhcontract::fast51_module_bytes,fast::image51)&&nrliveimage::detail::sha256_matches(fast::image51.data(),fast::image51.size(),nrmhcontract::fast51_sha256)){
    char line[128]{};NRLOGF(line,sizeof line,"FAST51 v0.5.1 pedido (NeuralRegV051=1) sw32_mask=0x%02X mh_mask=0x%02X",fast::v051_sw32_mask,fast::v051_mh_mask);log(line);}
   else {fast::image51.clear();log("FAST51 FALLBACK nr_reg_fast51.hsaco absent or digest mismatch; tudo no v0.4.3");}
  }

  {const int p=ini.empty()?0:int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRedePrecedencia",0,ini.c_str()));nr1080::precedencia=(p==1||p==2)?p:0;
   if(nr1080::precedencia){char line[96]{};NRLOGF(line,sizeof line,"PRECEDENCIA R55 x V051 = %s (NeuralRedePrecedencia)",p==1?"R55 sempre":"V051 sempre");log(line);}}
  if(fast::requested&&(!read_exact(folder/L"nr_reg_fast.hsaco",nrmhcontract::fast_module_bytes,fast::image)||!nrliveimage::detail::sha256_matches(fast::image.data(),fast::image.size(),nrmhcontract::fast_sha256))){
   fast::image.clear();
   if(!have43){fast::requested=false;log("FAST FALLBACK nr_reg_fast.hsaco absent or digest mismatch; tudo no v0.4.1");}
  }
  if(fast::requested){char line[120]{};NRLOGF(line,sizeof line,"FAST %s pedido mask=%u sw32_mask=%u",have43?"v0.4.3":"v0.4.2",fast::mask,fast::sw32_mask);log(line);}
  {
   int key=!ini.empty()?int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32X",0,ini.c_str())):0;

   nr1080::r55_sw32=false;
   if(!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralR55Sw32",0,ini.c_str())==1){
    const auto* r=nr1080::sw32x_r55();std::vector<unsigned char> t;
    if(!read_exact(folder/r->file,r->bytes,t)||!nrliveimage::detail::sha256_matches(t.data(),t.size(),r->sha256))log("R55 SW32 FALLBACK nr_r55_sw32.hsaco absent or digest mismatch; swin32 fica como antes");
    else{nr1080::r55_sw32=true;if(key==1)key=2;char line[120]{};NRLOGF(line,sizeof line,"R55 SW32 pedido: variante E = %s; NeuralRegSwin32X efetivo=%d",r->tag,key);log(line);}}

   nr1080::sw32x_ovfl=!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32Ovfl",0,ini.c_str())==1;
   if(nr1080::sw32x_ovfl){const auto* ao=nr1080::sw32x_ao();std::vector<unsigned char> t;
    if(!read_exact(folder/ao->file,ao->bytes,t)||!nrliveimage::detail::sha256_matches(t.data(),t.size(),ao->sha256)){nr1080::sw32x_ovfl=false;log("SW32X OVFL FALLBACK nr_reg_swin32x_ao.hsaco absent or digest mismatch; a variante A fica sem FP16_OVFL");}
    else log("SW32X OVFL pedido: a variante A usa o modulo A + FP16_OVFL");}
   sw32x::variant=nr1080::sw32x(key);sw32x::image.clear();
   nrsw32x::escolha.store(key==1||key==2?key:0,std::memory_order_relaxed);

   for(int v=1;v<=2;++v){sw32x::images[v].clear();const auto* var=nr1080::sw32x(v);
    if(var&&requested&&have43&&(fast::mask&2u)&&read_exact(folder/var->file,var->bytes,sw32x::images[v])&&nrliveimage::detail::sha256_matches(sw32x::images[v].data(),sw32x::images[v].size(),var->sha256)){
     char line[160]{};NRLOGF(line,sizeof line,"SW32X disponivel no menu: %s",var->tag);log(line);}
    else sw32x::images[v].clear();}
   if(sw32x::variant&&!(requested&&have43&&(fast::mask&2u))){sw32x::variant=nullptr;log("SW32X FALLBACK swin32 Fast v0.4.3 nao pedido/ausente; modulo novo nao usado");}
   else if(sw32x::variant&&(!read_exact(folder/sw32x::variant->file,sw32x::variant->bytes,sw32x::image)||!nrliveimage::detail::sha256_matches(sw32x::image.data(),sw32x::image.size(),sw32x::variant->sha256))){
    char line[160]{};NRLOGF(line,sizeof line,"SW32X FALLBACK %ls absent or digest mismatch; swin32 fica no Fast v0.4.3",sw32x::variant->file);log(line);
    sw32x::variant=nullptr;sw32x::image.clear();
   }
   else if(sw32x::variant){char line[160]{};NRLOGF(line,sizeof line,"SW32X pedido NeuralRegSwin32X=%d %s (%ls)",key,sw32x::variant->tag,sw32x::variant->file);log(line);}
   else if(key)log("SW32X valor desconhecido em NeuralRegSwin32X; swin32 fica no Fast v0.4.3");
  }
  if(requested&&layout_requested&&(!read_exact(folder/L"nr_reg_layout.hsaco",nrreglayout::module_bytes,layout_image)||!nrliveimage::detail::sha256_matches(layout_image.data(),layout_image.size(),nrreglayout::sha256))){requested=false;layout_image.clear();log("FALLBACK layout module absent or digest mismatch");}
  dispatch.chain=dispatch.chain&&requested;
  dispatch.park_pretin=dispatch.chain&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32PreTin",1,ini.c_str())==1;
  dispatch.drop_restore=dispatch.chain&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32DropRestore",1,ini.c_str())==1;
  dispatch.park_stage=dispatch.chain&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwin32Park",1,ini.c_str())==1;
  if(mh::requested&&(!read_exact(folder/L"nr_reg_swin_mh.hsaco",nrmhcontract::module_bytes,mh::image)||!nrliveimage::detail::sha256_matches(mh::image.data(),mh::image.size(),nrmhcontract::sha256))){mh::requested=false;mh::image.clear();log("MH FALLBACK module absent or digest mismatch");}
  if(mh::requested&&(!read_exact(folder/L"nr_mh_layout.hsaco",nrmhcontract::layout_bytes,mh::layout_image)||!nrliveimage::detail::sha256_matches(mh::layout_image.data(),mh::layout_image.size(),nrmhcontract::layout_sha256))){mh::requested=false;mh::image.clear();mh::layout_image.clear();log("MH FALLBACK layout module absent or digest mismatch");}
  c1d43::requested=c1d43::requested&&mh::requested;
  if(c1d43::requested&&(!read_exact(folder/L"nr_reg_1d43.hsaco",nr1d43contract::module_bytes,c1d43::image)||!nrliveimage::detail::sha256_matches(c1d43::image.data(),c1d43::image.size(),nr1d43contract::sha256))){c1d43::requested=false;c1d43::image.clear();log("1D43 FALLBACK nr_reg_1d43.hsaco absent or digest mismatch");}
  c1d::requested=c1d::requested&&mh::requested;
  if(c1d::requested&&(!read_exact(folder/L"nr_reg_v040.hsaco",nrmhcontract::v040_module_bytes,c1d::image)||!nrliveimage::detail::sha256_matches(c1d::image.data(),c1d::image.size(),nrmhcontract::v040_sha256))){c1d::requested=false;c1d::image.clear();log("1D FALLBACK nr_reg_v040.hsaco absent or digest mismatch");}
  mh::chain_enabled&=mh::requested;
  mh::drop_restore=nrmhpolicy::drop_restore_allowed(mh::chain_enabled&&!nrcapture::requested,NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwinMhDropRestore",1,ini.c_str())==1);
  mh::chain.park_enabled=mh::chain_enabled&&!nrcapture::requested&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwinMhPark",1,ini.c_str())==1;
  mh::chain.decoder_enabled=NRCFG_INT(L"DlssNrOnAmd",L"NeuralRegSwinMhChainDecoder",1,ini.c_str())==1;
  vit::requested=vit::requested&&mh::requested;
  if(vit::requested&&(!read_exact(folder/L"nr_vit_layout.hsaco",nrmhcontract::vit_layout_bytes,vit::layout_image)||!nrliveimage::detail::sha256_matches(vit::layout_image.data(),vit::layout_image.size(),nrmhcontract::vit_layout_sha256))){vit::requested=false;vit::layout_image.clear();log("VIT FALLBACK layout module absent or digest mismatch");}
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
   if(rw1d::requested&&(!read_exact(folder/L"nr_1d_rw.hsaco",rw1d_bytes,rw1d::image)||!(nrliveimage::detail::sha256_matches(rw1d::image.data(),rw1d::image.size(),rw1d_sha256)||(rw1d_sha256_alt&&nrliveimage::detail::sha256_matches(rw1d::image.data(),rw1d::image.size(),rw1d_sha256_alt))))){rw1d::requested=false;rw1d::image.clear();log("1D RW FALLBACK nr_1d_rw.hsaco absent or digest mismatch");}
   r55d1::requested=c1d43::requested&&c1d43::fast&&key(L"NeuralR55D1",0)==1;
   if(r55d1::requested&&(!read_exact(folder/L"nr_r55_1d.hsaco",r55_1d_bytes,r55d1::image)||!nrliveimage::detail::sha256_matches(r55d1::image.data(),r55d1::image.size(),r55_1d_sha256))){r55d1::requested=false;r55d1::image.clear();log("R55 1D FALLBACK nr_r55_1d.hsaco absent or digest mismatch; qkv do Daniel");}
   else if(r55d1::requested)log("R55 1D pedido: NeuralR55D1=1 (qkv_fast -> k1d_qkv_*)");
   vitx::requested=vit::requested&&key(L"NeuralRegVitx",1)!=0;
   vitx::mask=static_cast<unsigned>(key(L"NeuralRegVitxMask",7))&7u;
   if(vitx::requested){
    if(!read_exact(folder/L"nr_vitx_rel.hsaco",vitx_bytes,vitx::image)||!(nrliveimage::detail::sha256_matches(vitx::image.data(),vitx::image.size(),vitx_sha256)||(vitx_sha256_alt&&nrliveimage::detail::sha256_matches(vitx::image.data(),vitx::image.size(),vitx_sha256_alt)))){vitx::image.clear();log("VITX FALLBACK nr_vitx_rel.hsaco absent or digest mismatch (conv/ffwd do Daniel)");}

    if(read_exact(folder/L"nr_vitx_attn.hsaco",vitx_attn_nopulo_bytes,vitx::attn_image)&&nrliveimage::detail::sha256_matches(vitx::attn_image.data(),vitx::attn_image.size(),vitx_attn_nopulo_sha256))log("VITX attn2 sem pulo (7CC6C197)");
    else if(read_exact(folder/L"nr_vitx_attn.hsaco",vitx_attn_bytes,vitx::attn_image)&&nrliveimage::detail::sha256_matches(vitx::attn_image.data(),vitx::attn_image.size(),vitx_attn_sha256))log("VITX attn2 com pulo (61C41528)");
    else {vitx::attn_image.clear();log("VITX FALLBACK nr_vitx_attn.hsaco absent or digest mismatch (attn2 do Daniel)");}
    vitx::requested=!vitx::image.empty()||!vitx::attn_image.empty();
   }
   decup::requested=vit::requested&&have43&&key(L"NeuralRegDecup",1)!=0;
   cauda::requested=mh::requested&&key(L"NeuralRegFinalHead",1)!=0&&!nrcapture::requested;
   if(cauda::requested&&(!read_exact(folder/L"nr_cauda.hsaco",cauda_bytes,cauda::image)||!(nrliveimage::detail::sha256_matches(cauda::image.data(),cauda::image.size(),cauda_sha256)||(cauda_sha256_alt&&nrliveimage::detail::sha256_matches(cauda::image.data(),cauda::image.size(),cauda_sha256_alt))))){cauda::requested=false;cauda::image.clear();log("FINALHEAD FALLBACK nr_cauda.hsaco absent or digest mismatch");}
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
   float t{};if(nrinijogo::ler_float_em(ini.c_str(),L"NeuralHistMovMax",t,64)&&std::isfinite(t))nrhistmov::teto=std::clamp(t,1.f,16.f);
   if(nrhistmov::modo||nrhistmov::log_on){char line[160]{};NRLOGF(line,sizeof line,"HISTMOV pedido: modo=%d teto=%.1f log=%d",nrhistmov::modo,double(nrhistmov::teto),nrhistmov::log_on?1:0);log(line);}
  }

  {
   const int v=!ini.empty()?int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralV05Tokens",0,ini.c_str())):0;
   if(!nrv05::menu_escreveu.load())nrv05::ligado.store(v==1);
   v05::malloc_fn=reinterpret_cast<int(*)(void**,std::size_t)>(GetProcAddress(hip,"hipMalloc"));
   v05::memset_async=reinterpret_cast<int(*)(void*,int,std::size_t,void*)>(GetProcAddress(hip,"hipMemsetAsync"));
   v05::range=reinterpret_cast<int(*)(void**,std::size_t*,void*)>(GetProcAddress(hip,"hipMemGetAddressRange"));
   v05::free_fn=reinterpret_cast<int(*)(void*)>(GetProcAddress(hip,"hipFree"));
   v05::installed=v05::malloc_fn&&v05::memset_async&&!nrcapture::requested;
   r1d::memcpy_async=reinterpret_cast<decltype(r1d::memcpy_async)>(GetProcAddress(hip,"hipMemcpyAsync"));
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
     if(!read_exact(folder/L"nr_reuso_proj.hsaco",nrreusoproj::module_bytes,r1d::mov::image)||!nrliveimage::detail::sha256_matches(r1d::mov::image.data(),r1d::mov::image.size(),nrreusoproj::module_sha256)){
      r1d::mov::image.clear();log("REUSO1D MOV FALLBACK nr_reuso_proj.hsaco ausente ou digest diferente: reuso 1D sem corte por movimento");}
     else{r1d::mov::on=true;char line[200]{};NRLOGF(line,sizeof line,"REUSO1D MOV pedido: limiar=%.2f px fracao=%.1f%% saida=%.2f calmos=%u",double(r1d::mov::cfg.limiar_px),double(r1d::mov::cfg.fracao),double(r1d::mov::cfg.saida),r1d::mov::cfg.calmos);log(line);}
    }
   }
   char line[160]{};NRLOGF(line,sizeof line,"V05 %s NeuralV05Tokens=%d (menu ao vivo; grade do nivel global da v0.5.0)",v05::installed?"INSTALLED":"OFF (captura ou API)",v);log(line);
  }
  vit::tokens[0]=image+0x621a0;vit::tokens[1]=image+0x621a8;vit::tokens[2]=image+0x621b0;vit::tokens[3]=image+0x62200;
  InterlockedExchangePointer(reinterpret_cast<void* volatile*>(slot),reinterpret_cast<void*>(&launch));
  VirtualProtect(slot,sizeof(void*),old,&unused);register_hook(reinterpret_cast<void*>(&launch));installed_image.store(image);
  hostcopy::install(image,ini);
  log(requested?(dispatch.chain?(dispatch.park_pretin?(dispatch.drop_restore?(dispatch.park_stage?"INSTALLED awaiting first Swin32 launch chain=on pre_tin=reg drop_restore=on park=on":"INSTALLED awaiting first Swin32 launch chain=on pre_tin=reg drop_restore=on"):"INSTALLED awaiting first Swin32 launch chain=on pre_tin=reg"):"INSTALLED awaiting first Swin32 launch chain=on"):"INSTALLED awaiting first Swin32 launch chain=off"):"INSTALLED Swin32 original path (disabled or module rejected)");
  log(c1d::requested?"1D INSTALLED awaiting first contract2 launch":"1D INSTALLED original path (NeuralReg1d off or mh module rejected)");
  log(c1d43::requested?(c1d43::fast?"1D43 INSTALLED awaiting first k_repack (Fast)":"1D43 INSTALLED awaiting first k_repack (Reference)"):"1D43 INSTALLED off (NeuralReg1d43=0, capture/trace, or module rejected)");
  log(vit::requested?"VIT INSTALLED awaiting first vit512 launch":"VIT INSTALLED original path (NeuralRegVit off, capture/trace on, or module rejected)");
  log(mh::requested?(mh::chain_enabled?(mh::drop_restore?(mh::chain.park_enabled?"MH INSTALLED awaiting first Swin64/128/256 launch chain=on drop_restore=on park=on":"MH INSTALLED awaiting first Swin64/128/256 launch chain=on drop_restore=on"):"MH INSTALLED awaiting first Swin64/128/256 launch chain=on"):"MH INSTALLED awaiting first Swin64/128/256 launch chain=off"):"MH INSTALLED original path (NeuralRegSwinMh off or module rejected)");
  return true;
 }catch(...){log("REJECTED install exception");return false;}
}
}
