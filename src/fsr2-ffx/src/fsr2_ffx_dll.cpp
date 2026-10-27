// fsr2-ffx (26/10): ffx_fsr2_api_x64.dll nossa. A API do FSR 2.2 (a que o RDR2 importa: ContextCreate/Dispatch/Destroy, jitter,
// resolucao por qualidade, assert) em cima da FidelityFX API: cada contexto do FSR 2 vira um contexto de UPSCALE da FFX pela DLL da
// FidelityFX API da pasta do jogo (amd_fidelityfx_loader_dx12.dll, senao amd_fidelityfx_dx12.dll -- o loader 2.3 renomeado do RDR2): o
// loader escolhe o provedor (FSR 4.1 do driver na RDNA 4) e o proxy do mod, que engancha o ffxCreateContext/ffxDispatch do loader, roda a
// rede no despacho dele (o caminho FFX provado no RE, Witcher 3, Stellar Blade e SotTR). Mesmo modelo da libxess do SotTR
// (analysis/xess-fsr). A traducao do despacho e a do helper (active-scheduling/fsr2_ffx_traducao.*: o mesmo fonte, testado la).
// O jogo importa esta DLL: ela carrega na abertura, antes da swapchain; com GeracaoQuadros=1 (nr_fsr2_ffx.ini) a thread da carga poe o
// gancho do FG (geracao_quadros.h) e a swapchain do jogo nasce como a de FG do FFX.
// A ffx_fsr2_api_dx12_x64.dll do jogo continua a original (interface do backend, ffxGetResourceDX12...): daqui so se le o
// ID3D12Resource* de cada FfxResource e o ID3D12Device* da descricao. Nao depende da ffx_fsr2_api_x64.dll original.
// Jitter, numero de fases, resolucao/razao por qualidade e assert: as formulas do FSR 2.2.1 (ffx_fsr2.cpp e ffx_assert.cpp, MIT,
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc.).
#include <windows.h>
#include <d3d12.h>
#include "ffx_fsr2.h"
#include "fsr2_ffx_traducao.h"
#include "fsr2_ffx_recurso_dx12.h"
#include "geracao_quadros.h"
#include "registro.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace {
using f2f::logf;
constexpr unsigned kVersao = 1;
constexpr std::uint32_t kMagia = 0x46324E52u;
constexpr std::uint64_t kDespachosAteDestruir = 8;

constexpr unsigned kFalhasAteRecusar = 3;
constexpr std::size_t kAposentadosMax = 4;
static_assert(sizeof(FfxFsr2Context) >= 64, "o FfxFsr2Context do SDK tem 16536 uint32");

struct Runtime {
  bool tentou = false;
  HMODULE modulo = nullptr;
  char nome[48]{};
  PfnFfxCreateContext create = nullptr;
  PfnFfxDestroyContext destroy = nullptr;
  PfnFfxQuery query = nullptr;
  PfnFfxDispatch dispatch = nullptr;
  unsigned char inicio_create[16]{};
  bool gancho_visto = false;
  bool provedor_escolhido = false;
  std::uint64_t provedor = 0;
};
struct Aposentado { ffxContext ffx; std::uint64_t ate; };
struct Contexto {
  nrffx::fsr2ffx::Descricao desc{};
  ID3D12Device* device = nullptr;
  ffxContext ffx = nullptr;
  bool visto = false, reset_pendente = true;
  unsigned recriacoes = 0;
  std::uint64_t despachos = 0;
  std::uint64_t tentativas = 0;
  unsigned falhas_seguidas = 0;
  bool recusado = false;
  std::vector<Aposentado> aposentados;
};

struct Cabeca { nrffx::fsr2ffx::Descricao desc; std::uint32_t magia; Contexto* c; };

std::mutex g_trava;
Runtime rt;
std::atomic<std::uint64_t> g_despachos{0};
std::atomic<int> g_vivos{0};
std::atomic<std::uint64_t> g_criados{0}, g_falhas{0}, g_recusados{0};
std::vector<Contexto*> g_contextos;
FfxAssertCallback g_assert_cb = nullptr;

