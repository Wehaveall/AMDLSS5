#pragma once

#include <cstdint>
namespace nrgap {
enum class Invalidar { Nada, Suave, Dura };

inline Invalidar decidir(std::uint64_t tick,std::uint64_t ultimo,std::uint64_t gap_ms,bool mudou_estrutura_pele_estilo,bool suave) noexcept {
    if(mudou_estrutura_pele_estilo)return Invalidar::Dura;
    if(ultimo&&tick-ultimo>gap_ms)return suave?Invalidar::Suave:Invalidar::Dura;
    return Invalidar::Nada;
}

inline int gap_suave_de(int v) noexcept {return v==1?1:0;}
}
