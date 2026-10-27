#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include <immintrin.h>
#include <intrin.h>

namespace nrdet {
struct Rosto {float x,y,w,h;float lm[10];float nota;};

inline float mediana_numpy(std::vector<float>& v) noexcept {
    if(v.empty())return NAN;
    const size_t n=v.size(),m=n/2;
    std::nth_element(v.begin(),v.begin()+std::ptrdiff_t(m),v.end());
    if(n&1u)return v[m];
    const float a=v[m];const float b=*std::max_element(v.begin(),v.begin()+std::ptrdiff_t(m));
    return .5f*(a+b);
}

inline void tom_bgr(const float* rgb,int w,int h,std::vector<std::uint8_t>& bgr){
    const size_t n=size_t(w)*size_t(h)*3;bgr.assign(n,0);if(!n)return;
    std::vector<float> v(rgb,rgb+n);for(auto& x:v)if(!std::isfinite(x))x=0.f;
    const float med=mediana_numpy(v);
    const float k=.18f/(std::max)(1e-6f,med);
    for(size_t i=0;i<size_t(w)*size_t(h);++i)for(int c=0;c<3;++c){
        float x=rgb[i*3+size_t(c)];if(!std::isfinite(x))x=0.f;x*=k;
        x=(x*(2.51f*x+.03f))/(x*(2.43f*x+.59f)+.14f);x=std::clamp(x,0.f,1.f);x=std::pow(x,1.f/2.2f);
        bgr[i*3+size_t(2-c)]=std::uint8_t(std::clamp(std::floor(x*255.f+.5f),0.f,255.f));}
}

template<class Meia> inline void tom_bgr_meia(const std::uint16_t* hb,int w,int h,std::vector<std::uint8_t>& bgr,Meia meia){
    const size_t np=size_t(w)*size_t(h),n=np*3;bgr.assign(n,0);if(!n)return;
    auto val=[&](std::uint16_t c){float x=meia(c);return std::isfinite(x)?x:0.f;};
    static const std::vector<std::uint16_t> ordem=[&]{std::vector<std::uint16_t> o(65536);for(unsigned i=0;i<65536u;++i)o[i]=std::uint16_t(i);
        std::stable_sort(o.begin(),o.end(),[&](std::uint16_t a,std::uint16_t b){return val(a)<val(b);});return o;}();
    std::vector<std::uint32_t> hist(65536,0u);
    for(size_t i=0;i<np;++i){const std::uint16_t* p=hb+i*4;++hist[p[0]];++hist[p[1]];++hist[p[2]];}

    const size_t m=n/2;float vm=0.f,vm1=0.f;bool tem1=(n&1u)!=0;size_t acc=0;
    for(const std::uint16_t c:ordem){const std::uint32_t k=hist[c];if(!k)continue;
        if(!tem1&&acc+k>m-1){vm1=val(c);tem1=true;}
        if(acc+k>m){vm=val(c);break;}
        acc+=k;}
    float med=vm;if(!(n&1u))med=.5f*(med+vm1);
    const float k=.18f/(std::max)(1e-6f,med);
    std::vector<std::uint8_t> lut(65536,0);
    for(unsigned c=0;c<65536u;++c){if(!hist[c])continue;
        float x=meia(std::uint16_t(c));if(!std::isfinite(x))x=0;x*=k;
        x=(x*(2.51f*x+.03f))/(x*(2.43f*x+.59f)+.14f);x=std::clamp(x,0.f,1.f);x=std::pow(x,1.f/2.2f);
        lut[c]=std::uint8_t(std::clamp(std::floor(x*255.f+.5f),0.f,255.f));}
    for(size_t i=0;i<np;++i)for(int c=0;c<3;++c)bgr[i*3+size_t(2-c)]=lut[hb[i*4+size_t(c)]];
}

inline int nivel_simd(unsigned ecx1,unsigned ebx7,unsigned long long xcr0,int folhas) noexcept {
    const bool osx=(ecx1&(1u<<27))!=0,avx=(ecx1&(1u<<28))!=0,fma=(ecx1&(1u<<12))!=0;
    if(!osx||!avx||(xcr0&6u)!=6u||folhas<7)return 0;
    if((ebx7&(1u<<16))!=0&&(xcr0&0xE6u)==0xE6u)return 2;
    return ((ebx7&(1u<<5))!=0&&fma)?1:0;
}
inline int cpu_simd() noexcept {
    int r[4]{};__cpuid(r,1);const unsigned ecx1=unsigned(r[2]);
    const unsigned long long x=(ecx1&(1u<<27))?_xgetbv(0):0ull;
    int m[4]{};__cpuid(m,0);int e[4]{};if(m[0]>=7)__cpuidex(e,7,0);
    return nivel_simd(ecx1,unsigned(e[1]),x,m[0]);
}
inline const char* nome_simd(int nivel) noexcept {return nivel>=2?"AVX-512F":nivel==1?"AVX2+FMA":"sem AVX2+FMA";}
}
