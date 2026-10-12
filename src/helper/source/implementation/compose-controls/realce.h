#pragma once

#include "control_state.h"
#include <algorithm>
#include <cmath>
namespace nrrealce {
inline constexpr float luz_padrao=3.f,detalhe_padrao=1.3f,sem_limite=1000.f;
inline double ln_parte(float v){return v>=sem_limite ? std::log(64.) : std::log(double(std::max(v,1.f)));}
inline double fator(float luz,float detalhe){
    return (ln_parte(luz)+ln_parte(detalhe))/(std::log(double(luz_padrao))+std::log(double(detalhe_padrao)));}
inline bool formula_daniel(unsigned sel){return nrcompose::detail(sel)==20u&&nrcompose::color(sel)==20u&&nrcompose::guard(sel)==40u;}

inline unsigned selecao(unsigned sel,float luz,float detalhe){
    if(!nrcompose::valid(sel)||!(luz>1.f)||!(detalhe>1.f))return sel;
    if(luz==luz_padrao&&detalhe==detalhe_padrao)return sel;
    const unsigned di=nrcompose::detail(sel),co=nrcompose::color(sel),gu=nrcompose::guard(sel);
    const bool unitario=di==20u&&co==20u;
    auto com=[&](unsigned g){return (sel&~(255u<<21))|(g<<21);};
    const double k=fator(luz,detalhe);
    if(formula_daniel(sel)&&k>=1.)return sel;
    if(luz>=sem_limite&&detalhe>=sem_limite)return com(unitario?40u:160u);
    const double g0=formula_daniel(sel)?std::log(8.):std::log(gu/20.);
    if(!(g0>0.))return sel;
    const double G=std::exp(g0*k)*20.;
    unsigned g=G>=160. ? 160u : G<=21. ? 21u : unsigned(std::lround(G));
    if(unitario&&g==40u)g=41u;
    return com(g);
}

inline unsigned selecao_dividida(unsigned sel,float luz,float detalhe){
    if(!nrcompose::valid(sel)||!(luz>1.f)||!(detalhe>1.f))return sel;
    if(formula_daniel(sel)&&fator(luz,detalhe)<1.)return (sel&~(255u<<21))|(41u<<21);
    return sel;
}

inline unsigned tom(unsigned tone,float luz,float detalhe){return selecao(tone&~0xffu,luz,detalhe)|(tone&0xffu);}
inline unsigned tom_dividida(unsigned tone,float luz,float detalhe){return selecao_dividida(tone&~0xffu,luz,detalhe)|(tone&0xffu);}

inline bool luz_legada(double v){return v>=0.&&v<1.;}
}
