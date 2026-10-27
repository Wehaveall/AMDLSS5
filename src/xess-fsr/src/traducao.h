#pragma once
#include <cstdint>

namespace xf {

enum : uint32_t {
  XESS_HIGH_RES_MV = 1u << 0,
  XESS_INVERTED_DEPTH = 1u << 1,
  XESS_EXPOSURE_SCALE_TEXTURE = 1u << 2,
  XESS_RESPONSIVE_PIXEL_MASK = 1u << 3,
  XESS_USE_NDC_VELOCITY = 1u << 4,
  XESS_EXTERNAL_DESCRIPTOR_HEAP = 1u << 5,
  XESS_LDR_INPUT_COLOR = 1u << 6,
  XESS_JITTERED_MV = 1u << 7,
  XESS_ENABLE_AUTOEXPOSURE = 1u << 8,
};

enum : uint32_t {
  FFX_HDR = 1u << 0,
  FFX_DISPLAY_RES_MV = 1u << 1,
  FFX_MV_JITTER_CANCELLATION = 1u << 2,
  FFX_DEPTH_INVERTED = 1u << 3,
  FFX_DEPTH_INFINITE = 1u << 4,
  FFX_AUTO_EXPOSURE = 1u << 5,
};

enum : int32_t {
  XESS_Q_ULTRA_PERFORMANCE = 100,
  XESS_Q_PERFORMANCE = 101,
  XESS_Q_BALANCED = 102,
  XESS_Q_QUALITY = 103,
  XESS_Q_ULTRA_QUALITY = 104,
  XESS_Q_ULTRA_QUALITY_PLUS = 105,
  XESS_Q_AA = 106,
};

struct Ajustes {
  int auto_exposicao = -1;
  int profundidade_infinita = 0;
};

uint32_t flags_ffx(uint32_t flags_xess, const Ajustes& a = {});

float razao_preset(int32_t qualidade);

bool resolucao_entrada(uint32_t saida_w, uint32_t saida_h, int32_t qualidade, uint32_t& w, uint32_t& h);

struct Par { float x, y; };

Par escala_vetores(uint32_t flags_xess, Par velocity_scale, uint32_t entrada_w, uint32_t entrada_h,
                   uint32_t saida_w, uint32_t saida_h);

Par jitter(Par jitter_xess, Par jitter_scale, Par sinal);

struct Camera { float perto, longe, fov_vertical_rad; };

Camera sanear_camera(float perto, float longe, float fov);

float pre_exposicao(float exposure_scale, float multiplicador);

float delta_ms(int64_t qpc_atual, int64_t qpc_anterior, int64_t frequencia);

bool formato_exposicao_ok(uint32_t dxgi_format);

struct JanelaExec {
  uint64_t n = 0;
  float jx_min = 0, jx_max = 0, jy_min = 0, jy_max = 0;
  uint32_t resets = 0, nao_zero = 0;
  uint32_t w = 0, h = 0, trocas_entrada = 0;
  int64_t qpc_ini = 0, qpc_fim = 0;
  void adicionar(float jx, float jy, bool reset, uint32_t entrada_w, uint32_t entrada_h, int64_t qpc);

  double por_segundo(int64_t frequencia) const;
  void zerar() { *this = JanelaExec{}; }
};

bool formato_com_estencil(uint32_t dxgi_format);

uint32_t subrecurso(uint32_t mip, uint32_t fatia, uint32_t plano, uint32_t mips, uint32_t fatias);

struct ResumoEstencil {
  uint64_t total = 0;
  uint64_t por_valor[256]{};
  uint64_t por_bit[8]{};
  uint32_t valores_distintos() const;
};
ResumoEstencil resumir_estencil(const uint8_t* dados, uint32_t w, uint32_t h, uint32_t passo);

int captura_nesta(uint64_t e, uint64_t inicio, int quadros, int intervalo);

float nitidez_rcas(float nitidez, bool filtros_nitidez_do_helper, int nitidez_com_filtros);

}
