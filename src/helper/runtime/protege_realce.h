#pragma once

#include <algorithm>
#include <cmath>
#include <cwchar>
namespace nrprotreal {
struct Cfg { bool ligado=true; float ini=.55f,fim=.85f; };
inline float ini_de(unsigned c) noexcept {return .30f+.05f*float(c&15u);}
inline float fim_de(unsigned c) noexcept {return .30f+.05f*float((c>>4)&15u);}
inline unsigned codigo(const Cfg& c) noexcept {
    if(!c.ligado)return 0u;
    auto q=[](float v){return long(std::lround((double(v)-.30)/.05));};
    long a=q(c.ini),b=q(c.fim);
    if(!(std::isfinite(c.ini)&&std::isfinite(c.fim)&&c.ini>=.30f-1e-6f&&c.fim<=1.05f+1e-6f&&c.ini<c.fim)||a<0||b>15||a>=b){a=5;b=11;}
    return unsigned(a)|(unsigned(b)<<4);}

template<class F> Cfg ler(F&& ler_ini) {
    Cfg c;double v=0.;
    if(ler_ini(L"NeuralProtegeRealce",v))c.ligado=v>.5;
    double a=c.ini,b=c.fim;
    if(ler_ini(L"NeuralProtegeRealceIni",v))a=v;
    if(ler_ini(L"NeuralProtegeRealceFim",v))b=v;
    if(std::isfinite(a)&&std::isfinite(b)&&a>=.30-1e-9&&b<=1.05+1e-9&&a<b){c.ini=float(a);c.fim=float(b);}
    return c;}

inline void aplicar(unsigned codigo_,const float b[3],float d[3]) noexcept {
    if(codigo_==0u)return;
    const float a=ini_de(codigo_),z=fim_de(codigo_);
    const float t=std::clamp((std::max(b[0],std::max(b[1],b[2]))-a)/(z-a),0.f,1.f),w=1.f-t*t*(3.f-2.f*t);
    for(int k=0;k<3;++k)if(d[k]<0.f)d[k]*=w;}
}
