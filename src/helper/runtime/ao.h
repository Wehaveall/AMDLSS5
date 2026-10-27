#pragma once

#include "nr_publico.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include "ini_jogo.h"
namespace nrao {
inline constexpr double pi=3.14159265358979323846;

struct Lin { float c=0.f,e=1.f; };
inline Lin lin_fsr(float near_,float far_,bool inv,bool inf) noexcept {
    float mn=(std::min)(near_,far_),mx=(std::max)(near_,far_);if(inv){const float t=mn;mn=mx;mx=t;}
    const float q=mx/(mn-mx);Lin l;
    if(!inv&&!inf){l.c=q;l.e=q*mn;}else if(!inv&&inf){l.c=-1.f-FLT_EPSILON;l.e=-mn-FLT_EPSILON;}
    else if(inv&&!inf){l.c=q;l.e=q*mn;}else{l.c=FLT_EPSILON;l.e=mx;}
    return l;}
inline double z_lin(double d,const Lin& l) noexcept {return std::fabs(double(l.e)/(d+double(l.c)));}

inline double z_fechado(double d,double n,double f,bool inv,bool inf) noexcept {
    if(inv)return inf?n/d:n*f/(n+d*(f-n));
    return inf?n/(1.-d):n*f/(f-d*(f-n));}

struct Camera { float near_=.1f,far_=0.f,fov=1.0471976f; bool inv=true,inf=true,fov_ini=false,near_ruim=false; float metros=1.f; };
inline Camera resolver_camera(float near_,float far_,float fov,int invertida,int infinita,float fov_ini_graus,float metros_ini) noexcept {
    Camera c;
    const float fmin=5.f*float(pi)/180.f,fmax=150.f*float(pi)/180.f;
    if(std::isfinite(fov)&&fov>=fmin&&fov<=fmax)c.fov=fov;else{c.fov_ini=true;const float g=std::isfinite(fov_ini_graus)&&fov_ini_graus>=5.f&&fov_ini_graus<=150.f?fov_ini_graus:60.f;c.fov=g*float(pi)/180.f;}
    float a=near_,b=far_;
    if(!(std::isfinite(a)&&a>0.f)){a=.1f;c.near_ruim=true;}

    if(std::isfinite(b)&&b>0.f&&b<a){const float t=a;a=b;b=t;}
    c.near_=a;c.far_=b;
    c.inv=invertida!=0;
    const bool far_ruim=!(std::isfinite(b)&&b>a*1.0001f&&b<a*1e6f);
    c.inf=infinita<0?far_ruim:(infinita!=0||far_ruim);
    if(std::isfinite(metros_ini)&&metros_ini>0.f&&metros_ini<=100.f)c.metros=metros_ini;else c.metros=a>=1.f?.01f:1.f;
    return c;}

inline unsigned hilbert(unsigned x,unsigned y) noexcept {
    x&=63u;y&=63u;unsigned d=0;
    for(unsigned s=32u;s>0u;s>>=1){
        const unsigned rx=(x&s)?1u:0u,ry=(y&s)?1u:0u;d+=s*s*((3u*rx)^ry);
        if(ry==0u){if(rx==1u){x=s-1u-(x&(s-1u));y=s-1u-(y&(s-1u));}else{x&=s-1u;y&=s-1u;}const unsigned t=x;x=y;y=t;}else{x&=s-1u;y&=s-1u;}}
    return d;}

inline void ruido(unsigned x,unsigned y,unsigned quadro,float& a,float& b) noexcept {
    const unsigned i=hilbert(x,y)+288u*(quadro%64u);
    const double fa=.5+double(i)*0.7548776662466927,fb=.5+double(i)*0.5698402909980532;
    a=float(fa-std::floor(fa));b=float(fb-std::floor(fb));}

inline double iarc(double h,double n) noexcept {return (std::cos(n)+2.*h*std::sin(n)-std::cos(2.*h-n))/4.;}

inline double A(double h,double n) noexcept {return h>=0.?iarc(h,n):-iarc(h,n);}
inline double T(double n) noexcept {return std::cos(n)+n*std::sin(n);}

inline double u_unif(double h,double n) noexcept {const double u=(h-(n-pi/2.))/pi;return u<0.?0.:u>1.?1.:u;}
inline double u_cos(double h,double n) noexcept {
    const double lo=n-pi/2.,hi=n+pi/2.;h=h<lo?lo:h>hi?hi:h;const double t=T(n);if(!(t>1e-9))return u_unif(h,n);
    const double u=(A(h,n)-A(lo,n))/t;return u<0.?0.:u>1.?1.:u;}

inline std::uint32_t setores(double umin,double umax) noexcept {
    if(umax<umin){const double t=umin;umin=umax;umax=t;}if(umax-umin<.5/32.)return 0u;
    unsigned a=unsigned(std::floor(umin*32.));if(a>31u)a=31u;unsigned b=unsigned(std::ceil((umax-umin)*32.));if(b>32u-a)b=32u-a;
    return b>=32u?0xFFFFFFFFu:(((1u<<b)-1u)<<a);}
inline unsigned bits(std::uint32_t m) noexcept {unsigned c=0;while(m){m&=m-1u;++c;}return c;}

inline double fatia_classica(double n,double proj,double h0,double h1) noexcept {
    return (proj+(1.-proj)*.05)*(iarc(h0,n)+iarc(h1,n));}

struct Ocl { double frente,tras; };
inline double fatia_mascara(double n,double proj,const Ocl* o,unsigned k,int modo) noexcept {
    std::uint32_t m=0;
    for(unsigned i=0;i<k;++i){const double a=(std::min)(o[i].frente,o[i].tras),b=(std::max)(o[i].frente,o[i].tras);
        m|=modo==1?setores(u_unif(a,n),u_unif(b,n)):setores(u_cos(a,n),u_cos(b,n));}
    const double livre=1.-double(bits(m))/32.;
    return modo==1?livre:(proj+(1.-proj)*.05)*T(n)*livre;}

struct V3 { double x,y,z; };
inline V3 sub(V3 a,V3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V3 cruz(V3 a,V3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline V3 norm(V3 a){const double l=std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);return l>0?V3{a.x/l,a.y/l,a.z/l}:a;}
inline double sat(double v){return v<0.?0.:v>1.?1.:v;}

inline void bordas(double zc,double zl,double zr,double zt,double zb,double e[4]) noexcept {
    double d[4]{zl-zc,zr-zc,zt-zc,zb-zc};const double slr=(d[1]-d[0])*.5,stb=(d[3]-d[2])*.5;const double a[4]{d[0]+slr,d[1]-slr,d[2]+stb,d[3]-stb};
    for(int i=0;i<4;++i)e[i]=sat(1.25-(std::min)(std::fabs(d[i]),std::fabs(a[i]))/(zc*.011));}
inline V3 normal(const double e[4],V3 c,V3 l,V3 r,V3 t,V3 b) noexcept {
    const double w[4]{sat(e[0]*e[2]+.01),sat(e[2]*e[1]+.01),sat(e[1]*e[3]+.01),sat(e[3]*e[0]+.01)};
    l=norm(sub(l,c));r=norm(sub(r,c));t=norm(sub(t,c));b=norm(sub(b,c));
    const V3 a=cruz(l,t),bb=cruz(t,r),cc=cruz(r,b),dd=cruz(b,l);
    return norm({w[0]*a.x+w[1]*bb.x+w[2]*cc.x+w[3]*dd.x,w[0]*a.y+w[1]*bb.y+w[2]*cc.y+w[3]*dd.y,w[0]*a.z+w[1]*bb.z+w[2]*cc.z+w[3]*dd.z});}

struct Predef { unsigned fatias,passos,denoise,quadros; };
inline Predef predef(unsigned q) noexcept {
    static constexpr Predef t[4]{{1,2,1,16},{2,2,1,16},{3,3,1,12},{4,4,2,8}};return t[q<4u?q:1u];}

inline unsigned passo_resolucao(unsigned modo,unsigned w,unsigned h) noexcept {
    if(modo==1u)return 1u;if(modo==2u)return 2u;return std::uint64_t(w)*h>std::uint64_t(2560u)*1440u?2u:1u;}

struct Opcoes {
    float forca=1.f; int esp_linear=0; int modo=2; float dist_ini=-1.f,dist_min=.2f,personagem=0.f,claro_ini=.5f,claro_fim=2.f;
    int temporal=1,quadros=32; float tol_prof=.06f,clamp_margem=.06f; int denoise=-1; float denoise_beta=1.2f; int upsample=0;
    float raio_mult=1.457f,falloff=.615f,distribuicao=2.f,finos=0.f,potencia=2.2f,mip_offset=3.3f,raio_max_px=48.f; int multibounce=0;
    float fov_graus=60.f,metros=0.f; int pre=0,infinita=-1,invertida=-1; float desjitter=1.f;
    int hist_jitter=1,prof_viz=1; float clamp_sigma=1.25f;
 int instavel=1,clamp_cru=1;
};
inline std::atomic<bool> opcoes_lidas{false};
inline Opcoes& opcoes_() noexcept {static Opcoes o;return o;}
inline Opcoes opcoes() noexcept {return opcoes_();}

inline constexpr const wchar_t* chaves_ini[]{L"NeuralFiltroAoForca",L"NeuralFiltroAoEspessuraLinear",L"NeuralFiltroAoModo",L"NeuralFiltroAoDistIni",
    L"NeuralFiltroAoDistMin",L"NeuralFiltroAoPersonagem",L"NeuralFiltroAoClaroIni",L"NeuralFiltroAoClaroFim",L"NeuralFiltroAoTemporal",L"NeuralFiltroAoQuadros",
    L"NeuralFiltroAoTolProf",L"NeuralFiltroAoClampMargem",L"NeuralFiltroAoDenoise",L"NeuralFiltroAoDenoiseBeta",L"NeuralFiltroAoUpsample",L"NeuralFiltroAoRaioMult",
    L"NeuralFiltroAoFalloff",L"NeuralFiltroAoDistribuicao",L"NeuralFiltroAoFinos",L"NeuralFiltroAoPotencia",L"NeuralFiltroAoMipOffset",L"NeuralFiltroAoRaioMaxPx",
    L"NeuralFiltroAoMultiBounce",L"NeuralFiltroAoFovGraus",L"NeuralFiltroAoMetros",L"NeuralFiltroAoPre",L"NeuralFiltroAoInfinita",L"NeuralFiltroAoInvertida",
    L"NeuralFiltroAoDesjitter",L"NeuralFiltroAoHistJitter",L"NeuralFiltroAoProfVizinhanca",L"NeuralFiltroAoClampSigma",L"NeuralFiltroAoInstavel",L"NeuralFiltroAoClampCru"};
inline Opcoes ler_opcoes(const wchar_t* ini) noexcept {
    Opcoes o;if(!ini)return o;
    auto f=[&](const wchar_t* k,float& v,double lo,double hi){double x=0.;if(nrinijogo::ler_double_em(ini,k,x,32)&&x>=lo&&x<=hi)v=float(x);};
    auto i=[&](const wchar_t* k,int& v,int lo,int hi){double x=0.;if(nrinijogo::ler_double_em(ini,k,x,32)){const long n=std::lround(x);if(double(n)==x&&n>=lo&&n<=hi)v=int(n);}};
    f(L"NeuralFiltroAoForca",o.forca,0,1);i(L"NeuralFiltroAoEspessuraLinear",o.esp_linear,0,1);i(L"NeuralFiltroAoModo",o.modo,0,2);
    f(L"NeuralFiltroAoDistIni",o.dist_ini,0,500);f(L"NeuralFiltroAoDistMin",o.dist_min,0,10);f(L"NeuralFiltroAoPersonagem",o.personagem,0,1);
    f(L"NeuralFiltroAoClaroIni",o.claro_ini,0,8);f(L"NeuralFiltroAoClaroFim",o.claro_fim,0,8);if(!(o.claro_fim>o.claro_ini)){o.claro_ini=.5f;o.claro_fim=2.f;}
    i(L"NeuralFiltroAoTemporal",o.temporal,0,1);i(L"NeuralFiltroAoQuadros",o.quadros,2,32);f(L"NeuralFiltroAoTolProf",o.tol_prof,.005,.2);
    f(L"NeuralFiltroAoClampMargem",o.clamp_margem,0,.2);i(L"NeuralFiltroAoDenoise",o.denoise,0,3);f(L"NeuralFiltroAoDenoiseBeta",o.denoise_beta,.5,10);
    i(L"NeuralFiltroAoUpsample",o.upsample,0,3);f(L"NeuralFiltroAoRaioMult",o.raio_mult,.3,3);f(L"NeuralFiltroAoFalloff",o.falloff,.01,1);
    f(L"NeuralFiltroAoDistribuicao",o.distribuicao,1,3);f(L"NeuralFiltroAoFinos",o.finos,0,.7);f(L"NeuralFiltroAoPotencia",o.potencia,.5,5);
    f(L"NeuralFiltroAoMipOffset",o.mip_offset,2,6);f(L"NeuralFiltroAoRaioMaxPx",o.raio_max_px,16,512);i(L"NeuralFiltroAoMultiBounce",o.multibounce,0,1);
    f(L"NeuralFiltroAoFovGraus",o.fov_graus,20,120);f(L"NeuralFiltroAoMetros",o.metros,0,100);i(L"NeuralFiltroAoPre",o.pre,0,1);
    i(L"NeuralFiltroAoInfinita",o.infinita,-1,1);i(L"NeuralFiltroAoInvertida",o.invertida,-1,1);f(L"NeuralFiltroAoDesjitter",o.desjitter,-1,1);
    i(L"NeuralFiltroAoHistJitter",o.hist_jitter,0,1);i(L"NeuralFiltroAoProfVizinhanca",o.prof_viz,0,1);f(L"NeuralFiltroAoClampSigma",o.clamp_sigma,0,4);
    i(L"NeuralFiltroAoInstavel",o.instavel,0,1);i(L"NeuralFiltroAoClampCru",o.clamp_cru,0,1);
    return o;}
inline void recarregar(const wchar_t* ini) noexcept {opcoes_()=ler_opcoes(ini);opcoes_lidas.store(true,std::memory_order_release);}

inline void restaurar(const wchar_t* ini) noexcept {
    opcoes_()=Opcoes{};if(!ini)return;for(const wchar_t* k:chaves_ini)NRCFG_ESCREVER(L"DlssNrOnAmd",k,nullptr,ini);}

struct Entrada {
    bool valido=false;void* prof=nullptr;void* mov=nullptr;void* cor_entrada=nullptr;
    unsigned prof_estado=0,mov_estado=0,cor_estado=0;float mv_x=0.f,mv_y=0.f;unsigned render_w=0,render_h=0;float jitter_x=0.f,jitter_y=0.f;
    float near_=0.f,far_=0.f,fov=0.f;int invertida=-1;bool reset=false;
};
inline Entrada entrada{};
}
