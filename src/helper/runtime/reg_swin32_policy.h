#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "pad_apara.h"

namespace nrregpolicy {
struct Dim {unsigned x,y,z;};
using Launch=int(*)(const void*,Dim,Dim,void**,std::size_t,void*);
using ModuleLaunch=int(*)(void*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,void*,void**,void**);
using LayoutLaunch=int(*)(bool,void*,std::uint64_t,void*);
inline constexpr unsigned modes[]{0,1,2,4,5,8,20,32};
inline int mode_index(unsigned mode) noexcept {for(int i=0;i<8;++i)if(modes[i]==mode)return i;return -1;}

inline bool proven_shape(unsigned mode,int height,int width) noexcept {
 if(height==768&&width==1280)return mode==20||mode==32;
 if(height==384&&width==640)return mode==0||mode==1||mode==2||mode==4||mode==5||mode==8;
 return false;
}

inline bool todas_formas=false;
inline bool any_shape(unsigned mode,int height,int width) noexcept {
 return mode_index(mode)>=0&&height>0&&width>0&&height<=16384&&width<=16384&&height%32==0&&width%32==0;
}
inline bool admitted(unsigned mode,const void* params) noexcept {
 if(!params)return false;
 int height{},width{};const auto* b=static_cast<const char*>(params);
 std::memcpy(&height,b+0x18,4);std::memcpy(&width,b+0x1c,4);
 return todas_formas?any_shape(mode,height,width):proven_shape(mode,height,width);
}
struct LayoutPlan {void* input{};void* output{};std::uint64_t bytes{};};
inline bool layout_plan(unsigned mode,const void* params,LayoutPlan& p) noexcept {
 const bool input=mode==0||mode==2||mode==4||mode==8||mode==32;
 const bool output=mode==0||mode==1||mode==4||mode==5||mode==8||mode==20;
 if(!input&&!output)return false;
 int height{},width{};const auto* b=static_cast<const char*>(params);
 std::memcpy(&height,b+0x18,4);std::memcpy(&width,b+0x1c,4);
 if(height<=0||width<=0||height>16384||width>16384)return false;
 p.bytes=std::uint64_t(height)*std::uint64_t(width)*32;
 if(p.bytes%512)return false;
 void* native_input{};void* native_output{};
 std::memcpy(&native_input,b,8);std::memcpy(&native_output,b+8,8);
 if(input)p.input=native_input;
 if(output)p.output=native_output;
 if((input&&!p.input)||(output&&!p.output))return false;
 const auto a=reinterpret_cast<std::uintptr_t>(native_input),c=reinterpret_cast<std::uintptr_t>(native_output);
 if((a|c)&3u)return false;

 if(a&&c&&(a>c?a-c:c-a)<p.bytes)return false;
 return true;
}

inline constexpr std::size_t varparams_host_bytes=168,varparams_wide_bytes=192,varparams_inserted_at=0x70;
inline void widen_varparams(const void* host,unsigned char (&wide)[varparams_wide_bytes],std::uint64_t noise=0) noexcept {
 const auto* b=static_cast<const unsigned char*>(host);
 std::memcpy(wide,b,varparams_inserted_at);
 std::memcpy(wide+varparams_inserted_at,&noise,8);
 std::memcpy(wide+varparams_inserted_at+8,b+varparams_inserted_at,varparams_host_bytes-varparams_inserted_at);
}
struct Dispatcher {
 Launch original{};ModuleLaunch module_launch{};const void* token{};const void* alternate_token{};void* functions[8]{};

 bool wide[8]{};bool shifted[8]{};
 std::atomic<bool> ready{false};
 std::atomic<int> failure{0};
 LayoutLaunch layout{};

 bool chain{};bool pending{};
 struct Held {void* ptr{};std::uint64_t bytes{};void* stream{};} held{};

 bool park_pretin{};bool parked{};Held park{};

 bool drop_restore{};

 bool park_stage{};void* stage_out{};bool parked_y{};Held pky{};
 static bool consumes(unsigned mode) noexcept {return mode==0||mode==2||mode==4||mode==8;}
 unsigned flushes{};
 int fail(int error) noexcept {pending=false;parked=false;parked_y=false;failure.store(error,std::memory_order_release);ready.store(false,std::memory_order_release);return error;}

