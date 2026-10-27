#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace nrcabseg {
struct Area {float x{},y{},rx{},ry{};bool elipse{};};
struct Estado {bool tem{};Area a{};float vx{},vy{};std::uint64_t quadro{},ger{};unsigned idade{};bool atual_ant{};bool saida_ok{};Area saida{};};

inline bool passo(Estado& s,bool atual,const Area& a,std::uint64_t quadro,std::uint64_t ger,unsigned N,bool fora,float w,float h,Area& out) noexcept {
    if(quadro&&quadro==s.quadro){if(s.saida_ok)out=s.saida;return s.saida_ok;}
    const unsigned dq=s.quadro&&quadro>s.quadro?unsigned((std::min<std::uint64_t>)(quadro-s.quadro,64u)):1u;
    s.quadro=quadro;s.saida_ok=false;
    if(fora){s.tem=false;s.atual_ant=false;return false;}
    if(atual){
        const float meia=.5f*(std::max)((std::max)(a.rx,a.ry),1.f);
        if(s.tem&&s.atual_ant&&s.ger==ger&&std::hypot(a.x-s.a.x,a.y-s.a.y)<=meia){s.vx=.5f*s.vx+.5f*(a.x-s.a.x)/float(dq);s.vy=.5f*s.vy+.5f*(a.y-s.a.y)/float(dq);}
        else{s.vx=s.vy=0.f;}
        s.tem=true;s.a=a;s.idade=0;s.ger=ger;s.atual_ant=true;
        s.saida=a;s.saida_ok=true;out=a;return true;}
    s.atual_ant=false;
    if(!s.tem||!N||ger!=s.ger){s.tem=false;return false;}
    s.idade+=dq;if(s.idade>N){s.tem=false;return false;}
    for(unsigned k=0;k<dq;++k){s.a.x+=s.vx;s.a.y+=s.vy;s.vx*=.9f;s.vy*=.9f;}
    if(w>0.f)s.a.x=std::clamp(s.a.x,0.f,w);if(h>0.f)s.a.y=std::clamp(s.a.y,0.f,h);
    const unsigned F=(std::max)(1u,N/3u);
    const float e=s.idade<=N-F?1.f:float(N+1u-s.idade)/float(F+1u);
    Area o=s.a;o.rx*=e;o.ry*=e;
    s.saida=o;s.saida_ok=true;out=o;return true;}
}
