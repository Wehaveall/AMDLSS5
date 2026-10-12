#pragma once
#include <atomic>
#include <cmath>
#include <cwchar>
#include <cstring>
namespace nrcompose {
inline constexpr unsigned defaults=0x80000000u|(20u<<8)|(20u<<14)|(40u<<21);
inline unsigned detail(unsigned v){return (v>>8)&63u;}
inline unsigned color(unsigned v){return (v>>14)&127u;}
inline unsigned guard(unsigned v){return (v>>21)&255u;}
inline bool valid(unsigned v){return (v&0xe00000ffu)==0x80000000u && detail(v)<=40 && color(v)<=80 && guard(v)>=20 && guard(v)<=160;}
struct State {std::atomic<unsigned> requested{defaults},recorded{0};};
inline State state;
inline bool parse(const wchar_t* text,unsigned lo,unsigned hi,unsigned& value){
 if(!text||!*text)return false;wchar_t* end=nullptr;const double v=std::wcstod(text,&end);
 if(!end||*end||!std::isfinite(v)||v<double(lo)/20||v>double(hi)/20)return false;
 const double step=std::round(v*20);if(std::abs(v*20-step)>1e-5)return false;
 value=static_cast<unsigned>(step);return true;
}
inline bool set(State& s,unsigned which,unsigned value){
 const unsigned shift=which==0?8u:which==1?14u:21u;
 const unsigned mask=(which==0?63u:which==1?127u:255u)<<shift;
 if(which>2||(which==0&&value>40)||(which==1&&value>80)||(which==2&&(value<20||value>160)))return false;
 auto previous=s.requested.load();
 do{if(!valid(previous))return false;}while(!s.requested.compare_exchange_weak(previous,(previous&~mask)|(value<<shift)));
 return true;
}

}