 int flush() noexcept {
  if(!pending)return 0;
  pending=false;++flushes;
  if(const int error=layout(false,held.ptr,held.bytes,held.stream))return fail(error);
  return 0;
 }
 int flush_parked() noexcept {
  if(!parked)return 0;
  parked=false;++flushes;
  if(const int error=layout(false,park.ptr,park.bytes,park.stream))return fail(error);
  return 0;
 }
 int flush_y() noexcept {
  if(!parked_y)return 0;
  parked_y=false;++flushes;
  if(const int error=layout(false,pky.ptr,pky.bytes,pky.stream))return fail(error);
  return 0;
 }

 bool(*owner_device)() noexcept{};
 int refuse(const void* f,Dim grid,Dim block,void** args,std::size_t shared,void* stream) noexcept {
  if(!owner_device||owner_device()){
   if(const int error=flush())return error;
   if(f&&(f==token||f==alternate_token)){if(const int error=flush_parked())return error;if(const int error=flush_y())return error;}
  }
  return original(f,grid,block,args,shared,stream);
 }
 int launch(const void* f,Dim grid,Dim block,void** args,std::size_t shared,void* stream) noexcept {

  if(const int failed=failure.load(std::memory_order_acquire))return failed;
  if(!ready.load(std::memory_order_acquire)||!f||(f!=token&&f!=alternate_token)||block.x!=256||block.y!=1||block.z!=1||shared||!args||!args[0])
   return refuse(f,grid,block,args,shared,stream);

  unsigned mode{};std::memcpy(&mode,static_cast<const char*>(args[0])+0x28,sizeof(mode));
  const int index=mode_index(mode);
  if(index<0||!functions[index]||!admitted(mode,args[0]))return refuse(f,grid,block,args,shared,stream);
  LayoutPlan plan{};
  bool input_ready=false,pretin_ready=false,y_ready=false,park_out=false;
  if(layout){
   if(!layout_plan(mode,args[0],plan))return refuse(f,grid,block,args,shared,stream);
   input_ready=pending&&chain&&plan.input&&held.ptr==plan.input&&held.bytes==plan.bytes&&held.stream==stream;
   if(pending&&!input_ready){if(const int error=flush())return error;}
   pending=false;
   if(parked&&mode==32){
    pretin_ready=park_pretin&&park.ptr==plan.input&&park.bytes==plan.bytes&&park.stream==stream;
    if(pretin_ready)parked=false;else if(const int error=flush_parked())return error;
   }
   if(parked&&mode==20){

    if(park.ptr==plan.output&&park.bytes==plan.bytes&&park.stream==stream)parked=false;
    else if(const int error=flush_parked())return error;
   }
   if(mode==1){

    if(parked_y){if(pky.ptr==plan.output&&pky.bytes==plan.bytes&&pky.stream==stream)parked_y=false;else if(const int error=flush_y())return error;}
    stage_out=park_stage&&chain?plan.output:nullptr;
   }else if(mode==8&&parked_y){
    y_ready=park_stage&&pky.ptr==plan.input&&pky.bytes==plan.bytes&&pky.stream==stream;
    if(y_ready)parked_y=false;else if(const int error=flush_y())return error;
   }else if(mode==4){

    park_out=park_stage&&chain&&input_ready&&plan.output&&stage_out;
   }
   if(plan.input&&!input_ready&&!pretin_ready&&!y_ready){const int error=layout(true,plan.input,plan.bytes,stream);if(error)return fail(error);}
  }
  alignas(16) unsigned char wide_params[varparams_wide_bytes]{};void* wide_args[]{wide_params};
  if(wide[index]&&shifted[index])widen_varparams(args[0],wide_params);
  else if(wide[index])std::memcpy(wide_params,args[0],varparams_host_bytes);

  const nrpadapara::G2 gp=nrpadapara::antes2(mode,args[0],grid.x,grid.y);
  const int result=module_launch(functions[index],gp.x,gp.y,grid.z,32,1,1,0,stream,wide[index]?wide_args:args,nullptr);

  if(result)return fail(result);
  if(layout){
   if(plan.input&&!pretin_ready&&!(drop_restore&&consumes(mode))){const int error=layout(false,plan.input,plan.bytes,stream);if(error)return fail(error);}
   if(plan.output&&plan.output!=plan.input){
    const bool defer=chain&&(mode==1||mode==8||(mode==0&&input_ready));
    if(mode==20&&park_pretin){parked=true;park={plan.output,plan.bytes,stream};}
    else if(park_out){parked_y=true;pky={stage_out,plan.bytes,stream};stage_out=nullptr;}
    else if(defer){pending=true;held={plan.output,plan.bytes,stream};}
    else if(const int error=layout(false,plan.output,plan.bytes,stream))return fail(error);
   }
  }
  return 0;
 }
};
}
