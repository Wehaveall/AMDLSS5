#pragma once
#include "fsr2_descriptor.h"
#include <ffx_api.h>
#include <ffx_api_types.h>
#include <ffx_upscale.h>

namespace nrffx {
namespace fsr2ffx {

std::uint32_t flags_para_ffx(std::uint32_t fsr2) noexcept;

struct Descricao{std::uint32_t flags,max_w,max_h,tela_w,tela_h;};

bool descricao_plausivel(const void* inicio,Descricao& d) noexcept;

void criar_upscale(const Descricao& d,ffxCreateContextDescUpscale& up) noexcept;

using Descrever=FfxApiResource(*)(void* recurso,std::uint32_t estado_api,std::uint32_t uso_extra,void* dado);

constexpr std::size_t v22_viewSpaceToMetersFactor=0x548;

bool traduzir(const void* fsr2,std::size_t legivel,const fsr2::Leiaute& L,const Descricao& ctx,Descrever descrever,void* dado,
              ffxDispatchDescUpscale& out) noexcept;

constexpr std::size_t v22_enableAutoReactive=0x54C,v22_colorOpaqueOnly=0x550,v22_autoTcThreshold=0x608,v22_fim=0x618;
struct Auto22{bool lido{},ligado{},opaco{};float tc_limiar{},tc_escala{},reativo_escala{},reativo_max{};};

bool auto_reativo_22(const void* fsr2,std::size_t legivel,const fsr2::Leiaute& L,Auto22& a) noexcept;

void texto_entradas(const ffxDispatchDescUpscale& d,const Auto22& a,char* out,std::size_t n) noexcept;

std::uint64_t chave_entradas(const ffxDispatchDescUpscale& d,const Auto22& a) noexcept;
}
}
