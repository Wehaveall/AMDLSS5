#pragma once

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cwchar>
namespace nrpassesfrac {
inline std::atomic<int> decimos{10};
inline std::atomic<bool> semeado{false};

inline int ler(const wchar_t* s) noexcept {
 if(!s)return -1;
 wchar_t b[32]{};std::size_t n=0;
 while(*s==L' '||*s==L'\t')++s;
 for(;*s&&n+1<32;++s)b[n++]=*s==L','?L'.':*s;
 while(n&&(b[n-1]==L' '||b[n-1]==L'\t'))b[--n]=0;
 if(!n)return -1;
 for(std::size_t i=0;i<n;++i)if(!((b[i]>=L'0'&&b[i]<=L'9')||b[i]==L'.'))return -1;   // so digitos e ponto (sem nan/inf/expoente)
 wchar_t* fim{};const double v=std::wcstod(b,&fim);
 if(!fim||*fim||!std::isfinite(v))return -1;
 const long t=std::lround(v*10.0);
 return t>=10&&t<=20?int(t):-1;
}
inline unsigned passos(int total) noexcept {return total>10?2u:1u;}
inline int extra(int total) noexcept {return total<=10?0:(total>=20?10:total-10);}

inline int total(unsigned request,int dec) noexcept {
 if((request&255u)<=1u)return 10;
 if((request&255u)>2u)return 20;
 return 10+(dec<0?0:(dec>10?10:dec));
}

inline int proximo(int total,int delta) noexcept {
 if(total<10||total>20)return 10;
 const int t=total+(delta<0?-1:1);
 return t>20?10:(t<10?20:t);
}

inline unsigned config(unsigned request,int total) noexcept {return (request&0xff00u)|passos(total);}
inline float ganho(int dec) noexcept {return dec>=10?1.f:(dec<=0?0.f:float(dec)/10.f);}
inline void texto(int total,char* out,std::size_t n) noexcept {if(out&&n)std::snprintf(out,n,"%d.%d",total/10,total%10);}
inline void texto_w(int total,wchar_t* out,std::size_t n) noexcept {if(out&&n)std::swprintf(out,n,L"%d.%d",total/10,total%10);}

inline void semear(int total) noexcept {decimos.store(total<=10?10:extra(total),std::memory_order_relaxed);semeado.store(true,std::memory_order_release);}
}
