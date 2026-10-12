#pragma once

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace nrpadapara {
inline int modo=0;
inline int margem=0;

inline int modo_x=0;
struct G2 {unsigned x,y;};
inline unsigned char imp[48]{};
inline bool imp_ok=false;
inline int dec_k=-1;
inline std::atomic<unsigned long long> aparados{0},lancamentos{0},recusas{0};
inline void(*logger)(const char*)=nullptr;
inline std::atomic<unsigned> logged{0};
inline int pad128(int v) noexcept {return (v+127)&~127;}

inline bool acha_real(const unsigned char* p,int hf,int wf,int& h,int& w,int& off) noexcept {
 for(int o=0;o+8<=48;o+=4){int a{},b{};std::memcpy(&a,p+o,4);std::memcpy(&b,p+o+4,4);
  if(a>=64&&a<=hf&&pad128(a)==hf&&b>=64&&b<=wf&&pad128(b)==wf){h=a;w=b;off=o;return true;}}
 return false;
}

inline int janelas(int L,bool sh) noexcept {return sh?(L+4)/8+1:L/8+1;}
inline int ultima_lida(int L,bool sh) noexcept {const int r=sh?(L+4)/8:L/8;return sh?8*r+3:8*r+7;}

inline unsigned grid_y(int k,int real_full,int field_full,unsigned gy,int m) noexcept {
 if(k<0||k>4||real_full<=0||real_full>=field_full||m<0)return gy;
 static constexpr bool sh[4]{false,true,false,true};
 const int L=real_full-1+m;
 if(k==4){const unsigned n=unsigned(janelas(L,false));return n<gy?n:gy;}
 int Lh=ultima_lida(L,false)/2+m;
 for(int j=3;j>k;--j)Lh=ultima_lida(Lh,sh[j])+m;
 const unsigned n=unsigned(janelas(Lh,sh[k]));
 return n<gy?n:gy;
}

inline unsigned grid_x(int k,int real_full,int field_full,unsigned gx,int W,int m) noexcept {
 if(k<0||k>4||real_full<=0||real_full>=field_full||m<0||W<=0||W%8)return gx;
 const int L=real_full-1+m;
 if(k==4){if(gx!=unsigned(W/8))return gx;const unsigned n=unsigned(janelas(L,false));return n<gx?n:gx;}
 return gx;
}
inline unsigned grid_x_dec(int k,int real_full,int field_full,unsigned gx,const bool sh[4],int m) noexcept {
 if(k<0||k>3||real_full<=0||real_full>=field_full||m<0)return gx;
 const int L=real_full-1+m;
 int Lh=ultima_lida(L,false)/2+m;
 for(int j=3;j>k;--j)Lh=ultima_lida(Lh,sh[j])+m;
 const unsigned n=unsigned(janelas(Lh,sh[k]));
 return n<gx?n:gx;
}

inline bool forma_ok(int k,int H,unsigned gy) noexcept {
 static constexpr bool sh[4]{false,true,false,true};
 if(H<=0||H%8)return false;
 const unsigned base=unsigned(H/8);
 if(k==4)return gy==base;
 return gy==(sh[k]?base+1:base);
}

inline void no_import(const void* params) noexcept {
 if(!modo||!params)return;
 std::memcpy(imp,params,48);imp_ok=true;dec_k=-1;
}

inline unsigned antes(unsigned mode,const void* params,unsigned gy) noexcept;
inline G2 antes2(unsigned mode,const void* params,unsigned gx,unsigned gy) noexcept;
inline unsigned antes(unsigned mode,const void* params,unsigned gy) noexcept {return antes2(mode,params,0,gy).y;}
inline G2 antes2(unsigned mode,const void* params,unsigned gx,unsigned gy) noexcept {
 if(!modo||!params)return {gx,gy};
 int H{},W{};std::memcpy(&H,static_cast<const char*>(params)+0x18,4);std::memcpy(&W,static_cast<const char*>(params)+0x1c,4);
 int k=-1;
 if(mode==8)dec_k=k=0;
 else if(mode==0&&dec_k>=0&&dec_k<2)k=++dec_k;
 else if(mode==2&&dec_k==2)k=dec_k=3;
 else if(mode==32)k=4;
 else dec_k=-1;
 if(k<0)return {gx,gy};
 if(k==4)dec_k=-1;
 if(k<4&&modo<2)return {gx,gy};
 ++lancamentos;
 const int hf=k==4?H:2*H,wf=k==4?W:2*W;
 int h{},w{},off{};
 if(!imp_ok||!acha_real(imp,hf,wf,h,w,off)||!forma_ok(k,H,gy)){
  ++recusas;
  if(logger&&!(logged.fetch_or(1u<<8)&(1u<<8))){char t[160]{};std::snprintf(t,sizeof t,"APARA PAD RECUSA k=%d modo=%u H=%d W=%d grid.y=%u imp_ok=%d",k,mode,H,W,gy,imp_ok?1:0);logger(t);}
  return {gx,gy};
 }
 const unsigned n=grid_y(k,h,hf,gy,margem);
 if(n<gy)++aparados;

 unsigned nx=gx;
 if(modo_x&&gx&&W>0&&W%8==0){
  static constexpr bool shy[4]{false,true,true,false};
  if(k==4)nx=grid_x(4,w,wf,gx,W,margem);
  else if(gx==unsigned(W/8+(shy[k]?1:0)))nx=grid_x_dec(k,w,wf,gx,shy,margem);
 }
 const unsigned bit=1u<<(k+(hf>=1000?0:16));
 if(logger&&!(logged.fetch_or(bit)&bit)){char t[240]{};std::snprintf(t,sizeof t,"APARA PAD k=%d modo=%u real=%dx%d (ImportParams+0x%02x) campo=%dx%d H=%d W=%d grid.y %u -> %u grid.x %u -> %u margem=%d",k,mode,h,w,off,hf,wf,H,W,gy,n,gx,nx,margem);logger(t);}
 return {nx,n};
}
}
