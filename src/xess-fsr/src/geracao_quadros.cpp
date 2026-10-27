#include "geracao_quadros.h"

#include <dxgi1_6.h>

#include <ffx_api_types.h>
#include <dx12/ffx_api_dx12.h>
#include <ffx_framegeneration.h>
#include <dx12/ffx_api_framegeneration_dx12.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include "fg_publico.h"
#include "registro.h"

namespace xfg {
namespace {

using xf::logf;

constexpr int kSlotCreateSwapChain = 10;
constexpr int kSlotCreateSwapChainForHwnd = 15;
constexpr int kSlotRelease = 2;
constexpr int kSlotPresent = 8;
constexpr int kSlotPresent1 = 22;

using PfnCreateSwapChain = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**);
using PfnCreateSwapChainForHwnd = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*,
                                                              const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*,
                                                              IDXGISwapChain1**);
using PfnRelease = ULONG(STDMETHODCALLTYPE*)(IUnknown*);
using PfnPresent = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
using PfnPresent1 = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain1*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*);

struct Global {
  Carregar carregar = nullptr;
  const Ffx* ffx = nullptr;
  void** vt_fabrica = nullptr;
  PfnCreateSwapChain orig_cs = nullptr;
  PfnCreateSwapChainForHwnd orig_csh = nullptr;
  void** vt_sc = nullptr;
  PfnRelease orig_release = nullptr;
  PfnPresent orig_present = nullptr;
  PfnPresent1 orig_present1 = nullptr;

  IDXGISwapChain4* sc = nullptr;
  ffxContext ctx_sc = nullptr;

  ffxContext fg = nullptr;
  ID3D12Device* fg_device = nullptr;
  uint32_t fg_tela_w = 0, fg_tela_h = 0, fg_max_w = 0, fg_max_h = 0, fg_flags = 0;
  DXGI_FORMAT fg_formato = DXGI_FORMAT_UNKNOWN;
  uint64_t fg_provedor = 0;
  bool fg_ligado = false;
  bool reset_proximo = true;

  uint64_t execs_desde_present = 0;
  bool anterior_novo = false;
  bool aviso_sem_sc = false, aviso_slot = false, aviso_criar = false, aviso_removido = false;

  bool usuario = false;
  HANDLE mapa = nullptr;
  nrfgpub::Fg* pub = nullptr;
  Estado e;
};
Global g;
std::recursive_mutex g_trava;
thread_local int t_criando = 0;
thread_local int t_apresentando = 0;

std::mutex g_ordem_m;
std::condition_variable g_ordem_cv;
bool g_pendente = false;
std::atomic<bool> g_ordem_off{false};
int g_estouros_seguidos = 0;
struct Espera {
  bool esperou = false, estourou = false;
  double ms = 0;
};
void ordem_marcar(bool pendente) {
  {
    std::lock_guard<std::mutex> l(g_ordem_m);
    g_pendente = pendente;
  }
  if (!pendente) g_ordem_cv.notify_all();
}
Espera ordem_esperar() {
  Espera r;
  const int ms = xf::ini().fg_ordem_ms;
  if (!xf::ini().fg_ordem || ms <= 0 || g_ordem_off.load(std::memory_order_relaxed)) return r;
  std::unique_lock<std::mutex> l(g_ordem_m);
  if (!g_pendente) return r;
  r.esperou = true;
  const auto t0 = std::chrono::steady_clock::now();
  r.estourou = !g_ordem_cv.wait_for(l, std::chrono::milliseconds(ms), [] { return !g_pendente; });
  r.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  if (r.estourou) g_pendente = false;
  return r;
}

bool gravar_ptr(void** slot, void* novo) {
  DWORD velho = 0;
  if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &velho)) return false;
  *slot = novo;
  DWORD x = 0;
  VirtualProtect(slot, sizeof(void*), velho, &x);
  return true;
}

ffxReturnCode_t cb_gerar(ffxDispatchDescFrameGeneration* params, void* ctx) {

  if (!g.ffx || !ctx) return FFX_API_RETURN_ERROR_PARAMETER;
  const ffxReturnCode_t r = g.ffx->dispatch(static_cast<ffxContext*>(ctx), &params->header);
  ++g.e.geracoes;
  if (r != FFX_API_RETURN_OK && g.e.erros++ < 16) logf("ERRO FG: dispatch da geracao devolveu %u (frameID %llu)", r,
                                                     static_cast<unsigned long long>(params->frameID));
  return r;
}

