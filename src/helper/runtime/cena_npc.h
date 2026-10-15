#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
namespace nrcnpc {
struct Cfg {unsigned modo{1},inicio{6},ref_n{640};float fixo{1.f},fmin{.4f},fmax{1.3f},alfa_f{.25f},alfa_ref{0.f};};
struct Trilha {float rref{},fs{1.f};unsigned n{};};
inline float alvo(const Cfg& c,float rref,float r) noexcept {
    if(c.modo==2u)return std::clamp(rref/std::max(r,1e-3f),c.fmin,c.fmax);
    const float den=1.f-r,num=1.f-rref;
    if(std::fabs(den)<.03f)return 1.f;
    const float f=num/std::copysign(std::max(std::fabs(den),.03f),den);
    return f>0.f?std::clamp(f,c.fmin,c.fmax):c.fmin;
}

inline void atualizar(const Cfg& c,Trilha& t,bool reinicio,bool valido,float r) noexcept {
    if(reinicio){t=Trilha{};}
    if(valido){
        ++t.n;
        const unsigned ini=std::max(c.inicio,1u);
        const float rc=std::clamp(r,.3f,1.5f);
        if(t.n<=ini){t.rref=rc;t.fs=1.f;}
        else if(t.n<=2u*ini){t.rref+=(rc-t.rref)/float(t.n-ini);t.fs=1.f;}
        else{
            if(c.alfa_ref>0.f)t.rref+=(rc-t.rref)*c.alfa_ref;
            else if(c.alfa_ref==0.f)t.rref+=(rc-t.rref)/float(std::min(t.n-ini,std::max(c.ref_n/64u,1u)*64u));
            t.fs+=(alvo(c,t.rref,r)-t.fs)*c.alfa_f;
        }
    }
    if(c.modo==3u&&t.n>0u)t.fs=c.fixo;
}

inline unsigned zonas_validas(unsigned z) noexcept {return z>=6u?6u:z>=4u?4u:z>=2u?2u:1u;}
inline const float* bordas(unsigned z) noexcept {
    static const float b[4][7]={{-.95f,6.2f,6.2f,6.2f,6.2f,6.2f,6.2f},{-.95f,.75f,6.2f,6.2f,6.2f,6.2f,6.2f},
                                {-.95f,.75f,2.f,3.5f,6.2f,6.2f,6.2f},{-.95f,.4f,1.1f,1.9f,2.7f,3.7f,6.2f}};
    z=zonas_validas(z);return b[z==6u?3:z==4u?2:z==2u?1:0];}
inline unsigned zona_de(unsigned z,float v) noexcept {z=zonas_validas(z);const float* e=bordas(z);unsigned i=0;for(unsigned q=1;q<z;++q)if(v>=e[q])i=q;return i;}
inline float f_em(unsigned z,const float* f,float v) noexcept {
    z=zonas_validas(z);const float* e=bordas(z);float ca=.5f*(e[0]+e[1]),fa=f[0];
    if(v<=ca||z==1u)return fa;
    for(unsigned i=1;i<z;++i){const float c=.5f*(e[i]+e[i+1]);if(v<=c)return fa+(f[i]-fa)*(v-ca)/(c-ca);ca=c;fa=f[i];}
    return fa;
}

inline float compor(unsigned modo,float c,float e,float m,float f) noexcept {
    if(!(m>0.f))return e;
    const float a=1.f+m*(f-1.f);
    return modo==2u?e*a:c+a*(e-c);
}

struct Vista {bool on{};std::uint64_t ger{};float cx{},cy{},t{};};
inline bool reiniciar(const Vista& ant,const Vista& at) noexcept {
    if(!at.on)return false;
    if(!ant.on||ant.ger!=at.ger)return true;
    if(!(ant.t>0.f))return true;
    if(std::hypot(at.cx-ant.cx,at.cy-ant.cy)>at.t)return true;
    return at.t>1.6f*ant.t||at.t*1.6f<ant.t;
}

inline std::uint32_t f16(float f) noexcept {
    std::uint32_t x;std::memcpy(&x,&f,4);
    const std::uint32_t s=(x>>16)&0x8000u;const int e=int((x>>23)&255u)-127+15;const std::uint32_t m=x&0x7fffffu;
    if(e<=0)return s;
    if(e>=31)return s|0x7bffu;
    std::uint32_t h=s|(std::uint32_t(e)<<10)|(m>>13);
    if(m&0x1000u)++h;
    return h&0xffffu;
}

inline float fpx(float fov,unsigned altura) noexcept {
    const float f=(fov>.2f&&fov<2.6f)?fov:1.0471976f;
    return .5f*float(altura)/std::tan(.5f*f);
}
}
