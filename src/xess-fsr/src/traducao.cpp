#include "traducao.h"
#include <cmath>
#include <cstddef>

namespace xf {

uint32_t flags_ffx(uint32_t x, const Ajustes& a) {
  uint32_t f = 0;
  if (!(x & XESS_LDR_INPUT_COLOR)) f |= FFX_HDR;
  if (x & XESS_HIGH_RES_MV) f |= FFX_DISPLAY_RES_MV;
  if (x & XESS_JITTERED_MV) f |= FFX_MV_JITTER_CANCELLATION;
  if (x & XESS_INVERTED_DEPTH) f |= FFX_DEPTH_INVERTED;
  bool autoexp = (x & XESS_ENABLE_AUTOEXPOSURE) != 0;
  if (a.auto_exposicao == 0) autoexp = false;
  if (a.auto_exposicao == 1) autoexp = true;
  if (autoexp) f |= FFX_AUTO_EXPOSURE;
  if (a.profundidade_infinita == 1) f |= FFX_DEPTH_INFINITE;
  return f;
}

float razao_preset(int32_t q) {
  switch (q) {
    case XESS_Q_ULTRA_PERFORMANCE: return 3.0f;
    case XESS_Q_PERFORMANCE: return 2.0f;
    case XESS_Q_BALANCED: return 1.7f;
    case XESS_Q_QUALITY: return 1.5f;
    case XESS_Q_ULTRA_QUALITY: return 1.3f;
    case XESS_Q_ULTRA_QUALITY_PLUS: return 1.3f;
    case XESS_Q_AA: return 1.0f;
    default: return 0.0f;
  }
}

bool resolucao_entrada(uint32_t sw, uint32_t sh, int32_t q, uint32_t& w, uint32_t& h) {
  const float r = razao_preset(q);
  if (r <= 0.0f || sw == 0 || sh == 0) return false;
  w = static_cast<uint32_t>(static_cast<float>(sw) / r);
  h = static_cast<uint32_t>(static_cast<float>(sh) / r);
  if (w == 0) w = 1;
  if (h == 0) h = 1;
  return true;
}

Par escala_vetores(uint32_t x, Par v, uint32_t ew, uint32_t eh, uint32_t sw, uint32_t sh) {
  if (!(x & XESS_USE_NDC_VELOCITY)) return v;
  const bool alta = (x & XESS_HIGH_RES_MV) != 0;
  const float L = static_cast<float>(alta ? sw : ew);
  const float A = static_cast<float>(alta ? sh : eh);
  return {0.5f * L * v.x, -0.5f * A * v.y};
}

Par jitter(Par j, Par escala, Par sinal) {
  return {j.x * escala.x * sinal.x, j.y * escala.y * sinal.y};
}

Camera sanear_camera(float perto, float longe, float fov) {
  constexpr float pi = 3.14159265358979f;
  auto ok = [](float v) { return std::isfinite(v) && v > 0.0f; };
  if (!ok(perto)) perto = 0.1f;
  if (!ok(longe) || longe >= 3.0e38f) longe = 100000.0f;
  if (perto > longe) { const float t = perto; perto = longe; longe = t; }
  if (!(longe > perto)) longe = perto + 100000.0f;
  if (!ok(fov)) fov = 60.0f;
  if (fov > 2.0f * pi) fov = fov * pi / 180.0f;
  if (!(fov < pi)) fov = 60.0f * pi / 180.0f;
  return {perto, longe, fov};
}

float nitidez_rcas(float n, bool filtros, int com_filtros) {
  if (!(n > 0.0f)) return 0.0f;
  if (filtros && com_filtros != 1) return 0.0f;
  return n > 1.0f ? 1.0f : n;
}

float pre_exposicao(float e, float m) {
  if (!std::isfinite(e) || e <= 0.0f) e = 1.0f;
  if (!std::isfinite(m) || m <= 0.0f) m = 1.0f;
  const float p = 1.0f / (e * m);
  return (std::isfinite(p) && p > 0.0f) ? p : 1.0f;
}

float delta_ms(int64_t atual, int64_t anterior, int64_t freq) {
  if (anterior == 0 || freq <= 0 || atual <= anterior) return 1000.0f / 60.0f;
  const double ms = static_cast<double>(atual - anterior) * 1000.0 / static_cast<double>(freq);
  if (ms < 0.1 || ms > 200.0) return 1000.0f / 60.0f;
  return static_cast<float>(ms);
}

void JanelaExec::adicionar(float jx, float jy, bool reset, uint32_t ew, uint32_t eh, int64_t qpc) {
  if (n == 0) {
    jx_min = jx_max = jx;
    jy_min = jy_max = jy;
    qpc_ini = qpc;
    w = ew;
    h = eh;
  } else {
    if (jx < jx_min) jx_min = jx;
    if (jx > jx_max) jx_max = jx;
    if (jy < jy_min) jy_min = jy;
    if (jy > jy_max) jy_max = jy;
    if (ew != w || eh != h) ++trocas_entrada;
    w = ew;
    h = eh;
  }
  if (jx != 0.0f || jy != 0.0f) ++nao_zero;
  if (reset) ++resets;
  qpc_fim = qpc;
  ++n;
}

double JanelaExec::por_segundo(int64_t freq) const {
  if (n < 2 || freq <= 0 || qpc_fim <= qpc_ini) return 0.0;
  return static_cast<double>(n - 1) * static_cast<double>(freq) / static_cast<double>(qpc_fim - qpc_ini);
}

bool formato_exposicao_ok(uint32_t f) {

  return f == 39 || f == 41 || f == 53 || f == 54;
}

bool formato_com_estencil(uint32_t f) { return f == 44 || f == 45 || f == 19 || f == 20; }

uint32_t subrecurso(uint32_t mip, uint32_t fatia, uint32_t plano, uint32_t mips, uint32_t fatias) {
  return mip + fatia * mips + plano * mips * fatias;
}

uint32_t ResumoEstencil::valores_distintos() const {
  uint32_t n = 0;
  for (uint64_t v : por_valor) n += v ? 1u : 0u;
  return n;
}

ResumoEstencil resumir_estencil(const uint8_t* dados, uint32_t w, uint32_t h, uint32_t passo) {
  ResumoEstencil r;
  if (!dados || passo < w) return r;
  for (uint32_t y = 0; y < h; ++y) {
    const uint8_t* l = dados + static_cast<size_t>(y) * passo;
    for (uint32_t x = 0; x < w; ++x) ++r.por_valor[l[x]];
  }
  r.total = static_cast<uint64_t>(w) * h;
  for (int v = 0; v < 256; ++v)
    for (int b = 0; b < 8; ++b)
      if (v & (1 << b)) r.por_bit[b] += r.por_valor[v];
  return r;
}

int captura_nesta(uint64_t e, uint64_t inicio, int quadros, int intervalo) {
  if (inicio == 0 || quadros <= 0 || e < inicio) return -1;
  const uint64_t passo = intervalo < 1 ? 1u : static_cast<uint64_t>(intervalo);
  const uint64_t d = e - inicio;
  if (d % passo) return -1;
  const uint64_t i = d / passo;
  return i < static_cast<uint64_t>(quadros) ? static_cast<int>(i) : -1;
}

}
