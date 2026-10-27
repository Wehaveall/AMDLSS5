#pragma once

#include "reproject_controls.h"
#include "../compose-controls/realce.h"
#include <cwchar>
namespace nrreproject {
inline constexpr const wchar_t* chaves[ControlCount]={L"NeuralReprojectClampStrength",L"NeuralReprojectMotionDivergence",L"NeuralReprojectDepthRelative",
    L"NeuralReprojectColorThreshold",L"NeuralReprojectMotionFactor",L"NeuralReprojectDetailMotion",
    L"NeuralReprojectPointMotion",L"NeuralReprojectMotionReference",L"NeuralReprojectDepthAbsolute",
    L"NeuralAsyncMaxAge",L"NeuralReprojectComposeAfterUpscale",L"NeuralReprojectFillMode",
    L"NeuralReprojectRawDivergence",L"NeuralReprojectStabilize",L"NeuralGradeExposure",
    L"NeuralGradeContrast",L"NeuralGradeSaturation",L"NeuralReprojectMotionStrength",
    L"NeuralReprojectMotionStrengthRef",L"NeuralReprojectJitterComp",L"NeuralReprojectStabilizeStill",
    L"NeuralGainLimit",L"NeuralHighlightGuardSoft",L"NeuralHighlightGuardLight",
    L"NeuralHighlightGuardDetail",L"NeuralLightSmooth",L"NeuralHaloReduce",
    L"NeuralRatioFloor",L"NeuralLateCompose",L"NeuralHaloNetScale",
    L"NeuralLightGuideDepth",L"NeuralHaloExcess",L"NeuralHaloCutMoving"};

inline constexpr bool aceita_negativo(unsigned c) noexcept {
    return c==MotionFactor||c==JitterComp||c==GradeExposure||c==GradeContrast||c==GradeSaturation;}

inline unsigned indice_ausente(unsigned c) noexcept {return c==LightGuard?nearest(c,nrrealce::luz_padrao):c<ControlCount?defaults[c]:0u;}
enum class Ini {Ausente,Valido,Invalido};

inline Ini indice_do_ini(unsigned c,const wchar_t* texto,unsigned& indice,bool* legado=nullptr) noexcept {
    indice=indice_ausente(c);if(legado)*legado=false;
    if(c>=ControlCount)return Ini::Invalido;
    if(!texto||!*texto)return Ini::Ausente;
    if(std::wcslen(texto)>=31)return Ini::Invalido;
    wchar_t* fim{};const double v=std::wcstod(texto,&fim);
    if(fim==texto||!fim||*fim||!std::isfinite(v)||(v<0&&!aceita_negativo(c)))return Ini::Invalido;
    indice=nearest(c,float(v));
    if(c==LightGuard&&nrrealce::luz_legada(v)){indice=nearest(c,guarda_sem_limite);if(legado)*legado=true;}
    return Ini::Valido;
}

inline bool semear_indices(Controls& s,const unsigned (&indices)[ControlCount]) noexcept {
    if(s.seeded.exchange(true,std::memory_order_acq_rel))return false;
    for(unsigned i=0;i<ControlCount;++i)set(s,i,indices[i]);
    return true;
}
}
