#pragma once

#include <cstdint>

namespace nrffx_estado {

inline constexpr std::uint32_t COMMON=0,VERTEX_CB=0x1,INDEX=0x2,RT=0x4,UAV=0x8,DEPTH_WRITE=0x10,DEPTH_READ=0x20,
    NON_PIXEL=0x40,PIXEL=0x80,INDIRECT=0x200,COPY_DEST=0x400,COPY_SOURCE=0x800;
inline constexpr std::uint32_t recusa=~0u;

inline constexpr std::uint32_t leitura=NON_PIXEL,escrita=UAV;

inline constexpr std::uint32_t classico(std::uint32_t s) noexcept {
    if((s&1u)&&s!=1u)s&=~1u;
    switch(s){
    case 1:return COMMON;case 2:return escrita;case 4:return leitura;
    case 8:return PIXEL;case 12:return PIXEL|leitura;
    case 16:return COPY_SOURCE;case 32:return COPY_DEST;
    case 20:return COPY_SOURCE|leitura;default:return recusa;}
}

struct Resultado { std::uint32_t d3d12; bool tolerado; };

inline constexpr bool valido_em_compute(std::uint32_t d3d12) noexcept {
    return d3d12!=recusa && (d3d12&~(UAV|NON_PIXEL|INDIRECT|COPY_DEST|COPY_SOURCE))==0u;
}

inline constexpr Resultado traduzir(std::uint32_t s) noexcept {
    const std::uint32_t c=classico(s);
    if(c!=recusa)return {c,false};
    if(s==0)return {COMMON,true};
    std::uint32_t b=s&~1u;
    if(b&~0x3FFu)return {recusa,false};
    constexpr std::uint32_t escritas=0x2u|0x20u|0x100u;
    if(b&escritas){
        if(b==0x100u)return {RT,true};
        return {recusa,false};
    }
    if(b==0x200u)return {DEPTH_WRITE,true};
    if(b==0x80u)return {COMMON,true};
    std::uint32_t r=0;
    if(b&0x4u)r|=NON_PIXEL;
    if(b&0x8u)r|=PIXEL;
    if(b&0x10u)r|=COPY_SOURCE;
    if(b&0x40u)r|=INDIRECT;
    if(b&0x200u)r|=DEPTH_READ;

    return {r,true};
}
}
