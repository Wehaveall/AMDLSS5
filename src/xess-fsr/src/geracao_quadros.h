#pragma once
#include <windows.h>
#include <d3d12.h>
#include <cstdint>

#include <ffx_api.h>

namespace xfg {

struct Ffx {
  PfnFfxCreateContext create = nullptr;
  PfnFfxDestroyContext destroy = nullptr;
  PfnFfxConfigure configure = nullptr;
  PfnFfxQuery query = nullptr;
  PfnFfxDispatch dispatch = nullptr;
};

using Carregar = const Ffx* (*)();

void instalar(Carregar carregar);

struct Quadro {
  ID3D12Device* device = nullptr;
  ID3D12GraphicsCommandList* lista = nullptr;
  ID3D12Resource* profundidade = nullptr;
  ID3D12Resource* vetores = nullptr;
  uint32_t ew = 0, eh = 0;
  uint32_t sw = 0, sh = 0;
  uint32_t flags_upscale = 0;
  float jx = 0, jy = 0;
  float mvx = 1, mvy = 1;
  float delta_ms = 16.7f;
  float perto = 0.1f, longe = 100000.0f, fov_rad = 1.0472f;
  bool reset = false;
};

void por_execucao(const Quadro& q);

void soltar_contexto();

struct Estado {
  bool gancho_fabrica = false;
  bool swapchain = false;
  bool contexto = false;
  uint32_t swapchains_criadas = 0;
  uint64_t presents = 0, presents_teste = 0, pulados = 0, desligados = 0, prepares = 0, erros = 0;
  uint64_t frame_id = 0;
  uint64_t geracoes = 0;
  bool usuario = false;
  void* swapchain_ptr = nullptr;
  char provedor[64] = {};

  uint64_t presents_sem_exec = 0, presents_varios_exec = 0;
  uint64_t ordem_esperas = 0, ordem_estouros = 0;
  double ordem_espera_ms = 0;
  bool ordem_desligada = false;
};
Estado estado();

}