void estreito(const wchar_t* w, char* out, std::size_t n) {
  std::size_t i = 0;
  for (; w && w[i] && i + 1 < n; ++i) out[i] = w[i] < 128 ? static_cast<char>(w[i]) : '?';
  if (n) out[i] = 0;
}
std::wstring pasta_de(HMODULE m) {
  wchar_t c[MAX_PATH]{};
  const DWORD n = GetModuleFileNameW(m, c, MAX_PATH);
  if (!n || n >= MAX_PATH) return {};
  std::wstring s(c, n);
  const auto k = s.find_last_of(L"\\/");
  return k == std::wstring::npos ? std::wstring{} : s.substr(0, k);
}
bool desviado(const unsigned char* p) { return p[0] == 0xE9 || (p[0] == 0xFF && p[1] == 0x25); }

bool carregar() {
  if (rt.tentou) return rt.modulo != nullptr;
  rt.tentou = true;
  std::vector<std::wstring> pastas{f2f::pasta_da_dll()};
  const auto exe = pasta_de(nullptr);
  if (!exe.empty() && _wcsicmp(exe.c_str(), pastas[0].c_str()) != 0) pastas.push_back(exe);
  for (const wchar_t* nome : {L"amd_fidelityfx_loader_dx12.dll", L"amd_fidelityfx_dx12.dll"}) {
    HMODULE m = GetModuleHandleW(nome);
    for (std::size_t i = 0; !m && i < pastas.size(); ++i) {
      const std::wstring caminho = pastas[i] + L"\\" + nome;
      if (pastas[i].empty() || GetFileAttributesW(caminho.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
      m = LoadLibraryExW(caminho.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    }
    if (!m) continue;
    rt.create = reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(m, "ffxCreateContext"));
    rt.destroy = reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(m, "ffxDestroyContext"));
    rt.query = reinterpret_cast<PfnFfxQuery>(GetProcAddress(m, "ffxQuery"));
    rt.dispatch = reinterpret_cast<PfnFfxDispatch>(GetProcAddress(m, "ffxDispatch"));
    if (!rt.create || !rt.destroy || !rt.query || !rt.dispatch) continue;
    rt.modulo = m;
    estreito(nome, rt.nome, sizeof rt.nome);
    std::memcpy(rt.inicio_create, reinterpret_cast<const void*>(rt.create), sizeof rt.inicio_create);
    rt.gancho_visto = desviado(rt.inicio_create);
    logf("FFX: %s carregada (ffxCreateContext %s)", rt.nome, rt.gancho_visto ? "ja desviado" : "ainda sem desvio");
    return true;
  }
  logf("ERRO: nenhuma DLL da FidelityFX API (amd_fidelityfx_loader_dx12.dll ou amd_fidelityfx_dx12.dll) na pasta do jogo: sem upscale");
  return false;
}
bool gancho_chegou() {
  if (!rt.modulo) return false;
  if (!rt.gancho_visto && std::memcmp(reinterpret_cast<const void*>(rt.create), rt.inicio_create, sizeof rt.inicio_create) != 0) rt.gancho_visto = true;
  return rt.gancho_visto;
}
void mensagem(uint32_t tipo, const wchar_t* msg) {
  char t[256];
  estreito(msg, t, sizeof t);
  logf("FFX %s: %s", tipo == FFX_API_MESSAGE_TYPE_ERROR ? "erro" : "aviso", t);
}

void escolher_provedor(ID3D12Device* device) {
  if (rt.provedor_escolhido) return;
  rt.provedor_escolhido = true;
  std::uint64_t n = 0;
  ffxQueryDescGetVersions v{};
  v.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
  v.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
  v.device = device;
  v.outputCount = &n;
  if (rt.query(nullptr, &v.header) != FFX_API_RETURN_OK || n == 0 || n > 64) return;
  std::vector<std::uint64_t> ids(static_cast<std::size_t>(n));
  std::vector<const char*> nomes(static_cast<std::size_t>(n));
  v.versionIds = ids.data();
  v.versionNames = nomes.data();
  if (rt.query(nullptr, &v.header) != FFX_API_RETURN_OK) return;
  const std::string& pedido = f2f::ini().provedor;
  for (std::size_t i = 0; i < ids.size(); ++i) {
    const char* nome = nomes[i] ? nomes[i] : "";
    const bool esc = !pedido.empty() && !rt.provedor && std::strncmp(nome, pedido.c_str(), pedido.size()) == 0;
    if (esc) rt.provedor = ids[i];
    logf("FFX: provedor %zu: %s%s", i, nome, esc ? "  <- Provedor do INI" : "");
  }
}
void destruir(ffxContext& f) {
  if (f && rt.destroy && rt.destroy(&f, nullptr) == FFX_API_RETURN_OK) g_vivos.fetch_sub(1);
  f = nullptr;
}

bool criar(Contexto& c) {
  escolher_provedor(c.device);
  const bool com_gancho = gancho_chegou();
  ffxCreateContextDescUpscale up{};
  nrffx::fsr2ffx::criar_upscale(c.desc, up);
  up.fpMessage = &mensagem;
  ffxCreateBackendDX12Desc be{};
  be.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
  be.device = c.device;
  ffxOverrideVersion ov{};
  ov.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
  ov.versionId = rt.provedor;
  ffxCreateContextDescUpscaleVersion ver{};
  ver.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE_VERSION;
  ver.version = FFX_UPSCALER_VERSION;
  up.header.pNext = &be.header;
  ffxApiHeader* fim = &be.header;
  if (rt.provedor) { fim->pNext = &ov.header; fim = &ov.header; }
  fim->pNext = &ver.header;
  ffxReturnCode_t r = rt.create(&c.ffx, &up.header, nullptr);
  if (r != FFX_API_RETURN_OK) { fim->pNext = nullptr; c.ffx = nullptr; r = rt.create(&c.ffx, &up.header, nullptr); }
  if (r != FFX_API_RETURN_OK || !c.ffx) {
    c.ffx = nullptr;
    logf("ERRO: ffxCreateContext(%s) devolveu %u (flags FFX 0x%x, %ux%u -> %ux%u)", rt.nome, r, up.flags, c.desc.max_w, c.desc.max_h, c.desc.tela_w, c.desc.tela_h);
    return false;
  }
  g_vivos.fetch_add(1);
  g_criados.fetch_add(1);
  c.visto = com_gancho;
  c.reset_pendente = true;
  ffxQueryGetProviderVersion pv{};
  pv.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
  const bool tem = rt.query(&c.ffx, &pv.header) == FFX_API_RETURN_OK && pv.versionName;
  logf("contexto FFX %p: flags FSR 2 0x%x -> FFX 0x%x, %ux%u -> %ux%u, provedor %s, %s (recriacao %u)", static_cast<void*>(c.ffx), c.desc.flags,
       up.flags, c.desc.max_w, c.desc.max_h, c.desc.tela_w, c.desc.tela_h, tem ? pv.versionName : "?",
       com_gancho ? "com o gancho do proxy" : "ANTES do gancho do proxy (recria quando ele chegar)", c.recriacoes);
  return true;
}

Contexto* contexto_de(FfxFsr2Context* ctx) {
  if (!ctx) return nullptr;
  Cabeca cab{};
  std::memcpy(&cab, ctx, sizeof cab);
  if (cab.magia != kMagia) return nullptr;
  for (Contexto* c : g_contextos)
    if (c == cab.c) return c;
  return nullptr;
}

void aposentar(Contexto& c) {
  if (c.ffx) c.aposentados.push_back({c.ffx, c.tentativas + kDespachosAteDestruir});
  c.ffx = nullptr;
  while (c.aposentados.size() > kAposentadosMax) {
    destruir(c.aposentados.front().ffx);
    c.aposentados.erase(c.aposentados.begin());
  }
}
void limpar_aposentados(Contexto& c) {
  for (std::size_t i = 0; i < c.aposentados.size();)
    if (c.tentativas >= c.aposentados[i].ate) { destruir(c.aposentados[i].ffx); c.aposentados.erase(c.aposentados.begin() + static_cast<std::ptrdiff_t>(i)); }
    else ++i;
}

void contar_falha(Contexto& c, const char* onde) {
  g_falhas.fetch_add(1);
  if (++c.falhas_seguidas < kFalhasAteRecusar || c.recusado) return;
  c.recusado = true;
  logf("ERRO: %u falhas seguidas da FFX (a ultima no %s): este contexto do FSR 2 nao recria mais o da FFX; sem upscale ate o jogo "
       "recriar o FSR 2 (ffxFsr2ContextCreate)", kFalhasAteRecusar, onde);
}

void soltar(Contexto* c) {
  destruir(c->ffx);
  for (auto& a : c->aposentados) destruir(a.ffx);
  c->aposentados.clear();
  if (c->device) c->device->Release();
  c->device = nullptr;
  for (auto it = g_contextos.begin(); it != g_contextos.end(); ++it)
    if (*it == c) { g_contextos.erase(it); break; }
  delete c;
}

bool backend_vulkan(const FfxFsr2ContextDescription* d) {
  HMODULE m = nullptr;
  if (!d->callbacks.fpCreateBackendContext ||
      !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(d->callbacks.fpCreateBackendContext), &m))
    return false;
  wchar_t n[MAX_PATH]{};
  GetModuleFileNameW(m, n, MAX_PATH);
  for (wchar_t* q = n; *q; ++q) *q = static_cast<wchar_t>(towlower(*q));
  return std::wcsstr(n, L"_vk_") != nullptr;
}
float halton(int32_t index, int32_t base) {
  float f = 1.0f, result = 0.0f;
  for (int32_t currentIndex = index; currentIndex > 0;) {
    f /= (float)base;
    result = result + f * (float)(currentIndex % base);
    currentIndex = (uint32_t)(floorf((float)(currentIndex) / (float)(base)));
  }
  return result;
}
DWORD WINAPI thread_carga(void*) {
  char exe[MAX_PATH]{};
  GetModuleFileNameA(nullptr, exe, MAX_PATH);
  const auto& i = f2f::ini();
  logf("ffx_fsr2_api_x64.dll nossa (FSR 2.2 -> FidelityFX API, versao %u%s) no processo %lu, exe %s; INI: Provedor '%s' GeracaoQuadros %d "
       "FgOrdemMs %d RecriarAoGanchar %d", kVersao, NR_SEM_LOG ? ", sem log" : "", GetCurrentProcessId(), exe, i.provedor.c_str(),
       i.geracao_quadros, i.fg_ordem_ms, i.recriar_ao_ganchar);
  f2fg::instalar();
  return 0;
}
}

