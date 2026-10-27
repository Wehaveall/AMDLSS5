#pragma once
#include <cstdint>

struct ID3D12Device;
struct ffxDispatchDescUpscale;

namespace f2fg {

bool instalar() noexcept;

void apos_upscale(ID3D12Device* device, const ffxDispatchDescUpscale& up, std::uint32_t flags_upscale) noexcept;

void soltar_contexto() noexcept;

struct Estado {
  bool gancho_fabrica, memoria, swapchain, contexto, usuario, fg_ligado, upscale_criado;
  std::uint32_t swapchains_criadas;
  std::uint64_t presents, presents_teste, prepares, geracoes, desligados, erros, frame_id, despachos_ffx;
  void* swapchain_ptr;
  void* orig_csh;
  char provedor[32];

  std::uint64_t ffx_criados, ffx_falhas, ffx_recusados;
  int ffx_vivos;
};
Estado estado() noexcept;
}
