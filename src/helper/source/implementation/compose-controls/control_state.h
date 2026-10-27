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

inline constexpr unsigned passo_min[3]{0u,0u,20u},passo_max[3]{40u,80u,160u},passo_padrao[3]{20u,20u,40u};
struct Carga {unsigned requested=defaults;bool invalida[3]{};bool guarda_desligada=false;bool ignorados=false;};
inline bool guarda_zero(const wchar_t* text){
 if(!text||!*text)return false;wchar_t* end=nullptr;const double v=std::wcstod(text,&end);return end&&end!=text&&!*end&&v==0.;
}
inline Carga carregar(const wchar_t* const textos[3]){
 Carga c;unsigned passos[3]{passo_padrao[0],passo_padrao[1],passo_padrao[2]};
 for(unsigned i=0;i<3;++i){
  const wchar_t* t=textos?textos[i]:nullptr;
  if(!t||!*t)continue;
  if(i==2&&guarda_zero(t)){c.guarda_desligada=true;continue;}
  unsigned v=0;if(parse(t,passo_min[i],passo_max[i],v))passos[i]=v;else c.invalida[i]=true;
 }
 if(c.guarda_desligada){c.ignorados=passos[0]!=passo_padrao[0]||passos[1]!=passo_padrao[1];return c;}
 c.requested=0x80000000u|(passos[0]<<8)|(passos[1]<<14)|(passos[2]<<21);
 return c;
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