extern "C" {

bool ffxAssertReport(const char* file, int32_t line, const char* condition, const char* message) {
  if (!file) return true;
  char t[512];
  std::snprintf(t, sizeof t, "%s(%d): ASSERTION FAILED. %s\n", file, line, message ? message : (condition ? condition : ""));
  if (g_assert_cb) g_assert_cb(t);
  else OutputDebugStringA(t);
  return true;
}
void ffxAssertSetPrintingCallback(FfxAssertCallback callback) { g_assert_cb = callback; }

int32_t ffxFsr2GetJitterPhaseCount(int32_t renderWidth, int32_t displayWidth) {
  const float basePhaseCount = 8.0f;
  const int32_t jitterPhaseCount = int32_t(basePhaseCount * pow((float(displayWidth) / renderWidth), 2.0f));
  return jitterPhaseCount;
}
FfxErrorCode ffxFsr2GetJitterOffset(float* outX, float* outY, int32_t index, int32_t phaseCount) {
  if (!outX || !outY) return FFX_ERROR_INVALID_POINTER;
  if (!(phaseCount > 0)) return FFX_ERROR_INVALID_ARGUMENT;
  const float x = halton((index % phaseCount) + 1, 2) - 0.5f;
  const float y = halton((index % phaseCount) + 1, 3) - 0.5f;
  *outX = x;
  *outY = y;
  return FFX_OK;
}
float ffxFsr2GetUpscaleRatioFromQualityMode(FfxFsr2QualityMode qualityMode) {
  switch (qualityMode) {
    case FFX_FSR2_QUALITY_MODE_QUALITY: return 1.5f;
    case FFX_FSR2_QUALITY_MODE_BALANCED: return 1.7f;
    case FFX_FSR2_QUALITY_MODE_PERFORMANCE: return 2.0f;
    case FFX_FSR2_QUALITY_MODE_ULTRA_PERFORMANCE: return 3.0f;
    default: return 0.0f;
  }
}
FfxErrorCode ffxFsr2GetRenderResolutionFromQualityMode(uint32_t* renderWidth, uint32_t* renderHeight, uint32_t displayWidth,
                                                       uint32_t displayHeight, FfxFsr2QualityMode qualityMode) {
  if (!renderWidth || !renderHeight) return FFX_ERROR_INVALID_POINTER;
  if (!(FFX_FSR2_QUALITY_MODE_QUALITY <= qualityMode && qualityMode <= FFX_FSR2_QUALITY_MODE_ULTRA_PERFORMANCE)) return FFX_ERROR_INVALID_ENUM;
  const float ratio = ffxFsr2GetUpscaleRatioFromQualityMode(qualityMode);
  *renderWidth = (uint32_t)((float)displayWidth / ratio);
  *renderHeight = (uint32_t)((float)displayHeight / ratio);
  return FFX_OK;
}
bool ffxFsr2ResourceIsNull(FfxResource resource) { return resource.resource == NULL; }

FfxErrorCode ffxFsr2ContextCreate(FfxFsr2Context* context, const FfxFsr2ContextDescription* desc) {
  if (!context || !desc) return FFX_ERROR_INVALID_POINTER;
  f2fg::instalar();
  if (!desc->device) return FFX_ERROR_NULL_DEVICE;
  if (backend_vulkan(desc)) {
    logf("ERRO: ffxFsr2ContextCreate com o backend Vulkan do FSR 2: esta DLL so faz D3D12 (rode o jogo em DX12)");
    return FFX_ERROR_BACKEND_API_ERROR;
  }
  nrffx::fsr2ffx::Descricao d{};
  std::uint32_t bruto[5]{desc->flags, desc->maxRenderSize.width, desc->maxRenderSize.height, desc->displaySize.width, desc->displaySize.height};
  if (!nrffx::fsr2ffx::descricao_plausivel(bruto, d)) {
    logf("ERRO: ffxFsr2ContextCreate com descricao implausivel (flags 0x%x, %ux%u -> %ux%u)", bruto[0], bruto[1], bruto[2], bruto[3], bruto[4]);
    return FFX_ERROR_INVALID_ARGUMENT;
  }
  std::lock_guard<std::mutex> l(g_trava);

  if (Contexto* velho = contexto_de(context)) {
    logf("AVISO: ffxFsr2ContextCreate %p por cima de um contexto vivo (sem ffxFsr2ContextDestroy): o velho sai (%llu despachos)",
         static_cast<void*>(context), static_cast<unsigned long long>(velho->despachos));
    soltar(velho);
  }
  if (!carregar()) return FFX_ERROR_BACKEND_API_ERROR;
  auto* c = new (std::nothrow) Contexto();
  if (!c) return FFX_ERROR_OUT_OF_MEMORY;
  try { g_contextos.push_back(c); } catch (...) { delete c; return FFX_ERROR_OUT_OF_MEMORY; }
  c->desc = d;
  c->device = static_cast<ID3D12Device*>(desc->device);
  c->device->AddRef();
  std::memset(context, 0, sizeof(FfxFsr2Context));
  const Cabeca cab{d, kMagia, c};
  std::memcpy(context, &cab, sizeof cab);
  logf("ffxFsr2ContextCreate %p: flags 0x%x, render %ux%u -> tela %ux%u (contexto FFX no 1o despacho)", static_cast<void*>(context), d.flags,
       d.max_w, d.max_h, d.tela_w, d.tela_h);
  return FFX_OK;
}
FfxErrorCode ffxFsr2ContextDispatch(FfxFsr2Context* context, const FfxFsr2DispatchDescription* desc) {
  if (!context || !desc) return FFX_ERROR_INVALID_POINTER;
  std::lock_guard<std::mutex> l(g_trava);
  Contexto* c = contexto_de(context);
  if (!c) return FFX_ERROR_INVALID_ARGUMENT;
  ++c->tentativas;
  limpar_aposentados(*c);
  if (c->recusado) { g_recusados.fetch_add(1); return FFX_ERROR_BACKEND_API_ERROR; }

  if (c->ffx && !c->visto && f2f::ini().recriar_ao_ganchar && gancho_chegou()) {
    aposentar(*c);
    ++c->recriacoes;
  }
  if (!c->ffx && !criar(*c)) { contar_falha(*c, "ffxCreateContext"); return FFX_ERROR_BACKEND_API_ERROR; }
  ffxDispatchDescUpscale up{};
  if (!nrffx::fsr2ffx::traduzir(desc, sizeof(FfxFsr2DispatchDescription), nrffx::fsr2::v21, c->desc, &nrffx::fsr2ffx::descrever_dx12, nullptr, up))
    return FFX_ERROR_INVALID_ARGUMENT;
  if (c->reset_pendente) up.reset = true;
  const ffxReturnCode_t r = rt.dispatch(&c->ffx, &up.header);
  if (r != FFX_API_RETURN_OK) {
    logf("ERRO: ffxDispatch devolveu %u (contexto %p): recria no proximo despacho", r, static_cast<void*>(c->ffx));
    aposentar(*c);
    contar_falha(*c, "ffxDispatch");
    return FFX_ERROR_BACKEND_API_ERROR;
  }
  c->falhas_seguidas = 0;
  c->reset_pendente = false;
  ++c->despachos;
  const auto total = g_despachos.fetch_add(1) + 1;
  if (c->despachos <= static_cast<std::uint64_t>(f2f::ini().log_despachos))
    logf("despacho %llu (contexto FFX %p): render %ux%u -> %ux%u, reset %u, nitidez %u %.2f, jitter %.4f %.4f, MV x%g %g, dt %.2f ms, pre %g, "
         "camera %g/%g/%.3f, total %llu", static_cast<unsigned long long>(c->despachos), static_cast<void*>(c->ffx), up.renderSize.width,
         up.renderSize.height, up.upscaleSize.width, up.upscaleSize.height, up.reset ? 1u : 0u, up.enableSharpening ? 1u : 0u, double(up.sharpness),
         double(up.jitterOffset.x), double(up.jitterOffset.y), double(up.motionVectorScale.x), double(up.motionVectorScale.y),
         double(up.frameTimeDelta), double(up.preExposure), double(up.cameraNear), double(up.cameraFar), double(up.cameraFovAngleVertical),
         static_cast<unsigned long long>(total));

  f2fg::apos_upscale(c->device, up, nrffx::fsr2ffx::flags_para_ffx(c->desc.flags));
  return FFX_OK;
}
FfxErrorCode ffxFsr2ContextDestroy(FfxFsr2Context* context) {
  if (!context) return FFX_ERROR_INVALID_POINTER;
  std::lock_guard<std::mutex> l(g_trava);
  Contexto* c = contexto_de(context);
  if (!c) return FFX_ERROR_INVALID_ARGUMENT;
  f2fg::soltar_contexto();
  logf("ffxFsr2ContextDestroy %p (%llu despachos, %u recriacoes)", static_cast<void*>(context), static_cast<unsigned long long>(c->despachos), c->recriacoes);
  soltar(c);
  std::memset(context, 0, sizeof(Cabeca));
  return FFX_OK;
}

FfxErrorCode ffxFsr2ContextGenerateReactiveMask(FfxFsr2Context*, const FfxFsr2GenerateReactiveDescription*) {
  static std::atomic<bool> avisou{false};
  if (!avisou.exchange(true)) logf("AVISO: ffxFsr2ContextGenerateReactiveMask nao suportado (o jogo segue sem a mascara gerada)");
  return FFX_ERROR_BACKEND_API_ERROR;
}

unsigned nrFsr2FfxVersao() { return kVersao; }
void nrFsr2FfxFgEstado(f2fg::Estado* e) {
  if (!e) return;
  *e = f2fg::estado();
  e->despachos_ffx = g_despachos.load();
  e->upscale_criado = g_vivos.load() > 0;
  e->ffx_criados = g_criados.load();
  e->ffx_falhas = g_falhas.load();
  e->ffx_recusados = g_recusados.load();
  e->ffx_vivos = g_vivos.load();
}
}

BOOL WINAPI DllMain(HINSTANCE modulo, DWORD motivo, LPVOID) {
  if (motivo == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(modulo);
    f2f::log_iniciar(modulo);

    if (HANDLE t = CreateThread(nullptr, 0, &thread_carga, nullptr, 0, nullptr)) CloseHandle(t);
  }
  return TRUE;
}
