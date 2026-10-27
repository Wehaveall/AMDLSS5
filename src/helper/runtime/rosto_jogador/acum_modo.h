#pragma once

#include <algorithm>
#include <cmath>

namespace nrac {
struct Padroes {float lenta,rapida,mov_px,sigma;};
inline constexpr Padroes padroes_hoje{0.f,0.f,4.f,6.f};
inline constexpr Padroes padroes_inteiro{.05f,.6f,4.f,16.f};
inline constexpr Padroes padroes(bool inteiro) noexcept {return inteiro?padroes_inteiro:padroes_hoje;}
inline constexpr float sigma_teto_inteiro=16.f;

inline float por_quadro(float a,float kq) noexcept {a=std::clamp(a,.001f,1.f);return a>=1.f?1.f:1.f-std::pow(1.f-a,1.f/kq);}

struct TaxasBaixa {float a_baixa,a_lenta,a_rapida,mov_px;};
inline TaxasBaixa taxas_baixa(float acum_baixa,float lenta,float rapida,float mov_px,float kq) noexcept {
    TaxasBaixa t{por_quadro(acum_baixa,kq),0.f,0.f,0.f};
    if(lenta>0.f){t.a_lenta=por_quadro(lenta,kq);t.a_rapida=rapida>0.f?por_quadro(rapida,kq):t.a_baixa;t.mov_px=mov_px;}
    return t;}
}
