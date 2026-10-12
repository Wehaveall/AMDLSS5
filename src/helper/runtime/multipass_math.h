#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace nrmp {
inline unsigned valid_config(unsigned v) {
    return !(v & 0xffff0000u) && (v & 255u)>=1 && (v & 255u)<=3 && ((v>>8)&255u)<=6 ? v : 0x301u;
}
inline float multiplier(unsigned v) {
    constexpr float values[]={0,.5f,.75f,1,1.25f,1.5f,2};
    return values[(valid_config(v)>>8)&255u];
}
inline float from_half(std::uint16_t h) {
    const unsigned sign=unsigned(h&0x8000u)<<16, exp=(h>>10)&31, frac=h&1023;
    if(exp==0) return std::copysign(std::ldexp(float(frac),-24),sign?-1.f:1.f);
    unsigned bits=sign | (exp==31 ? 0x7f800000u : (exp+112)<<23) | (frac<<13);
    float f;std::memcpy(&f,&bits,4);return f;
}
inline std::uint16_t to_half(float f) {
    unsigned bits;std::memcpy(&bits,&f,4);
    unsigned sign=(bits>>16)&0x8000u, frac=bits&0x7fffffu;
    int exp=int((bits>>23)&255)-127+15;
    if(exp>=31)return std::uint16_t(sign|0x7c00u|(frac?0x200u:0));
    if(exp<=0) {
        if(exp < -10)return std::uint16_t(sign);
        frac|=0x800000u;unsigned shift=unsigned(14-exp), q=frac>>shift;
        unsigned remainder=frac&((1u<<shift)-1), half=1u<<(shift-1);
        return std::uint16_t(sign| (q+(remainder>half || (remainder==half && (q&1)))));
    }
    unsigned q=frac>>13, rem=frac&8191;
    q+=(rem>4096 || (rem==4096 && (q&1)));
    return std::uint16_t(sign + (unsigned(exp)<<10) + q);
}
inline float finite_clamp(float v) {
    if(!std::isfinite(v))throw std::runtime_error("nonfinite working RGB");
    return std::clamp(v,0.f,1.f);
}

inline std::uint16_t compose(float base,std::uint16_t delta) {
    return to_half(finite_clamp(base+from_half(delta)));
}
inline std::uint16_t residual(float base,std::uint16_t full) {
    float d=finite_clamp(from_half(full))-base;
    if(!std::isfinite(d)||std::abs(d)>65504.f)throw std::runtime_error("unrepresentable residual");
    return to_half(d);
}
}
