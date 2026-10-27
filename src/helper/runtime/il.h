#pragma once

#include "ao.h"
#include <cmath>
#include <cstdint>
#include <cstring>
namespace nril {
using nrao::pi;using nrao::T;

struct Predef { unsigned fatias,passos; };
inline Predef predef(unsigned q) noexcept {static constexpr Predef t[4]{{1,4},{2,4},{3,6},{4,8}};return t[q<4u?q:1u];}

inline unsigned passo_resolucao(int modo) noexcept {return modo==1?1u:modo==2?4u:2u;}

inline unsigned rodizio(unsigned atual) noexcept {return atual==0u?1u:atual==2u?4u:2u;}
inline unsigned quadros_padrao(unsigned R) noexcept {return R<=1u?16u:R==2u?12u:8u;}

inline void despacho(unsigned Wi,unsigned Hi,unsigned R,unsigned& tx,unsigned& ty) noexcept {
    if(R<=1u){tx=Wi;ty=Hi;}else if(R==2u){tx=(Wi+1u)/2u;ty=Hi;}else{tx=(Wi+1u)/2u;ty=(Hi+1u)/2u;}}
inline constexpr unsigned bayer_x[4]{0u,1u,1u,0u},bayer_y[4]{0u,1u,0u,1u};
inline void mapa(unsigned tx,unsigned ty,unsigned fase,unsigned R,unsigned& px,unsigned& py) noexcept {
    if(R<=1u){px=tx;py=ty;}else if(R==2u){px=2u*tx+((ty+fase)&1u);py=ty;}else{px=2u*tx+bayer_x[fase&3u];py=2u*ty+bayer_y[fase&3u];}}
inline bool novo(unsigned px,unsigned py,unsigned fase,unsigned R) noexcept {
    if(R<=1u)return true;if(R==2u)return ((px+py+fase)&1u)==0u;return (px&1u)==bayer_x[fase&3u]&&(py&1u)==bayer_y[fase&3u];}

inline double lum(const double c[3]) noexcept {return .2126*c[0]+.7152*c[1]+.0722*c[2];}

inline void fonte_linear(const double c[3],bool hdr,double o[3]) noexcept {
    for(int i=0;i<3;++i)if(!std::isfinite(c[i])){o[0]=o[1]=o[2]=0.;return;}
    for(int i=0;i<3;++i){double x=c[i]>0.?c[i]:0.;if(!hdr){x=std::pow(x<1.?x:1.,2.2);x=x/(1.1-x);}o[i]=x;}}

inline void vagalume(const double L[3],double Ybar,double rel,double o[3]) noexcept {
    const double lim=(std::max)(rel*Ybar,1e-30),k=1./(1.+lum(L)/lim);for(int i=0;i<3;++i)o[i]=L[i]*k;}

inline void albedo(const double lin[3],double Ybar,double m,double rmed,double rmax,double a[3]) noexcept {
    const double Y=lum(lin);const double E=(std::max)((std::max)(std::pow(Y,1.-m)*std::pow((std::max)(Ybar,0.),m)/rmed,Y/rmax),1e-6*Ybar);
    for(int i=0;i<3;++i)a[i]=E>0.?lin[i]/E:0.;}

inline void energia(const double lin[3],double fao,const double dC[3],double brilho,double o[3]) noexcept {
    double c1[3],b[3];for(int i=0;i<3;++i){b[i]=lin[i]*fao;c1[i]=b[i]+dC[i];}
    const double alvo=lum(b)+brilho*lum(dC),l1=lum(c1);const double k=alvo/(std::max)(l1,1e-6*alvo+1e-12);for(int i=0;i<3;++i)o[i]=c1[i]*k;}

struct AmostraIl { double frente,tras; double L[3]; double g,wgt,ndl; };
inline void fatia_il(double n,double proj,const AmostraIl* s,unsigned k,int modo,double o[3]) noexcept {
    std::uint32_t masc=0;double acc[3]{0.,0.,0.};
    for(unsigned i=0;i<k;++i){const double a=(std::min)(s[i].frente,s[i].tras),b=(std::max)(s[i].frente,s[i].tras);
        const std::uint32_t bits=modo==1?nrao::setores(nrao::u_unif(a,n),nrao::u_unif(b,n)):nrao::setores(nrao::u_cos(a,n),nrao::u_cos(b,n));
        const std::uint32_t nv=bits&~masc;masc|=bits;const unsigned kn=nrao::bits(nv);
        if(kn&&s[i].wgt>0.){double w=double(kn)/32.*s[i].wgt*s[i].g;if(modo==1)w*=(std::max)(0.,(std::min)(1.,s[i].ndl))*pi/2.;for(int c=0;c<3;++c)acc[c]+=w*s[i].L[c];}}
    const double f=modo==1?1.:(proj+(1.-proj)*.05)*T(n);for(int c=0;c<3;++c)o[c]=acc[c]*f;}

inline std::uint16_t meia(float v) noexcept {
    if(!std::isfinite(v))return v>0.f?0x7c00u:0u;if(v<0.f)v=0.f;if(v>65504.f)return 0x7bffu;
    std::uint32_t u;std::memcpy(&u,&v,4);const int e=int((u>>23)&255u)-127;if(e<-24)return 0u;
    if(e<-14){const std::uint32_t m=(u&0x7fffffu)|0x800000u;const int sh=-e-14+13;const std::uint32_t r=(m+(1u<<(sh-1)))>>sh;return std::uint16_t(r);}
    std::uint32_t h=(std::uint32_t(e+15)<<10)|((u>>13)&1023u);if(u&0x1000u)++h;return std::uint16_t(h>0x7bffu?0x7bffu:h);}
inline std::uint32_t meia2(float a,float b) noexcept {return std::uint32_t(meia(a))|(std::uint32_t(meia(b))<<16);}

struct Opcoes {
    float raio=-1.f,espessura=-1.f; int modo=2,resolucao=0,fonte=0,rebatimentos=1; float rebate_forca=.5f; int quadros=-1;
    float clamp_sigma=1.25f,tol_prof=.03f; int atrous=2,albedo=0; float albedo_viz=-1.f,albedo_medio=.4f,albedo_max=.9f,luz_max_rel=8.f;
    float claro=.5f,claro_ini=2.f,claro_fim=6.f,claro_max=2.f; int ceu=0; float costas=0.f; int normal_emissor=0; float mip_bias=1.f,personagem=0.f; int tudo_cada_n=0;
    int emissor=1,continuo=1,passos_longe=-1; float raio_longe=-1.f,teto_longe_px=540.f,raio_max_px=-1.f;
};

struct Estilo { float ganho,raio,espessura,teto_px,raio_longe,albedo_viz; unsigned passos_longe; };
inline Estilo estilo(unsigned i) noexcept {
    static constexpr Estilo t[3]{{.6f,1.5f,.35f,192.f,3.f,.5f,2u},{1.f,2.5f,.5f,256.f,4.f,.4f,3u},{1.6f,3.5f,.75f,320.f,5.f,.25f,4u}};return t[i<3u?i:1u];}

struct Efetivas { float ganho,raio,espessura,teto_px,raio_longe,albedo_viz; unsigned passos_longe; };
inline Efetivas efetivas(const Opcoes& o,unsigned est) noexcept {
    const Estilo e=estilo(est);
    return {e.ganho,o.raio>=0.f?o.raio:e.raio,o.espessura>=0.f?o.espessura:e.espessura,o.raio_max_px>0.f?o.raio_max_px:e.teto_px,o.raio_longe>0.f?o.raio_longe:e.raio_longe,
            o.albedo_viz>=0.f?o.albedo_viz:e.albedo_viz,o.passos_longe>=0?unsigned(o.passos_longe):e.passos_longe};}
inline std::atomic<bool> opcoes_lidas{false};
inline Opcoes& opcoes_() noexcept {static Opcoes o;return o;}
inline Opcoes opcoes() noexcept {return opcoes_();}
inline constexpr const wchar_t* chaves_ini[]{L"NeuralFiltroIlRaio",L"NeuralFiltroIlEspessura",L"NeuralFiltroIlModo",L"NeuralFiltroIlResolucao",L"NeuralFiltroIlFonte",
    L"NeuralFiltroIlRebatimentos",L"NeuralFiltroIlRebateForca",L"NeuralFiltroIlQuadros",L"NeuralFiltroIlClampSigma",L"NeuralFiltroIlTolProf",L"NeuralFiltroIlAtrous",
    L"NeuralFiltroIlAlbedo",L"NeuralFiltroIlAlbedoVizinhanca",L"NeuralFiltroIlAlbedoMedio",L"NeuralFiltroIlAlbedoMax",L"NeuralFiltroIlLuzMaxRel",L"NeuralFiltroIlClaro",
    L"NeuralFiltroIlClaroIni",L"NeuralFiltroIlClaroFim",L"NeuralFiltroIlClaroMax",L"NeuralFiltroIlCeu",L"NeuralFiltroIlCostas",L"NeuralFiltroIlNormalEmissor",
    L"NeuralFiltroIlMipBias",L"NeuralFiltroIlPersonagem",L"NeuralFiltroIlTudoCadaN",
    L"NeuralFiltroIlEmissor",L"NeuralFiltroIlContinuo",L"NeuralFiltroIlPassosLonge",L"NeuralFiltroIlRaioLonge",L"NeuralFiltroIlTetoLongePx",L"NeuralFiltroIlRaioMaxPx"};
inline Opcoes ler_opcoes(const wchar_t* ini) noexcept {
    Opcoes o;if(!ini)return o;
    auto f=[&](const wchar_t* k,float& v,double lo,double hi,double esc=1.){double x=0.;if(nrinijogo::ler_double_em(ini,k,x,32)&&x>=lo&&x<=hi)v=float(x*esc);};
    auto i=[&](const wchar_t* k,int& v,int lo,int hi){double x=0.;if(nrinijogo::ler_double_em(ini,k,x,32)){const long n=std::lround(x);if(double(n)==x&&n>=lo&&n<=hi)v=int(n);}};
    {double x=0.;if(nrinijogo::ler_double_em(ini,L"NeuralFiltroIlRaio",x,32)&&(x==0.||(x>=.1&&x<=8.)))o.raio=float(x);}
    {double x=0.;if(nrinijogo::ler_double_em(ini,L"NeuralFiltroIlEspessura",x,32)&&(x==0.||(x>=.02&&x<=2.)))o.espessura=float(x);}
    i(L"NeuralFiltroIlModo",o.modo,1,2);i(L"NeuralFiltroIlResolucao",o.resolucao,0,2);i(L"NeuralFiltroIlFonte",o.fonte,0,2);i(L"NeuralFiltroIlRebatimentos",o.rebatimentos,1,2);
    f(L"NeuralFiltroIlRebateForca",o.rebate_forca,0,1);i(L"NeuralFiltroIlQuadros",o.quadros,2,32);f(L"NeuralFiltroIlClampSigma",o.clamp_sigma,.5,3);
    f(L"NeuralFiltroIlTolProf",o.tol_prof,.005,.2);i(L"NeuralFiltroIlAtrous",o.atrous,0,3);i(L"NeuralFiltroIlAlbedo",o.albedo,0,2);
    f(L"NeuralFiltroIlAlbedoVizinhanca",o.albedo_viz,0,1);f(L"NeuralFiltroIlAlbedoMedio",o.albedo_medio,.1,.8);f(L"NeuralFiltroIlAlbedoMax",o.albedo_max,.5,1);
    f(L"NeuralFiltroIlLuzMaxRel",o.luz_max_rel,2,64);f(L"NeuralFiltroIlClaro",o.claro,0,100,.01);
    f(L"NeuralFiltroIlClaroIni",o.claro_ini,.1,64);f(L"NeuralFiltroIlClaroFim",o.claro_fim,.1,64);if(!(o.claro_fim>o.claro_ini)){o.claro_ini=2.f;o.claro_fim=6.f;}
    f(L"NeuralFiltroIlClaroMax",o.claro_max,.5,8);i(L"NeuralFiltroIlCeu",o.ceu,0,1);f(L"NeuralFiltroIlCostas",o.costas,0,1);i(L"NeuralFiltroIlNormalEmissor",o.normal_emissor,0,1);
    f(L"NeuralFiltroIlMipBias",o.mip_bias,0,3);f(L"NeuralFiltroIlPersonagem",o.personagem,0,100,.01);i(L"NeuralFiltroIlTudoCadaN",o.tudo_cada_n,0,1);
    i(L"NeuralFiltroIlEmissor",o.emissor,0,1);i(L"NeuralFiltroIlContinuo",o.continuo,0,1);i(L"NeuralFiltroIlPassosLonge",o.passos_longe,0,6);
    f(L"NeuralFiltroIlRaioLonge",o.raio_longe,1.5,8);f(L"NeuralFiltroIlTetoLongePx",o.teto_longe_px,64,1080);f(L"NeuralFiltroIlRaioMaxPx",o.raio_max_px,16,1080);
    if(o.albedo_medio>o.albedo_max)o.albedo_medio=o.albedo_max;
    return o;}
inline void recarregar(const wchar_t* ini) noexcept {opcoes_()=ler_opcoes(ini);opcoes_lidas.store(true,std::memory_order_release);}

inline void restaurar(const wchar_t* ini) noexcept {opcoes_()=Opcoes{};if(!ini)return;for(const wchar_t* k:chaves_ini)NRCFG_ESCREVER(L"DlssNrOnAmd",k,nullptr,ini);}
}
