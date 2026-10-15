#pragma once
#include "nr_publico.h"

#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <filesystem>
#include "native_ini_path.h"
#include "reg_swin32.h"
#include "hip_priority_policy.h"
namespace nrhipprio {
struct Dim {unsigned x,y,z;};
using Launch=int(*)(const void*,Dim,Dim,void**,size_t,void*);
using CopyAsync=int(*)(void*,const void*,size_t,int,void*);
using SetAsync=int(*)(void*,int,size_t,void*);
using EventRecord=int(*)(void*,void*);
inline Launch launch_original{};
inline CopyAsync copy_original{};
inline SetAsync set_original{};
inline EventRecord record_original{};
inline thread_local bool in_eval=false;
inline std::atomic<int> state{0};
inline std::once_flag creation;
inline void* stream{};
inline std::atomic<unsigned long long> redirected{0};
inline std::atomic<bool> handshake_logged[policy::handshake_count]{};
inline char* image_base{};
inline std::filesystem::path folder;
inline void log(const char* text) noexcept {
 try{FILE* f=nullptr;if(_wfopen_s(&f,(folder/L"nr_prioridade.log").c_str(),L"a")==0&&f){std::fprintf(f,"pid=%lu %s\n",GetCurrentProcessId(),text);std::fclose(f);}}catch(...){}
}

inline int create() noexcept {
 auto hip=GetModuleHandleW(L"amdhip64_7.dll");
 auto range=hip?reinterpret_cast<int(*)(int*,int*)>(GetProcAddress(hip,"hipDeviceGetStreamPriorityRange")):nullptr;
 auto make=hip?reinterpret_cast<int(*)(void**,unsigned,int)>(GetProcAddress(hip,"hipStreamCreateWithPriority")):nullptr;
 auto get=hip?reinterpret_cast<int(*)(void*,int*)>(GetProcAddress(hip,"hipStreamGetPriority")):nullptr;
 auto by_symbol=hip?reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipGetFuncBySymbol")):nullptr;
 auto name=hip?reinterpret_cast<const char*(*)(void*)>(GetProcAddress(hip,"hipKernelNameRef")):nullptr;
 if(!by_symbol||!name){log("REJECTED hipGetFuncBySymbol/hipKernelNameRef missing");return -1;}
 for(unsigned i=0;i<policy::handshake_count;++i){
  void* fn{};const char* n{};
  if(by_symbol(&fn,image_base+policy::handshake_offsets[i])||!(n=name(fn))||std::strcmp(n,policy::handshake_names[i])){
   char text[160];NRLOGF(text,sizeof text,"REJECTED handshake token 0x%x is not %s",policy::handshake_offsets[i],policy::handshake_names[i]);log(text);return -1;
  }
 }
 int least=0,greatest=0,actual=0;void* created=nullptr;int r1=-1,r2=-1,r3=-1;
 if(range&&make){r1=range(&least,&greatest);if(!r1)r2=make(&created,0u,greatest);if(!r2&&get)r3=get(created,&actual);}

 auto current=hip?reinterpret_cast<int(*)(int*)>(GetProcAddress(hip,"hipGetDevice")):nullptr;int device=-1;if(current&&current(&device))device=-1;
 char text[256];NRLOGF(text,sizeof text,"PRIORITY_STREAM device=%d range_result=%d least=%d greatest=%d create_result=%d stream=%p get_result=%d priority=%d flags=0 handshake=null",device,r1,least,greatest,r2,created,r3,actual);
 log(text);
 if(!r1&&!r2&&created){stream=created;return 1;}
 return -1;
}
inline std::atomic<unsigned> outside_logged{0};
inline void* target(void* requested,policy::Call call,bool handshake=false) noexcept {
 if(!policy::wants_stream(call,in_eval,requested,handshake))return requested;
 int s=state.load(std::memory_order_acquire);
 if(s==0){std::call_once(creation,[]{state.store(create(),std::memory_order_release);});s=state.load(std::memory_order_acquire);}
 if(policy::route(call,in_eval,s==1,requested,handshake)!=policy::Route::priority)return requested;
 const auto n=redirected.fetch_add(1,std::memory_order_relaxed)+1;
 if(n==1||n==100000){char text[96];NRLOGF(text,sizeof text,"REDIRECTED calls=%llu stream=%p",n,stream);log(text);}
 return stream;
}
inline int launch(const void* f,Dim g,Dim b,void** args,size_t shared,void* s){
 const int h=policy::handshake_index(f,image_base);
 if(h>=0&&!handshake_logged[h].exchange(true,std::memory_order_relaxed)){
  char text[160];NRLOGF(text,sizeof text,"HANDSHAKE %s in_eval=%d stream=%p (fica no stream pedido)",policy::handshake_names[h],in_eval?1:0,s);log(text);
 }
 void* const routed=target(s,policy::Call::kernel,h>=0);

 if(h<0&&!in_eval&&routed!=s&&outside_logged.fetch_add(1,std::memory_order_relaxed)<24){
  char text[128];NRLOGF(text,sizeof text,"OUTSIDE_EVAL kernel token=+0x%llx grid=%u,%u,%u -> prioridade",
   static_cast<unsigned long long>(static_cast<const char*>(f)-image_base),g.x,g.y,g.z);log(text);
 }
 return launch_original(f,g,b,args,shared,routed);
}
inline int copy_async(void* d,const void* s,size_t n,int kind,void* st){return copy_original(d,s,n,kind,target(st,policy::Call::copy_memset_event));}
inline int set_async(void* d,int v,size_t n,void* st){return set_original(d,v,n,target(st,policy::Call::copy_memset_event));}
inline int event_record(void* e,void* st){return record_original(e,target(st,policy::Call::copy_memset_event));}
struct Scope {
 bool previous;explicit Scope(bool active) noexcept:previous(in_eval){if(active)in_eval=true;}
 ~Scope(){in_eval=previous;}
};
inline bool active() noexcept {return launch_original!=nullptr;}
inline std::atomic<char*> installed_image{nullptr};
inline bool install(char* image) noexcept {
 if(installed_image.load()==image)return true;
 try{
  wchar_t path[32768]{};HMODULE self{};
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&install),&self)||
     !GetModuleFileNameW(self,path,32768))return false;
  folder=std::filesystem::path(path).parent_path();
  const auto ini=nrinipath::resolve(image,reinterpret_cast<const void*>(&install));
  if(ini.empty())return true;
  const int key=static_cast<int>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralHipPriority",policy::absent,ini.c_str()));
  const int legacy=static_cast<int>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralAsyncHipPriority",policy::absent,ini.c_str()));
  if(!policy::enabled(key,legacy)){log("OFF NeuralHipPriority=0 (rede no stream nulo)");return true;}
  auto hip=GetModuleHandleW(L"amdhip64_7.dll");if(!hip){log("REJECTED amdhip64_7 not loaded");return false;}
  constexpr unsigned count=4;
  void* originals[count]{GetProcAddress(hip,"hipLaunchKernel"),GetProcAddress(hip,"hipMemcpyAsync"),GetProcAddress(hip,"hipMemsetAsync"),GetProcAddress(hip,"hipEventRecord")};
  void* hooks[count]{reinterpret_cast<void*>(&launch),reinterpret_cast<void*>(&copy_async),reinterpret_cast<void*>(&set_async),reinterpret_cast<void*>(&event_record)};
  constexpr unsigned slots[count]{policy::slot_launch,policy::slot_copy_async,policy::slot_set_async,policy::slot_event_record};
  for(unsigned i=0;i<count;++i){void* current=*reinterpret_cast<void**>(image+slots[i]);
   if(!originals[i]||(i==0?!nrregswin::known_launch(current,originals[i]):current!=originals[i])){log("REJECTED import slot is not the pinned amdhip64 export or a known helper hook");return false;}originals[i]=current;}
  image_base=image;
  launch_original=reinterpret_cast<Launch>(originals[0]);copy_original=reinterpret_cast<CopyAsync>(originals[1]);
  set_original=reinterpret_cast<SetAsync>(originals[2]);record_original=reinterpret_cast<EventRecord>(originals[3]);
  for(unsigned i=0;i<count;++i){auto slot=reinterpret_cast<void**>(image+slots[i]);DWORD old{},unused{};
   if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old)){log("REJECTED VirtualProtect");return false;}
   InterlockedExchangePointer(reinterpret_cast<void* volatile*>(slot),hooks[i]);VirtualProtect(slot,sizeof(void*),old,&unused);}
  nrregswin::register_hook(reinterpret_cast<void*>(&launch));
  installed_image.store(image);
  log("INSTALLED null-stream -> high priority stream: kernels do proxy (menos o handshake) sempre; hipMemcpyAsync/hipMemsetAsync/hipEventRecord dentro do Eval (Sincrono e Assincrono)");
  return true;
 }catch(...){return false;}
}
}
