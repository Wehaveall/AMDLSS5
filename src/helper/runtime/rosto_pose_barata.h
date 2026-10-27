#pragma once

#include <algorithm>
#include <cmath>

namespace nrpose {

inline constexpr double razao_frente=.42;
inline float yaw_pontos(const float lm[10],float w,float h) noexcept {
    const double lf=(std::max)(double(w),double(h));
    if(!(lf>1.0))return -1.f;
    const double ol=std::hypot(double(lm[0])-double(lm[2]),double(lm[1])-double(lm[3]));
    if(!std::isfinite(ol)||!(ol>0.0))return -1.f;
    const double c=std::clamp(ol/lf/razao_frente,0.0,1.0);
    return float(std::acos(c)*57.29577951308232);
}

enum Amostra : int {Nada=0,Rosto=1,SemRosto=2,SemCaixa=3};

template<class F>
inline int rosto_dela(const F* rostos,int n,double esc,const double est[3],float nota_min,float rosto_cab,float rosto_dy) noexcept {
    if(!rostos||n<=0||!(est[2]>0.0))return -1;
    esc=(std::max)(esc,1e-3);int ki=-1;double kd=1e9;
    for(int i=0;i<n;++i){const auto& f=rostos[i];if(!(f.h>0.f)||!(f.nota>=nota_min))continue;
        const double fx=(f.x+.5*f.w)/esc,fy=(f.y+.5*f.h)/esc,fh=f.h/esc,y=fy-double(rosto_dy)*fh,l=double(rosto_cab)*fh;
        const double d=std::hypot(fx-est[0],y-est[1])/(std::max)(est[2],1.0);
        if(d<=.6&&l>=.6*est[2]&&l<=1.6*est[2]&&d<kd){kd=d;ki=i;}}
    return ki;
}

template<class F>
inline Amostra amostrar(bool lista_nova,bool tem_caixa,bool ext_tem,const F* rostos,int n,double esc,const double est[3],
                        float nota_min,float rosto_cab,float rosto_dy,float& yaw) noexcept {
    yaw=-1.f;
    if(!tem_caixa)return SemCaixa;
    if(!lista_nova)return Nada;
    const int i=rosto_dela(rostos,n,esc,est,nota_min,rosto_cab,rosto_dy);
    if(i>=0){yaw=yaw_pontos(rostos[i].lm,rostos[i].w,rostos[i].h);return yaw>=0.f?Rosto:Nada;}
    return ext_tem?SemRosto:Nada;
}

struct Classificador {
    bool longe{};unsigned seguidas{};float yaw{-1.f};int motivo{};unsigned long long trocas{};

    bool amostra(Amostra a,float yaw_g,float lim,unsigned n) noexcept {
        if(a==Nada)return false;
        const bool ant=longe;
        if(a==SemCaixa){seguidas=0;longe=false;motivo=3;yaw=-1.f;}
        else if(a==Rosto&&yaw_g<=lim){seguidas=0;longe=false;motivo=0;yaw=yaw_g;}
        else{if(seguidas<0xffffffffu)++seguidas;motivo=a==Rosto?1:2;yaw=a==Rosto?yaw_g:-1.f;if(seguidas>=(std::max)(n,1u))longe=true;}
        if(longe!=ant)++trocas;
        return longe!=ant;}
    void zerar() noexcept {longe=false;seguidas=0;yaw=-1.f;motivo=0;}
};
inline const char* motivo_texto(int m) noexcept {return m==1?"perfil":m==2?"sem_rosto":m==3?"sem_caixa":"frente";}

inline bool pula(bool longe,unsigned seguidos,unsigned mult,int idade,unsigned q) noexcept {
    return longe&&mult>1u&&seguidos+1u<mult&&idade>=0&&unsigned(idade)+q+4u<16u;
}

}