void publicar() {
  if (!g.pub) return;
  g.pub->disponivel = g.sc ? 1u : 0u;
  g.pub->ligado = g.usuario && g.sc ? 1u : 0u;
  g.pub->geracoes = g.e.geracoes;
  g.pub->tick_ms = GetTickCount64();
  std::snprintf(g.pub->provedor, sizeof g.pub->provedor, "%s", g.e.provedor);
}

void aplicar_pedido() {
  if (!g.pub) return;
  const long p = InterlockedExchange(&g.pub->pedido, -1);
  if (p != 0 && p != 1) return;
  const bool novo = p == 1;
  logf("FG: pedido do menu: %s (era %s), so nesta sessao", novo ? "ligar" : "desligar", g.usuario ? "ligado" : "desligado");
  g.usuario = novo;
  g.e.usuario = novo;
}

void configurar(bool ligado) {
  if (!g.fg || !g.sc) return;
  ffxConfigureDescFrameGeneration c{};
  c.header.type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;
  c.swapChain = g.sc;
  c.frameGenerationCallback = ligado ? &cb_gerar : nullptr;
  c.frameGenerationCallbackUserContext = ligado ? &g.fg : nullptr;
  c.frameGenerationEnabled = ligado;
  c.allowAsyncWorkloads = ligado && xf::ini().fg_assincrono != 0;
  c.flags = static_cast<uint32_t>(xf::ini().fg_debug) &
            (FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_TEAR_LINES | FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_RESET_INDICATORS |
             FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_PACING_LINES);
  c.generationRect = {0, 0, static_cast<int32_t>(g.fg_tela_w), static_cast<int32_t>(g.fg_tela_h)};
  c.frameID = ++g.e.frame_id;
  const ffxReturnCode_t r = g.ffx->configure(&g.fg, &c.header);
  if (r != FFX_API_RETURN_OK && g.e.erros++ < 16) logf("ERRO FG: ffxConfigure(%s) devolveu %u", ligado ? "ligado" : "desligado", r);
  if (ligado != g.fg_ligado) {
    logf("FG %s (frameID %llu)",
         ligado ? "ligado" : (g.usuario ? "desligado: Presents sem Execute (menu/carregamento)" : "desligado pelo menu do mod"),
         static_cast<unsigned long long>(g.e.frame_id));
    if (!ligado) ++g.e.desligados;
  }
  if (!ligado) {
    g.reset_proximo = true;
    ordem_marcar(false);
  }
  g.fg_ligado = ligado;
}

void destruir_fg() {
  if (!g.fg) return;
  configurar(false);
  const ffxReturnCode_t r = g.ffx->destroy(&g.fg, nullptr);
  logf("FG: contexto %p destruido (%u)", static_cast<void*>(g.fg), r);
  g.fg = nullptr;
  g.fg_ligado = false;
  g.reset_proximo = true;
  if (g.fg_device) g.fg_device->Release();
  g.fg_device = nullptr;
  g.e.contexto = false;
}

ULONG STDMETHODCALLTYPE meu_release(IUnknown* s) {
  if (!g.sc || s != static_cast<IUnknown*>(g.sc)) return g.orig_release(s);
  std::lock_guard<std::recursive_mutex> t(g_trava);
  const ULONG n = g.orig_release(s);
  if (n != 1 || s != static_cast<IUnknown*>(g.sc) || !g.ctx_sc) return n;

  logf("FG: o jogo soltou a swapchain de FG %p (presents %llu, pulados %llu)", static_cast<void*>(g.sc),
       static_cast<unsigned long long>(g.e.presents), static_cast<unsigned long long>(g.e.pulados));

  ffxContext c = g.ctx_sc;
  g.ctx_sc = nullptr;
  destruir_fg();
  g.sc = nullptr;
  g.e.swapchain = false;
  g.e.swapchain_ptr = nullptr;
  publicar();
  g.ffx->destroy(&c, nullptr);
  return 0;
}

bool antes_do_present(UINT sync, UINT flags) {
  ++g.e.presents;
  if (g.fg_ligado) {
    if (g.execs_desde_present == 0) ++g.e.presents_sem_exec;
    else if (g.execs_desde_present >= 2) ++g.e.presents_varios_exec;
  }
  ordem_marcar(false);
  const bool novo = g.execs_desde_present > 0;
  g.execs_desde_present = 0;
  bool apresentar = true;
  if (!novo && g.anterior_novo && xf::ini().fg_pula_repetido) apresentar = false;
  else if (!novo && !g.anterior_novo && g.fg_ligado) configurar(false);
  if (g.e.presents <= static_cast<uint64_t>(xf::ini().fg_log_presents < 0 ? 0 : xf::ini().fg_log_presents))
    logf("FG Present %llu (thread %lu, frameID %llu): intervalo %u flags 0x%X, Execute antes %s -> %s",
         static_cast<unsigned long long>(g.e.presents), GetCurrentThreadId(), static_cast<unsigned long long>(g.e.frame_id), sync,
         flags, novo ? "sim" : "nao", apresentar ? (g.fg_ligado ? "apresenta com FG" : "apresenta sem FG") : "PULADO (repetido)");
  g.anterior_novo = novo;
  if (!apresentar) ++g.e.pulados;
  return apresentar;
}

