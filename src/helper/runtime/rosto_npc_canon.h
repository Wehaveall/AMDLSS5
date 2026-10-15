#pragma once

#include <algorithm>
#include <cmath>

namespace nrcanon {

struct Param {
    float fracao=.36f;
    float razao=2.35f;
    float min_corte=.6f;
    float beta=4.f;
    float d_corte=2.f;
    float rej_pos=.8f;
    float rej_esc=.45f;
    int rej_n=3;
    int rede=384;
};

inline bool medida(const float* lm,double& cx,double& cy,double& s) noexcept {
    for(int i=0;i<10;++i)if(!std::isfinite(lm[i]))return false;
    const double ex=.5*(double(lm[0])+lm[2]),ey=.5*(double(lm[1])+lm[3]),mx=.5*(double(lm[6])+lm[8]),my=.5*(double(lm[7])+lm[9]);
    const double ee=std::hypot(double(lm[2])-lm[0],double(lm[3])-lm[1]),em=std::hypot(mx-ex,my-ey);
    s=(std::max)(2.0*ee,1.8*em);if(!(s>1e-3)||!(em>1e-3))return false;
    cx=ex+.1*(mx-ex);cy=ey+.1*(my-ey);return true;
}

inline double alfa(double corte,double dt) noexcept {const double tau=1.0/(2.0*3.14159265358979*(std::max)(corte,1e-3));return 1.0/(1.0+tau/(std::max)(dt,1e-4));}

struct Trilha {
    bool tem=false;double cx=0,cy=0,s=0;
    double dvx=0,dvy=0,dvs=0;
    double t_med=-1;
    double pf=0;
    int fora=0;unsigned long long medidas=0,rejeitadas=0,reinicios=0;
    void zera() noexcept {*this=Trilha{};}

    void prever(double mx,double my,double pf_agora) noexcept {
        if(!tem)return;cx-=mx;cy-=my;
        if(pf_agora>0&&pf>0){const double r=std::clamp(pf_agora/pf,.89,1.12);s*=r;pf=pf_agora;}else if(pf_agora>0)pf=pf_agora;}

    int medir(double zx,double zy,double zs,double t,double w,double pf_agora,const Param& P) noexcept {
        if(!(zs>0)||!std::isfinite(zx)||!std::isfinite(zy))return -1;
        if(!tem){tem=true;cx=zx;cy=zy;s=zs;dvx=dvy=dvs=0;t_med=t;pf=pf_agora;fora=0;++medidas;return 1;}
        const double ix=(zx-cx)/s,iy=(zy-cy)/s,is=std::log(zs/s);
        if(std::hypot(ix,iy)>double(P.rej_pos)||std::fabs(is)>double(P.rej_esc)){
            ++rejeitadas;if(++fora>=P.rej_n){tem=false;++reinicios;return medir(zx,zy,zs,t,w,pf_agora,P);}return -1;}
        fora=0;const double dt=(t_med>=0&&t>t_med)?std::clamp(t-t_med,1.0/240.0,.5):1.0/30.0;t_med=t;

        const double ad=alfa(double(P.d_corte),dt);
        dvx+=ad*(ix/dt-dvx);dvy+=ad*(iy/dt-dvy);dvs+=ad*(is/dt-dvs);
        const double wv=std::clamp(w,.05,1.0);
        const double ap=wv*alfa(double(P.min_corte)+double(P.beta)*std::hypot(dvx,dvy),dt),as=wv*alfa(double(P.min_corte)+double(P.beta)*std::fabs(dvs),dt);
        cx+=ap*ix*s;cy+=ap*iy*s;s*=std::exp(as*is);
        if(pf_agora>0)pf=pf_agora;++medidas;return 0;}
};

inline int lado(double s,double esc4k,const Param& P,int lmin,int lmax) noexcept {
    const double v=double(P.razao)*s*esc4k/(std::max)(double(P.fracao),.05);
    const int L=int(std::lround(v/4.0))*4;return std::clamp(L,lmin,(std::max)(lmin,lmax));
}

}
