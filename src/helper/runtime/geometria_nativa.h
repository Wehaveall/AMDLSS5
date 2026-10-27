#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace nrgeo {
inline std::atomic<bool> ligada{false};
inline constexpr std::uint32_t rva_marca=0x739a00,rva_chave=0x739a04,marca=0x314f4547u;

constexpr unsigned alinhar(unsigned v,unsigned a) noexcept {return (v+a-1u)/a*a;}

constexpr unsigned alinhamento(unsigned valido) noexcept {
    unsigned r=0,s=valido;
    for(int nivel=0;nivel<6;++nivel){const unsigned m=alinhar((s+1u)/2u,4u);if(m<s)++r;if(nivel==0&&(m%8u))++r;s=m;}
    return 1u<<r;
}
struct Campo {unsigned w{},h{};};
constexpr Campo campo(unsigned w,unsigned h) noexcept {
    const unsigned aw=alinhamento(w),ah=alinhamento(h);
    Campo c{(std::max)(320u,alinhar(w,aw)),(std::max)(320u,alinhar(h,ah))};
    if(c.w%(4u*aw)==0&&c.h%(4u*ah)==0)c.w+=aw;
    return c;
}

constexpr bool compativel(unsigned w,unsigned h) noexcept {
    if(w<64||h<64)return false;const Campo c=campo(w,h);return c.w%128u==0&&c.h%128u==0;}

constexpr unsigned area_antiga(unsigned w,unsigned h) noexcept {return alinhar(w,128u)*alinhar(h,128u);}

struct Tam {unsigned w{},h{};};
inline Tam cena(unsigned w,unsigned h) noexcept {
    if(w<64||h<64||compativel(w,h))return {w,h};
    const double asp=double(w)/double(h);const unsigned teto=area_antiga(w,h);
    Tam melhor{w,h};std::uint64_t melhor_area=0;
    for(unsigned x=(w*85u/100u)&~1u;x<=w*110u/100u;x+=2u){

        const unsigned y=unsigned(std::lround(double(x)/asp/2.))*2u;
        if(y<64||std::fabs(double(x)/double(y)/asp-1.)>0.003||!compativel(x,y))continue;
        const Campo c=campo(x,y);if(std::uint64_t(c.w)*c.h>teto)continue;
        const std::uint64_t a=std::uint64_t(x)*y;
        if(a>melhor_area){melhor_area=a;melhor={x,y};}
    }
    return melhor;
}

inline unsigned rosto(unsigned n,unsigned teto,unsigned L) noexcept {
    if(teto>=64u&&n>teto)n=teto;
    if(n<64||compativel(n,n))return n;
    const unsigned limite=(std::min)(teto,2u*L);
    unsigned baixo=0,alto=0;
    for(unsigned k=384u;k<=4096u;k+=256u){if(k<=n)baixo=k;if(k>n&&!alto)alto=k;}
    const bool alto_ok=alto&&alto<=limite,baixo_ok=baixo>=384u&&baixo<=limite;
    if(alto_ok&&baixo_ok)return std::fabs(std::log(double(alto)/double(n)))<std::fabs(std::log(double(n)/double(baixo)))?alto:baixo;
    if(baixo_ok)return baixo;
    if(alto_ok)return alto;
    return n;
}
}