HRESULT present_teste() {
  static bool avisou = false;
  if (!avisou) { avisou = true; logf("FG: Present de teste respondido pela libxess (S_OK), sem chegar a swapchain real (FgTesteLocal=1)"); }
  ++g.e.presents_teste;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE meu_present(IDXGISwapChain* s, UINT sync, UINT flags) {
  if (t_apresentando || s != static_cast<IDXGISwapChain*>(g.sc)) return g.orig_present(s, sync, flags);
  if (flags & DXGI_PRESENT_TEST) {
    if (xf::ini().fg_teste_local) return present_teste();
    ++g.e.presents_teste;
    return g.orig_present(s, sync, flags);
  }
  std::lock_guard<std::recursive_mutex> t(g_trava);
  if (!antes_do_present(sync, flags)) return S_OK;
  ++t_apresentando;
  const HRESULT r = g.orig_present(s, sync, flags);
  --t_apresentando;
  return r;
}

HRESULT STDMETHODCALLTYPE meu_present1(IDXGISwapChain1* s, UINT sync, UINT flags, const DXGI_PRESENT_PARAMETERS* p) {
  if (t_apresentando || s != static_cast<IDXGISwapChain1*>(g.sc)) return g.orig_present1(s, sync, flags, p);
  if (flags & DXGI_PRESENT_TEST) {
    if (xf::ini().fg_teste_local) return present_teste();
    ++g.e.presents_teste;
    return g.orig_present1(s, sync, flags, p);
  }
  std::lock_guard<std::recursive_mutex> t(g_trava);
  if (!antes_do_present(sync, flags)) return S_OK;
  ++t_apresentando;
  const HRESULT r = g.orig_present1(s, sync, flags, p);
  --t_apresentando;
  return r;
}

void enganchar_swapchain(IDXGISwapChain4* sc) {
  void** vt = *reinterpret_cast<void***>(sc);
  if (vt == g.vt_sc) return;
  if (g.vt_sc) logf("AVISO FG: vtable da swapchain de FG mudou (%p -> %p)", static_cast<void*>(g.vt_sc), static_cast<void*>(vt));
  g.orig_release = reinterpret_cast<PfnRelease>(vt[kSlotRelease]);
  g.orig_present = reinterpret_cast<PfnPresent>(vt[kSlotPresent]);
  g.orig_present1 = reinterpret_cast<PfnPresent1>(vt[kSlotPresent1]);
  const bool ok = gravar_ptr(&vt[kSlotRelease], reinterpret_cast<void*>(&meu_release)) &&
                  gravar_ptr(&vt[kSlotPresent], reinterpret_cast<void*>(&meu_present)) &&
                  gravar_ptr(&vt[kSlotPresent1], reinterpret_cast<void*>(&meu_present1));
  g.vt_sc = vt;
  logf("FG: Release/Present/Present1 da swapchain de FG enganchados (vtable %p): %s", static_cast<void*>(vt), ok ? "ok" : "FALHOU");
}

void adotar(ffxContext ctx, IDXGISwapChain4* sc, const char* via) {
  if (g.sc && g.ctx_sc) logf("AVISO FG: swapchain de FG %p anterior ainda viva (o jogo nao soltou); a nova vale", static_cast<void*>(g.sc));
  destruir_fg();
  g.sc = sc;
  g.ctx_sc = ctx;
  ++g.e.swapchains_criadas;
  g.e.swapchain = true;
  g.e.swapchain_ptr = sc;
  g.execs_desde_present = 0;
  g.anterior_novo = false;
  enganchar_swapchain(sc);
  publicar();
  DXGI_SWAP_CHAIN_DESC1 d{};
  sc->GetDesc1(&d);
  logf("FG: swapchain de FG %p criada por %s: %ux%u, formato DXGI %u, %u buffers, efeito %u, flags 0x%X", static_cast<void*>(sc), via,
       d.Width, d.Height, static_cast<unsigned>(d.Format), d.BufferCount, static_cast<unsigned>(d.SwapEffect), d.Flags);
}

bool fila_d3d12(IUnknown* dev, ID3D12CommandQueue** fila) {
  *fila = nullptr;
  return dev && SUCCEEDED(dev->QueryInterface(IID_PPV_ARGS(fila))) && *fila;
}

HRESULT STDMETHODCALLTYPE meu_create_swapchain(IDXGIFactory* f, IUnknown* dev, DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** out) {
  if (t_criando || !desc || !out) return g.orig_cs(f, dev, desc, out);
  ID3D12CommandQueue* fila = nullptr;
  if (!fila_d3d12(dev, &fila)) return g.orig_cs(f, dev, desc, out);

  const Ffx* x = g.ffx ? g.ffx : (g.carregar ? g.carregar() : nullptr);
  if (!x) { fila->Release(); logf("ERRO FG: sem a DLL do FFX na criacao da swapchain: swapchain normal"); return g.orig_cs(f, dev, desc, out); }
  std::lock_guard<std::recursive_mutex> t(g_trava);
  g.ffx = x;
  IDXGISwapChain4* sc = nullptr;
  ffxCreateContextDescFrameGenerationSwapChainNewDX12 d{};
  d.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_NEW_DX12;
  d.swapchain = &sc;
  d.desc = desc;
  d.dxgiFactory = f;
  d.gameQueue = fila;
  ffxCreateContextDescFrameGenerationSwapChainVersionDX12 v{};
  v.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_DX12;
  v.version = FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION;
  d.header.pNext = &v.header;
  ffxContext ctx = nullptr;
  ++t_criando;
  const ffxReturnCode_t r = g.ffx->create(&ctx, &d.header, nullptr);
  --t_criando;
  fila->Release();
  if (r != FFX_API_RETURN_OK || !sc) {
    logf("ERRO FG: swapchain de FG (CreateSwapChain %ux%u fmt %u) devolveu %u: swapchain normal", desc->BufferDesc.Width,
         desc->BufferDesc.Height, static_cast<unsigned>(desc->BufferDesc.Format), r);
    return g.orig_cs(f, dev, desc, out);
  }
  adotar(ctx, sc, "CreateSwapChain");
  *out = sc;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE meu_create_swapchain_hwnd(IDXGIFactory2* f, IUnknown* dev, HWND hwnd, const DXGI_SWAP_CHAIN_DESC1* desc,
                                                    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* tela, IDXGIOutput* saida,
                                                    IDXGISwapChain1** out) {
  if (t_criando || !desc || !out || saida) return g.orig_csh(f, dev, hwnd, desc, tela, saida, out);
  ID3D12CommandQueue* fila = nullptr;
  if (!fila_d3d12(dev, &fila)) return g.orig_csh(f, dev, hwnd, desc, tela, saida, out);
  const Ffx* x = g.ffx ? g.ffx : (g.carregar ? g.carregar() : nullptr);
  if (!x) { fila->Release(); logf("ERRO FG: sem a DLL do FFX na criacao da swapchain: swapchain normal"); return g.orig_csh(f, dev, hwnd, desc, tela, saida, out); }
  std::lock_guard<std::recursive_mutex> t(g_trava);
  g.ffx = x;
  IDXGISwapChain4* sc = nullptr;
  DXGI_SWAP_CHAIN_DESC1 d1 = *desc;
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC tela_copia{};
  if (tela) tela_copia = *tela;
  ffxCreateContextDescFrameGenerationSwapChainForHwndDX12 d{};
  d.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12;
  d.swapchain = &sc;
  d.hwnd = hwnd;
  d.desc = &d1;
  d.fullscreenDesc = tela ? &tela_copia : nullptr;
  d.dxgiFactory = f;
  d.gameQueue = fila;
  ffxCreateContextDescFrameGenerationSwapChainVersionDX12 v{};
  v.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_DX12;
  v.version = FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION;
  d.header.pNext = &v.header;
  ffxContext ctx = nullptr;
  ++t_criando;
  const ffxReturnCode_t r = g.ffx->create(&ctx, &d.header, nullptr);
  --t_criando;
  fila->Release();
  if (r != FFX_API_RETURN_OK || !sc) {
    logf("ERRO FG: swapchain de FG (CreateSwapChainForHwnd %ux%u fmt %u) devolveu %u: swapchain normal", desc->Width, desc->Height,
         static_cast<unsigned>(desc->Format), r);
    return g.orig_csh(f, dev, hwnd, desc, tela, saida, out);
  }
  adotar(ctx, sc, "CreateSwapChainForHwnd");
  *out = sc;
  return S_OK;
}

void escolher_provedor(ID3D12Device* device) {
  static bool feito = false;
  if (feito) return;
  feito = true;
  const std::string& pedido = xf::ini().fg_provedor;
  uint64_t n = 0;
  ffxQueryDescGetVersions v{};
  v.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
  v.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;
  v.device = device;
  v.outputCount = &n;
  if (g.ffx->query(nullptr, &v.header) != FFX_API_RETURN_OK || n == 0 || n > 64) {
    logf("FG: nao listou provedores de FG (n=%llu)", static_cast<unsigned long long>(n));
    return;
  }
  std::vector<uint64_t> ids(static_cast<size_t>(n));
  std::vector<const char*> nomes(static_cast<size_t>(n));
  v.versionIds = ids.data();
  v.versionNames = nomes.data();
  if (g.ffx->query(nullptr, &v.header) != FFX_API_RETURN_OK) return;
  for (uint64_t i = 0; i < n; ++i) {
    const char* nome = nomes[i] ? nomes[i] : "";
    const bool escolhido = !pedido.empty() && !g.fg_provedor && std::strncmp(nome, pedido.c_str(), pedido.size()) == 0;
    if (escolhido) g.fg_provedor = ids[i];
    logf("FG: provedor %llu: id 0x%llX %s%s", static_cast<unsigned long long>(i), static_cast<unsigned long long>(ids[i]), nome,
         escolhido ? "  <- FgProvedor do INI" : "");
  }
  if (!pedido.empty() && !g.fg_provedor) logf("AVISO FG: FgProvedor=%s nao existe; o loader escolhe", pedido.c_str());
}

void mensagem_fg(uint32_t tipo, const wchar_t* msg) {
  std::string s;
  for (const wchar_t* q = msg; q && *q; ++q) s.push_back(*q < 128 ? static_cast<char>(*q) : '?');
  logf("FFX FG %s: %s", tipo == FFX_API_MESSAGE_TYPE_ERROR ? "erro" : "aviso", s.c_str());
}

bool criar_fg(const Quadro& q, uint32_t tela_w, uint32_t tela_h, DXGI_FORMAT formato) {
  escolher_provedor(q.device);
  ffxCreateBackendDX12Desc be{};
  be.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
  be.device = q.device;
  ffxCreateContextDescFrameGenerationVersion ver{};
  ver.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_VERSION;
  ver.version = FFX_FRAMEGENERATION_VERSION;
  ffxCreateContextDescFrameGeneration fd{};
  fd.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;

  fd.flags = q.flags_upscale & (FFX_FRAMEGENERATION_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS |
                                FFX_FRAMEGENERATION_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION | FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED |
                                FFX_FRAMEGENERATION_ENABLE_DEPTH_INFINITE);

  if (formato == DXGI_FORMAT_R10G10B10A2_UNORM || formato == DXGI_FORMAT_R16G16B16A16_FLOAT)
    fd.flags |= FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE;
  if (xf::ini().fg_assincrono) fd.flags |= FFX_FRAMEGENERATION_ENABLE_ASYNC_WORKLOAD_SUPPORT;
  fd.displaySize = {tela_w, tela_h};
  fd.maxRenderSize = {q.sw > tela_w ? q.sw : tela_w, q.sh > tela_h ? q.sh : tela_h};
  fd.backBufferFormat = ffxApiGetSurfaceFormatDX12(formato);
  ffxOverrideVersion ov{};
  ov.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
  ov.versionId = g.fg_provedor;
  fd.header.pNext = &be.header;
  be.header.pNext = &ver.header;
  ver.header.pNext = g.fg_provedor ? &ov.header : nullptr;
  ffxContext ctx = nullptr;
  const ffxReturnCode_t r = g.ffx->create(&ctx, &fd.header, nullptr);
  if (r != FFX_API_RETURN_OK || !ctx) {
    if (!g.aviso_criar) {
      g.aviso_criar = true;
      logf("ERRO FG: ffxCreateContext(FRAMEGENERATION) devolveu %u (tela %ux%u, teto %ux%u, formato DXGI %u, flags 0x%X): sem FG", r,
           tela_w, tela_h, fd.maxRenderSize.width, fd.maxRenderSize.height, static_cast<unsigned>(formato), fd.flags);
    }
    ++g.e.erros;
    return false;
  }
  g.fg = ctx;
  q.device->AddRef();
  g.fg_device = q.device;
  g.fg_tela_w = tela_w;
  g.fg_tela_h = tela_h;
  g.fg_max_w = fd.maxRenderSize.width;
  g.fg_max_h = fd.maxRenderSize.height;
  g.fg_flags = q.flags_upscale;
  g.fg_formato = formato;
  g.reset_proximo = true;
  g.aviso_criar = false;
  g.e.contexto = true;
  ffxQueryGetProviderVersion pv{};
  pv.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
  const bool ok = g.ffx->query(&g.fg, &pv.header) == FFX_API_RETURN_OK && pv.versionName;
  std::snprintf(g.e.provedor, sizeof g.e.provedor, "%s", ok ? pv.versionName : "?");
  logf("FG: contexto %p criado: provedor %s (id 0x%llX), tela %ux%u, teto %ux%u, formato DXGI %u (FFX %u), flags 0x%X%s", ctx,
       g.e.provedor, static_cast<unsigned long long>(pv.versionId), tela_w, tela_h, g.fg_max_w, g.fg_max_h,
       static_cast<unsigned>(formato), fd.backBufferFormat, fd.flags, xf::ini().fg_assincrono ? " (assincrono)" : "");
  return true;
}

}

void instalar(Carregar carregar) {
  if (xf::ini().geracao_quadros < 0) return;
  std::lock_guard<std::recursive_mutex> t(g_trava);
  if (g.vt_fabrica) return;
  g.carregar = carregar;
  g.usuario = false;
  g.e.usuario = g.usuario;
  {
    wchar_t nome[64];
    swprintf(nome, 64, L"Local\\NR_FG_%lu", GetCurrentProcessId());
    g.mapa = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(nrfgpub::Fg), nome);
    g.pub = g.mapa ? static_cast<nrfgpub::Fg*>(MapViewOfFile(g.mapa, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(nrfgpub::Fg))) : nullptr;
    if (g.pub) {
      g.pub->pedido = -1;
      g.pub->versao = nrfgpub::kVersao;
      publicar();
      MemoryBarrier();
      g.pub->magia = nrfgpub::kMagia;
    }
    logf("FG: memoria do menu %ls %s (FG %s na abertura)", nome, g.pub ? "criada" : "FALHOU", g.usuario ? "ligado" : "desligado");
  }
  HMODULE dxgi = GetModuleHandleW(L"dxgi.dll");
  if (!dxgi) dxgi = LoadLibraryW(L"dxgi.dll");
  using PfnCriarFabrica = HRESULT(WINAPI*)(REFIID, void**);
  auto criar = dxgi ? reinterpret_cast<PfnCriarFabrica>(GetProcAddress(dxgi, "CreateDXGIFactory1")) : nullptr;
  IDXGIFactory2* f = nullptr;
  if (!criar || FAILED(criar(IID_PPV_ARGS(&f))) || !f) {
    logf("ERRO FG: sem IDXGIFactory2 (dxgi %p): sem geracao de quadros", static_cast<void*>(dxgi));
    return;
  }
  void** vt = *reinterpret_cast<void***>(f);
  g.orig_cs = reinterpret_cast<PfnCreateSwapChain>(vt[kSlotCreateSwapChain]);
  g.orig_csh = reinterpret_cast<PfnCreateSwapChainForHwnd>(vt[kSlotCreateSwapChainForHwnd]);
  const bool ok = gravar_ptr(&vt[kSlotCreateSwapChain], reinterpret_cast<void*>(&meu_create_swapchain)) &&
                  gravar_ptr(&vt[kSlotCreateSwapChainForHwnd], reinterpret_cast<void*>(&meu_create_swapchain_hwnd));
  f->Release();
  if (!ok) {
    logf("ERRO FG: VirtualProtect na vtable da fabrica falhou: sem geracao de quadros");
    return;
  }
  g.vt_fabrica = vt;
  g.e.gancho_fabrica = true;
  HMODULE m1 = nullptr, m2 = nullptr;
  GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     reinterpret_cast<LPCWSTR>(g.orig_cs), &m1);
  GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     reinterpret_cast<LPCWSTR>(g.orig_csh), &m2);
  wchar_t n1[MAX_PATH]{}, n2[MAX_PATH]{};
  if (m1) GetModuleFileNameW(m1, n1, MAX_PATH);
  if (m2) GetModuleFileNameW(m2, n2, MAX_PATH);
  std::string s1, s2;
  for (const wchar_t* q = n1; *q; ++q) s1.push_back(*q < 128 ? static_cast<char>(*q) : '?');
  for (const wchar_t* q = n2; *q; ++q) s2.push_back(*q < 128 ? static_cast<char>(*q) : '?');
  logf("FG: GeracaoQuadros=%d, gancho na fabrica DXGI (vtable %p): CreateSwapChain era %p (%s), CreateSwapChainForHwnd era %p (%s). "
       "Swapchain criada antes disto fica sem FG.",
       xf::ini().geracao_quadros, static_cast<void*>(vt), reinterpret_cast<void*>(g.orig_cs), s1.c_str(), reinterpret_cast<void*>(g.orig_csh), s2.c_str());
}

