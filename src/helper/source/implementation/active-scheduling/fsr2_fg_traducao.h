#pragma once
#include "fsr2_ffx_traducao.h"
#include <ffx_framegeneration.h>
#include <cstdint>

namespace nrffx {
namespace fsr2fg {

std::uint32_t flags_fg(std::uint32_t flags_upscale) noexcept;

bool backbuffer_hdr(std::uint32_t formato_ffx) noexcept;

void criar_fg(std::uint32_t flags_upscale,std::uint32_t tela_w,std::uint32_t tela_h,std::uint32_t render_w,std::uint32_t render_h,
              std::uint32_t formato_ffx,ffxCreateContextDescFrameGeneration& fd) noexcept;

void configurar(void* swapchain,bool ligado,std::uint64_t frame_id,std::uint32_t tela_w,std::uint32_t tela_h,
                FfxApiFrameGenerationDispatchFunc callback,void* contexto_callback,ffxConfigureDescFrameGeneration& c) noexcept;

bool preparar(const ffxDispatchDescUpscale& up,std::uint64_t frame_id,bool reset_extra,ffxDispatchDescFrameGenerationPrepareV2& p) noexcept;

struct Ritmo{std::uint32_t upscales_desde_present=0;bool anterior_novo=false;};
enum class AoPresent{Segue,Desliga};
void ao_upscale(Ritmo& r) noexcept;
AoPresent ao_present(Ritmo& r,bool fg_ligado) noexcept;

bool aplicar_pedido(long pedido,bool atual) noexcept;

struct Chave{const void* device;std::uint32_t tela_w,tela_h,render_w,render_h,flags,formato;};
bool precisa_recriar(const Chave& atual,const Chave& nova) noexcept;
}
}
