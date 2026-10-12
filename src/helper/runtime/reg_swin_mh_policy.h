#pragma once
#include <cstdint>
#include <cstring>

namespace nrmhpolicy {
inline int window_index(unsigned window) noexcept {return window==64?0:window==128?1:window==256?2:-1;}
inline unsigned layout_shift(unsigned window) noexcept {return window==64?1u:window==128?2u:window==256?3u:0u;}
inline int mh_mode_index(unsigned mode) noexcept {
 constexpr unsigned modes[8]{0,1,2,3,4,5,8,10};
 for(int i=0;i<8;++i)if(modes[i]==mode)return i;
 return -1;
}

struct Route {bool enabled; unsigned mh_mode; bool convert_input; bool convert_output; bool convert_aux;};
inline Route route(unsigned mode) noexcept {
 switch(mode){

  case 0: return {true,0,true,true,false};

  case 1: return {true,1,false,true,false};

  case 2: return {true,2,true,false,false};

  case 4: return {true,4,true,true,false};
  case 5: return {true,5,false,true,false};

  case 8: return {true,8,true,true,false};
  default: return {false,0,false,false,false};
 }
}
struct Plan {unsigned mh_mode{};int mode_index{-1};unsigned shift{};std::uint64_t bytes{};void* input{};void* output{};void* aux{};
 bool convert_input{};bool convert_output{};bool convert_aux{};};

inline bool plan(unsigned window,const void* params,Plan& p) noexcept {
 const int w=window_index(window);if(w<0||!params)return false;
 const auto* b=static_cast<const char*>(params);
 unsigned mode{};std::memcpy(&mode,b+0x28,4);
 const Route r=route(mode);if(!r.enabled)return false;
 int height{},width{};std::memcpy(&height,b+0x18,4);std::memcpy(&width,b+0x1c,4);
 if(height<=0||width<=0||height>16384||width>16384)return false;
 p.shift=layout_shift(window);
 p.bytes=std::uint64_t(height)*std::uint64_t(width)*window;
 if(p.bytes%(512ull<<p.shift))return false;
 std::memcpy(&p.input,b,8);std::memcpy(&p.output,b+8,8);
 if(!p.input||!p.output)return false;
 const auto a=reinterpret_cast<std::uintptr_t>(p.input),c=reinterpret_cast<std::uintptr_t>(p.output);
 if((a|c)&3u)return false;

 if((a>c?a-c:c-a)<p.bytes)return false;
 auto disjunto=[](std::uintptr_t p0,std::uint64_t n0,std::uintptr_t p1,std::uint64_t n1){return p0+n0<=p1||p1+n1<=p0;};

 if(mode==8){
  void* aux{};std::memcpy(&aux,b+0x30,8);const auto x=reinterpret_cast<std::uintptr_t>(aux);
  const std::uint64_t half=p.bytes/2;
  if(!aux||(x&3u)||!disjunto(x,half,a,p.bytes)||!disjunto(x,half,c,p.bytes))return false;
 }
 if(mode==4||mode==5){

  void* aux{};std::memcpy(&aux,b+0x38,8);const auto x=reinterpret_cast<std::uintptr_t>(aux);
  const std::uint64_t half=p.bytes/2;
  if(!aux||(x&3u)||!disjunto(x,half,a,p.bytes)||!disjunto(x,half,c,p.bytes))return false;
 }
 p.mh_mode=r.mh_mode;p.mode_index=mh_mode_index(r.mh_mode);
 p.convert_input=r.convert_input;p.convert_output=r.convert_output;p.convert_aux=r.convert_aux;
 if(r.convert_aux){std::memcpy(&p.aux,b+0x38,8);if(!p.aux)return false;}
 return p.mode_index>=0;
}

struct Tensor {void* ptr{};std::uint64_t bytes{};unsigned shift{};void* stream{};};
struct Chain {
 bool pending{};Tensor held{};

 bool park_enabled{};void* stage_out[3]{};bool parked[3]{};Tensor park[3]{};

 bool decoder_enabled{true};
 struct Step {bool input_ready{};bool defer_output{};bool flush{};Tensor flush_tensor{};bool park_output{};bool flush_park{};Tensor park_tensor{};};
 Step begin(const Plan& p,unsigned mode,void* stream,bool enabled) noexcept {
  Step s{};
  const bool matches=pending&&enabled&&p.convert_input&&held.ptr==p.input&&held.bytes==p.bytes&&held.shift==p.shift&&held.stream==stream;
  if(pending&&!matches){s.flush=true;s.flush_tensor=held;}
  pending=false;s.input_ready=matches;

  s.defer_output=enabled&&p.convert_output&&(mode==1||(mode==8&&decoder_enabled)||(mode==0&&matches));
  const int w=int(p.shift)-1;
  if(w>=0&&w<3){
   if(mode==1){

    if(parked[w]){if(!(park[w].ptr==p.output&&park[w].bytes==p.bytes&&park[w].stream==stream)){s.flush_park=true;s.park_tensor=park[w];}parked[w]=false;}
    stage_out[w]=enabled&&park_enabled?p.output:nullptr;
   }else if(mode==8&&parked[w]){
    if(enabled&&p.convert_input&&park[w].ptr==p.input&&park[w].bytes==p.bytes&&park[w].shift==p.shift&&park[w].stream==stream)s.input_ready=true;
    else {s.flush_park=true;s.park_tensor=park[w];}
    parked[w]=false;
   }else if(mode==4){

    s.park_output=enabled&&park_enabled&&matches&&p.convert_output&&stage_out[w];
   }
  }
  return s;
 }
 void end(const Plan& p,void* stream,const Step& s) noexcept {
  if(s.defer_output){pending=true;held={p.output,p.bytes,p.shift,stream};}
  const int w=int(p.shift)-1;
  if(s.park_output&&w>=0&&w<3){parked[w]=true;park[w]={stage_out[w],p.bytes,p.shift,stream};stage_out[w]=nullptr;}
 }
 bool take(Tensor& t) noexcept {if(!pending)return false;t=held;pending=false;return true;}
 bool take_park(int w,Tensor& t) noexcept {if(w<0||w>2||!parked[w])return false;t=park[w];parked[w]=false;return true;}
};

inline bool restore_input(const Plan& p,bool drop) noexcept {return p.convert_input&&!drop;}

inline bool may_flush(bool query_ok,int current,int owner) noexcept {return query_ok&&current==owner;}
inline bool drop_restore_allowed(bool chain_enabled,bool drop_ini) noexcept {return chain_enabled&&drop_ini;}

inline bool chain_allowed(bool mh_requested,bool chain_ini,bool kernel_trace,bool contract_trace,bool stages_env) noexcept {
 return mh_requested&&chain_ini&&!kernel_trace&&!contract_trace&&!stages_env;
}
}