void por_execucao(const Quadro& q) {
  if (xf::ini().geracao_quadros < 0 || !g.vt_fabrica) return;
  const Espera esp = ordem_esperar();
  std::lock_guard<std::recursive_mutex> t(g_trava);
  if (esp.esperou) {
    ++g.e.ordem_esperas;
    g.e.ordem_espera_ms += esp.ms;
    if (esp.estourou) {
      ++g.e.ordem_estouros;
      if (++g_estouros_seguidos >= 8 && !g_ordem_off.exchange(true)) {
        g.e.ordem_desligada = true;
        logf("AVISO FG: FgOrdem: 8 Executes seguidos sem o Present do quadro anterior em %d ms: a espera sai nesta sessao", xf::ini().fg_ordem_ms);
      }
    } else g_estouros_seguidos = 0;
    if (g.e.ordem_esperas <= 8)
      logf("FG ordem: o Execute (thread %lu) esperou %.2f ms o Present do frameID %llu%s", GetCurrentThreadId(), esp.ms,
           static_cast<unsigned long long>(g.e.frame_id), esp.estourou ? " (ESTOUROU FgOrdemMs: segue)" : "");
  }
  ++g.execs_desde_present;

  if (!g.aviso_removido && q.device) {
    const HRESULT rr = q.device->GetDeviceRemovedReason();
    if (FAILED(rr)) {
      g.aviso_removido = true;
      logf("ERRO FG: device removido 0x%08lX (FG %s, provedor %s, frameID %llu, geracoes %llu, prepares %llu, Presents %llu)",
           static_cast<unsigned long>(rr), g.fg_ligado ? "ligado" : "desligado", g.e.provedor,
           static_cast<unsigned long long>(g.e.frame_id), static_cast<unsigned long long>(g.e.geracoes),
           static_cast<unsigned long long>(g.e.prepares), static_cast<unsigned long long>(g.e.presents));
    }
  }
  aplicar_pedido();

  if (!g.aviso_slot && (g.vt_fabrica[kSlotCreateSwapChain] != reinterpret_cast<void*>(&meu_create_swapchain) ||
                        g.vt_fabrica[kSlotCreateSwapChainForHwnd] != reinterpret_cast<void*>(&meu_create_swapchain_hwnd))) {
    g.aviso_slot = true;
    g.e.gancho_fabrica = false;
    logf("AVISO FG: os slots da fabrica DXGI nao sao mais nossos (%p / %p): outra swapchain criada agora fica sem FG",
         g.vt_fabrica[kSlotCreateSwapChain], g.vt_fabrica[kSlotCreateSwapChainForHwnd]);
  }
  if (!g.sc) {
    if (!g.aviso_sem_sc) {
      g.aviso_sem_sc = true;
      logf("AVISO FG: Execute sem swapchain de FG (o jogo criou a swapchain antes da libxess carregar, ou a criacao falhou): sem FG "
           "ate o jogo recriar a swapchain");
    }
    return;
  }
  if (!g.usuario) {
    if (g.fg_ligado) configurar(false);
    publicar();
    return;
  }
  if (!g.ffx || !q.device || !q.lista || !q.profundidade || !q.vetores) return;
  DXGI_SWAP_CHAIN_DESC1 d{};
  if (FAILED(g.sc->GetDesc1(&d)) || !d.Width || !d.Height) return;

  if (g.fg && (g.fg_device != q.device || g.fg_tela_w != d.Width || g.fg_tela_h != d.Height || q.sw > g.fg_max_w ||
               q.sh > g.fg_max_h || g.fg_flags != q.flags_upscale || g.fg_formato != d.Format)) {
    logf("FG: recriando o contexto (tela %ux%u -> %ux%u, saida %ux%u, flags 0x%X -> 0x%X, formato DXGI %u -> %u)", g.fg_tela_w, g.fg_tela_h,
         d.Width, d.Height, q.sw, q.sh, g.fg_flags, q.flags_upscale, static_cast<unsigned>(g.fg_formato), static_cast<unsigned>(d.Format));
    destruir_fg();
  }
  if (!g.fg && !criar_fg(q, d.Width, d.Height, d.Format)) return;

  configurar(true);
  ffxDispatchDescFrameGenerationPrepareV2 p{};
  p.header.type = FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_V2;
  p.frameID = g.e.frame_id;
  p.flags = static_cast<uint32_t>(xf::ini().fg_debug) &
            (FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_TEAR_LINES | FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_RESET_INDICATORS |
             FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_PACING_LINES);
  p.commandList = q.lista;
  p.renderSize = {q.ew, q.eh};
  p.jitterOffset = {q.jx, q.jy};
  p.motionVectorScale = {q.mvx, q.mvy};
  p.frameTimeDelta = q.delta_ms;
  p.reset = q.reset || g.reset_proximo;
  p.cameraNear = q.perto;
  p.cameraFar = q.longe;
  p.cameraFovAngleVertical = q.fov_rad;
  p.viewSpaceToMetersFactor = 1.0f;
  FfxApiResource prof = ffxApiGetResourceDX12(q.profundidade, FFX_API_RESOURCE_STATE_COMPUTE_READ);
  FfxApiResource vet = ffxApiGetResourceDX12(q.vetores, FFX_API_RESOURCE_STATE_COMPUTE_READ);
  if (prof.description.format == FFX_API_SURFACE_FORMAT_R32_TYPELESS) prof.description.format = FFX_API_SURFACE_FORMAT_R32_FLOAT;
  if (vet.description.format == FFX_API_SURFACE_FORMAT_R16G16_TYPELESS) vet.description.format = FFX_API_SURFACE_FORMAT_R16G16_FLOAT;
  p.depth = prof;
  p.motionVectors = vet;

  p.cameraPosition[0] = p.cameraPosition[1] = p.cameraPosition[2] = 0.0f;
  p.cameraUp[1] = 1.0f;
  p.cameraRight[0] = 1.0f;
  p.cameraForward[2] = 1.0f;
  const ffxReturnCode_t r = g.ffx->dispatch(&g.fg, &p.header);
  ++g.e.prepares;
  if (r == FFX_API_RETURN_OK) ordem_marcar(true);
  if (r != FFX_API_RETURN_OK) {
    if (g.e.erros++ < 16) logf("ERRO FG: prepare devolveu %u (frameID %llu)", r, static_cast<unsigned long long>(p.frameID));
    return;
  }
  if (g.e.prepares <= 3)
    logf("FG prepare %llu (thread %lu): frameID %llu, entrada %ux%u, jitter %.4f %.4f, vetores x%g %g, delta %.2f ms, reset %d, camera %g/%g/%.3f, "
         "prof fmt %u, vetores fmt %u",
         static_cast<unsigned long long>(g.e.prepares), GetCurrentThreadId(), static_cast<unsigned long long>(p.frameID), q.ew, q.eh, q.jx, q.jy, q.mvx, q.mvy,
         q.delta_ms, p.reset ? 1 : 0, q.perto, q.longe, q.fov_rad, prof.description.format, vet.description.format);
  if (g.e.prepares % 3000 == 0)
    logf("FG: %llu prepares, %llu Presents (%llu pulados, %llu de teste), desligado %llu vezes, %llu erros; ordem: %llu Presents sem "
         "Execute antes, %llu com 2+ Executes antes, %llu esperas do Execute (media %.2f ms), %llu estouros%s",
         static_cast<unsigned long long>(g.e.prepares), static_cast<unsigned long long>(g.e.presents),
         static_cast<unsigned long long>(g.e.pulados), static_cast<unsigned long long>(g.e.presents_teste),
         static_cast<unsigned long long>(g.e.desligados), static_cast<unsigned long long>(g.e.erros),
         static_cast<unsigned long long>(g.e.presents_sem_exec), static_cast<unsigned long long>(g.e.presents_varios_exec),
         static_cast<unsigned long long>(g.e.ordem_esperas), g.e.ordem_esperas ? g.e.ordem_espera_ms / double(g.e.ordem_esperas) : 0.0,
         static_cast<unsigned long long>(g.e.ordem_estouros), g.e.ordem_desligada ? " (FgOrdem desligado)" : "");
  g.reset_proximo = false;
  publicar();
}

void soltar_contexto() {
  std::lock_guard<std::recursive_mutex> t(g_trava);
  destruir_fg();
}

Estado estado() {
  std::lock_guard<std::recursive_mutex> t(g_trava);
  return g.e;
}

}

extern "C" void nrXessFsrFgEstado(xfg::Estado* saida) {
  if (saida) *saida = xfg::estado();
}
