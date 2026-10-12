#pragma once
#include "../../../runtime/nr_publico.h"

namespace nrsuave {
struct Config {
    float forca = 1.f, entrada = 4.f, residuo = 10.f, mov = 0.f;
    bool escuro = true;
    unsigned banda = 0;
    bool ligado() const { return forca > 0.f && forca < 1.f; }
};

template <class Ler> Config ler(Ler&& ler_ini) {
    Config c;
    double v = 0.;
    if (ler_ini(L"NeuralSuave", v) && v > 0. && v < 1.) c.forca = float(v);
    v = 4.;  if (ler_ini(L"NeuralSuaveEntrada", v) && v > 0. && v <= 255.) c.entrada = float(v);
    v = 10.; if (ler_ini(L"NeuralSuaveResiduo", v) && v >= 0. && v <= 255.) c.residuo = float(v);
    v = 0.;  if (ler_ini(L"NeuralSuaveMov", v) && v >= 0. && v <= 64.) c.mov = float(v);
    v = 1.;  if (ler_ini(L"NeuralSuaveEscuro", v)) c.escuro = v > .5;
    v = 0.;  if (ler_ini(L"NeuralSuaveBanda", v)) c.banda = v > .5 ? 1u : 0u;
    return c;
}

struct Constantes {
    unsigned largura, altura, fatia, fatiaAnterior, valido, tonemap, escuro, banda;
    float forca, limiarEntrada, limiarResiduo, expo, profRel, profAbs, refMov, escuroBaixo, escuroAlto, r0, r1, r2;
};
static_assert(sizeof(Constantes) == 80, "suave.hlsl: 20 constantes");
inline Constantes constantes(const Config& c, unsigned w, unsigned h, unsigned fatia, unsigned anterior, bool valido, unsigned tom,
                             float expo, float prof_rel, float prof_abs) {
    return Constantes{w, h, fatia, anterior, valido ? 1u : 0u, tom & 1u, c.escuro ? 1u : 0u, c.banda,
                      c.forca, c.entrada / 255.f, c.residuo / 255.f, expo, prof_rel, prof_abs, c.mov, 3.f / 255.f, 12.f / 255.f, 0.f, 0.f, 0.f};
}
}
