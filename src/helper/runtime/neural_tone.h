#pragma once
#include "nr_publico.h"
#include <atomic>
#include "native_ini_path.h"
#include "pasta_helper.h"
#include "modulos_pin.h"
#include "neural_tone_state.h"
#include "../source/implementation/live-transition-image/live_image.h"
#include "../../v033-port/kernels/reg_swin32_contract.h"
#include "../../v033-port/tone_export_contract.h"
#include "../../v040-port/kernels/reg_swin_mh_contract.h"
#include "rede1080_policy.h"
#include <windows.h>
#include "log_fila.h"
#include <hip/hip_runtime_api.h>
#include "hip_api.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <mutex>
#include <cstring>
#include <cmath>
#include <cstdio>
namespace nrneuraltone {
inline bool ready=false,failed=false;
inline unsigned curve=0;inline float lift=0;

inline bool v43=false;inline int fastpow=1;

inline bool export_fast=false;inline std::vector<unsigned char> cauda_bytes;inline void* cauda_module{};inline void* export_r0_kernel{};
inline char* image{};inline int device=-1;
inline std::filesystem::path folder;
inline std::once_flag init;
inline std::vector<unsigned char> module_bytes,export_bytes;
inline void* module{},*export_module{},*import_kernel{},*reproject_kernel{},*export_kernel{},*full_export_kernel{};
using ModuleLaunch=int(*)(void*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,void*,void**,void**);
inline ModuleLaunch launch{};inline int(*get_device)(int*){};
inline bool enabled() noexcept{return requested;}
inline void log(const char* s)noexcept{
(void)s;
}
template<class T>inline T get(const void* p,size_t offset){T v{};std::memcpy(&v,static_cast<const char*>(p)+offset,sizeof v);return v;}

inline bool read_module(const std::filesystem::path& p,size_t size,const unsigned char* hash,std::vector<unsigned char>& b,const unsigned char* alt=nullptr){
 return nrmodpin::ler_conferido(p,size,hash,b,alt);
}
inline bool install(char* base)noexcept{
 static std::mutex installation;
 std::lock_guard<std::mutex> lock(installation);
 if(image)return image==base;
 try {image=base;
  if(nrpasta::helper().empty())return false;
  folder=nrpasta::helper();const auto ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&install));
  wchar_t selected[32]{},number[32]{};NRCFG_STR(L"DlssNrOnAmd",L"ToneCurve",L"legacy",selected,32,ini.c_str());
  if(!_wcsicmp(selected,L"legacy"))return true;
  if(_wcsicmp(selected,L"reinhard")&&_wcsicmp(selected,L"aces")){log("FALLBACK invalid ToneCurve");return true;}
  curve=!_wcsicmp(selected,L"aces");NRCFG_STR(L"DlssNrOnAmd",L"ToneLift",L"0",number,32,ini.c_str());
  wchar_t* end{};lift=std::wcstof(number,&end);if(end==number||*end||!std::isfinite(lift)||lift<0||lift>.25f){log("FALLBACK invalid ToneLift");return true;}
  const int versao=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralToneVersao",43,ini.c_str()));
  fastpow=NRCFG_INT(L"DlssNrOnAmd",L"NeuralToneFastPow",1,ini.c_str())?1:0;
  v43=versao==43&&nrmhcontract::fast43_available&&read_module(folder/L"nr_reg_fast43.hsaco",nrmhcontract::fast43_module_bytes,nrmhcontract::fast43_sha256,module_bytes,nr1080::contract::fast43_mhpeep_sha256);
  if(versao==43&&!v43)log("FALLBACK nr_reg_fast43.hsaco absent or changed; tone kernels v0.3.3");
  if(!v43&&!read_module(folder/L"nr_reg_swin32.hsaco",nrregcontract::module_bytes,nrregcontract::sha256,module_bytes)||
     !read_module(folder/L"nr_tone_export.hsaco",nrtonemodule::bytes,nrtonemodule::sha256,export_bytes)){log("FALLBACK missing or changed tone module");return true;}
  requested=true;log("REQUESTED input tone transform; residual export translated to legacy compositor; restart required");
  multipass_ok=curve==0&&lift==0.f&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralToneMultipass",1,ini.c_str())!=0;
  if(multipass_ok)log("MULTIPASS allowed: reinhard lift 0 = identity export");
  export_fast=curve==0&&lift==0.f&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralToneExportFast",1,ini.c_str())!=0;
  if(export_fast&&!read_module(folder/L"nr_cauda.hsaco",nr1080::contract::cauda_bytes,nr1080::contract::cauda_sha256,cauda_bytes,nr1080::contract::cauda_sha256_alt)){export_fast=false;cauda_bytes.clear();log("EXPORT_R0 FALLBACK nr_cauda.hsaco absent or changed; nr_tone_export");}
  return true;
 }catch(...){requested=false;return false;}
}
inline void initialize()noexcept{
 try {
  const auto load=nrhip::api().module_load_data;
  const auto function=nrhip::api().module_get_function;
  const auto global=reinterpret_cast<int(*)(void**,size_t*,void*,const char*)>(nrhip::api().module_get_global);
  const auto copy=reinterpret_cast<int(*)(void*,const void*,size_t,int)>(nrhip::api().memcpy_);
  const auto props=nrhip::api().get_device_properties;
  get_device=reinterpret_cast<decltype(get_device)>(nrhip::api().get_device);launch=nrhip::api().module_launch_kernel;
  if(!load||!function||!global||!copy||!launch||!get_device||!props)return;
  if(!nrhip::dispositivo_ok(get_device,props,device))return;
  if(load(&module,module_bytes.data())||load(&export_module,export_bytes.data()))return;
  if(export_fast&&(load(&cauda_module,cauda_bytes.data())||!cauda_module||function(&export_r0_kernel,cauda_module,nr1080::contract::export_r0)||!export_r0_kernel)){export_r0_kernel=nullptr;log("EXPORT_R0 FALLBACK nr_cauda.hsaco recusado; nr_tone_export");}
  else if(export_r0_kernel)log("EXPORT_R0 READY nr_tone_export_r0 (reinhard lift 0) no lugar do nr_tone_export");
  if(function(&import_kernel,module,"_Z8k_import12ImportParams")||function(&reproject_kernel,module,"_Z11k_reproject12ReprojParams")||
     function(&full_export_kernel,module,"_Z8k_export12ExportParams")||function(&export_kernel,export_module,"nr_tone_export"))return;
  void* tm{};void* li{};size_t a{},b{};if(global(&tm,&a,module,"g_tm")||global(&li,&b,module,"g_lift")||a!=4||b!=4)return;
  if(copy(tm,&curve,4,1)||copy(li,&lift,4,1))return;
  if(v43){void* fp{};size_t c{};if(global(&fp,&c,module,"g_fastpow")||c!=4||copy(fp,&fastpow,4,1))return;}
  ready=true;log(v43?(fastpow?"READY gfx1201 import/reproject/export v043 (fastpow 1); same stream; custom residual-domain export":"READY gfx1201 import/reproject/export v043 (fastpow 0); same stream; custom residual-domain export"):"READY gfx1201 import/reproject v033; same stream; custom residual-domain export");
 }catch(...){}
}
inline bool try_launch(const void* f,unsigned gx,unsigned gy,unsigned gz,unsigned bx,unsigned by,unsigned bz,
 void** args,size_t shared,void* stream,int& result){
 if(!requested||!image||(f!=image+0x62228&&f!=image+0x62230&&f!=image+0x62238))return false;
 std::call_once(init,[]{initialize();if(!ready){requested=false;log("FALLBACK tone initialization failed before import");}});
 if(!ready)return false;
 int current{};if(failed||!args||!args[0]||get_device(&current)||current!=device){result=1;failed=true;return true;}
 if(f==image+0x62230&&get<unsigned>(args[0],0x38)){
  const auto p=args[0];auto graph=get<void*>(p,0),original=get<void*>(p,0x30),destination=get<void*>(p,0x20);
  auto stride=get<unsigned>(p,8),height=get<unsigned>(p,12),width=get<unsigned>(p,16),pitch=get<unsigned>(p,20),tone=get<unsigned>(p,0x28);
  if(get<unsigned>(p,24)!=0||!graph||!original||!destination||width==0||height==0||width>16384||height>16384||stride<width||pitch<width*8u){result=1;failed=true;log("REJECT unsupported residual export ABI");return true;}
  void* a[]={&graph,&original,&stride,&width,&height,&pitch,&destination,&tone,&curve,&lift};

  if(export_r0_kernel&&curve==0&&lift==0.f&&!(reinterpret_cast<std::uintptr_t>(graph)&15u)&&!(reinterpret_cast<std::uintptr_t>(destination)&7u)&&!(pitch&7u))
   result=launch(export_r0_kernel,(width+255u)/256u,height,1,256,1,1,0,stream,a,nullptr);
  else result=launch(export_kernel,(width*height+255u)/256u,1,1,256,1,1,0,stream,a,nullptr);
 }else result=launch(f==image+0x62228?import_kernel:f==image+0x62238?reproject_kernel:full_export_kernel,gx,gy,gz,bx,by,bz,unsigned(shared),stream,args,nullptr);
 if(result){failed=true;log("FAILED tone kernel; propagated, no mixed-domain fallback");}return true;
}
}
