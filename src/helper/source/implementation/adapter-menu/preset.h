#pragma once

#include <cmath>
namespace nrpreset {
inline constexpr unsigned personalizado=3;
inline constexpr double local_structure=0.88,skin_structure=1.0,local_tone=1.0;
inline constexpr unsigned auto_mask=1,passes=2,tone_channels=1,tone_config=5;

struct Leitura {
    bool tem_estilo{},tem_local{},tem_skin{},tem_mascara{},tem_passes{},tem_tom{},tem_canais{},tem_config{};
    int estilo{};double local{},skin{};unsigned mascara{},pass_config{};double tom{};unsigned canais{},config{};
};
inline bool perto(double a,double b){return std::fabs(a-b)<5e-4;}

inline bool tom_do_preset(const Leitura& l){
    if(l.tem_config&&l.config==tone_config)return true;
    return (!l.tem_config||l.config==0)&&l.tem_tom&&perto(l.tom,local_tone)&&l.tem_canais&&l.canais==tone_channels;
}

inline unsigned detectar(const Leitura& l,bool tone_curve_active=false){
    if(!l.tem_estilo||l.estilo<0||l.estilo>2)return personalizado;
    if(!l.tem_local||!perto(l.local,local_structure)||!l.tem_skin||!perto(l.skin,skin_structure))return personalizado;
    if(!l.tem_mascara||l.mascara!=auto_mask)return personalizado;
    if(!l.tem_passes||(l.pass_config&255u)!=(tone_curve_active?1u:passes))return personalizado;
    if(!tom_do_preset(l))return personalizado;
    return unsigned(l.estilo);
}

inline unsigned pass_config(bool tem,unsigned atual,bool tone_curve_active=false){
    (void)tem;(void)atual;
    return 0x300u|(tone_curve_active?1u:passes);
}

inline unsigned proximo(unsigned atual,int delta){return (atual%4u+(delta<0?3u:1u))%4u;}
}
