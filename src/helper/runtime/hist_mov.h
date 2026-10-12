#pragma once

#include "nr_publico.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <algorithm>
namespace nrhistmov {
inline int modo=0;inline float teto=8.f;inline bool log_on=false;
inline std::uint64_t ultimo=0;
inline unsigned long long n=0,fora_cena=0,escalados=0,sem_quadro=0;inline unsigned long long hist_p[10]{};
inline thread_local int job_atual=0;
struct Salvo {void* p{};float sx{},sy{};bool ok{false};};

inline unsigned quadros_desde(std::uint64_t q) noexcept {
 unsigned P=1;
 if(q&&ultimo&&q>ultimo){const std::uint64_t d=q-ultimo;P=d>1000u?1000u:unsigned(d);}
 if(q)ultimo=q;
 return P;
}

inline Salvo antes(void* params,bool cena,std::uint64_t quadro,int job,float media_ult,bool media_ok,void(*logf)(const char*)) noexcept {
 Salvo s{};
 if(!params)return s;
 char* p=static_cast<char*>(params);
 ++n;
 if(!cena){++fora_cena;return s;}
 const unsigned P=quadros_desde(quadro);
 if(!quadro)++sem_quadro;
 ++hist_p[P<9u?P:9u];
 float sx{},sy{};std::memcpy(&sx,p+0x58,4);std::memcpy(&sy,p+0x5c,4);
 float k=1.f;
 if(modo==1&&P>1u&&std::isfinite(sx)&&std::isfinite(sy)){
  k=std::min(float(P),teto);
  s.p=params;s.sx=sx;s.sy=sy;s.ok=true;
  const float nx=sx*k,ny=sy*k;std::memcpy(p+0x58,&nx,4);std::memcpy(p+0x5c,&ny,4);++escalados;
 }
 if(log_on&&(n<=48||n%64==0)){
  const void* mv{};int pitch{},fmt{},mvw{},mvh{},neth{},netw{};float razao{};
  std::memcpy(&mv,p+0x10,8);std::memcpy(&pitch,p+0x18,4);std::memcpy(&fmt,p+0x1c,4);std::memcpy(&mvw,p+0x20,4);std::memcpy(&mvh,p+0x24,4);
  std::memcpy(&neth,p+0x68,4);std::memcpy(&netw,p+0x6c,4);std::memcpy(&razao,p+0x78,4);
  char line[420]{};
  NRLOGF(line,sizeof line,"HISTMOV n=%llu job=%d quadro=%llu P=%u k=%.2f modo=%d escala=%.5f,%.5f mv=%p fmt=%d %dx%d pitch=%d rede=%dx%d razao=%.4f media_ult=%s%.2f fora_cena=%llu sem_quadro=%llu hist_P1..9+=%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu",
   n,job,static_cast<unsigned long long>(quadro),P,double(k),modo,double(sx),double(sy),mv,fmt,mvw,mvh,pitch,netw,neth,double(razao),
   media_ok?"":"?",double(media_ult),fora_cena,sem_quadro,hist_p[1],hist_p[2],hist_p[3],hist_p[4],hist_p[5],hist_p[6],hist_p[7],hist_p[8],hist_p[9]);
  if(logf)logf(line);
 }
 return s;
}
inline void depois(const Salvo& s) noexcept {
 if(!s.ok||!s.p)return;
 char* p=static_cast<char*>(s.p);std::memcpy(p+0x58,&s.sx,4);std::memcpy(p+0x5c,&s.sy,4);
}
}
