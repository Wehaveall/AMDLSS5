#include <windows.h>
#include <d3d12.h>

#include <xess/xess.h>
#include <xess/xess_d3d12.h>
#include <xess/xess_debug.h>
#include <xess/xess_d3d12_debug.h>

#include <ffx_api.h>
#include <ffx_api_types.h>
#include <dx12/ffx_api_dx12.h>
#include <ffx_upscale.h>

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "estencil_publico.h"
#include "geracao_quadros.h"
#include "registro.h"
#include "traducao.h"

static_assert(xf::XESS_HIGH_RES_MV == XESS_INIT_FLAG_HIGH_RES_MV);
static_assert(xf::XESS_INVERTED_DEPTH == XESS_INIT_FLAG_INVERTED_DEPTH);
static_assert(xf::XESS_EXPOSURE_SCALE_TEXTURE == XESS_INIT_FLAG_EXPOSURE_SCALE_TEXTURE);
static_assert(xf::XESS_RESPONSIVE_PIXEL_MASK == XESS_INIT_FLAG_RESPONSIVE_PIXEL_MASK);
static_assert(xf::XESS_USE_NDC_VELOCITY == XESS_INIT_FLAG_USE_NDC_VELOCITY);
static_assert(xf::XESS_EXTERNAL_DESCRIPTOR_HEAP == XESS_INIT_FLAG_EXTERNAL_DESCRIPTOR_HEAP);
static_assert(xf::XESS_LDR_INPUT_COLOR == XESS_INIT_FLAG_LDR_INPUT_COLOR);
static_assert(xf::XESS_JITTERED_MV == XESS_INIT_FLAG_JITTERED_MV);
static_assert(xf::XESS_ENABLE_AUTOEXPOSURE == XESS_INIT_FLAG_ENABLE_AUTOEXPOSURE);
static_assert(xf::FFX_HDR == FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE);
static_assert(xf::FFX_DISPLAY_RES_MV == FFX_UPSCALE_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS);
static_assert(xf::FFX_MV_JITTER_CANCELLATION == FFX_UPSCALE_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION);
static_assert(xf::FFX_DEPTH_INVERTED == FFX_UPSCALE_ENABLE_DEPTH_INVERTED);
static_assert(xf::FFX_DEPTH_INFINITE == FFX_UPSCALE_ENABLE_DEPTH_INFINITE);
static_assert(xf::FFX_AUTO_EXPOSURE == FFX_UPSCALE_ENABLE_AUTO_EXPOSURE);
static_assert(xf::XESS_Q_ULTRA_PERFORMANCE == XESS_QUALITY_SETTING_ULTRA_PERFORMANCE);
static_assert(xf::XESS_Q_PERFORMANCE == XESS_QUALITY_SETTING_PERFORMANCE);
static_assert(xf::XESS_Q_BALANCED == XESS_QUALITY_SETTING_BALANCED);
static_assert(xf::XESS_Q_QUALITY == XESS_QUALITY_SETTING_QUALITY);
static_assert(xf::XESS_Q_ULTRA_QUALITY == XESS_QUALITY_SETTING_ULTRA_QUALITY);
static_assert(xf::XESS_Q_ULTRA_QUALITY_PLUS == XESS_QUALITY_SETTING_ULTRA_QUALITY_PLUS);
static_assert(xf::XESS_Q_AA == XESS_QUALITY_SETTING_AA);

static_assert(offsetof(ffxDispatchDescUpscale, cameraNear) == 0x19C);

static_assert(offsetof(xess_d3d12_execute_params_t, pOutputTexture) == 0x28);
static_assert(offsetof(xess_d3d12_execute_params_t, jitterOffsetX) == 0x30);
static_assert(offsetof(xess_d3d12_execute_params_t, jitterOffsetY) == 0x34);
static_assert(offsetof(xess_d3d12_execute_params_t, exposureScale) == 0x38);
static_assert(offsetof(xess_d3d12_execute_params_t, resetHistory) == 0x3C);
static_assert(offsetof(xess_d3d12_execute_params_t, inputWidth) == 0x40);
static_assert(offsetof(xess_d3d12_execute_params_t, inputHeight) == 0x44);
static_assert(offsetof(xess_d3d12_execute_params_t, inputColorBase) == 0x48);
static_assert(offsetof(xess_d3d12_execute_params_t, outputColorBase) == 0x70);
static_assert(offsetof(xess_d3d12_execute_params_t, pDescriptorHeap) == 0x78);
static_assert(offsetof(xess_d3d12_execute_params_t, descriptorHeapOffset) == 0x80);

namespace {

using xf::logf;

constexpr uint32_t kMagia = 0x52465846u;
constexpr uint64_t kQuadrosAteDestruir = 8;
constexpr uint64_t kExecPorLinha = 300;

struct Runtime {
  bool tentou = false;
  HMODULE modulo = nullptr;
  std::string nome;
  PfnFfxCreateContext create = nullptr;
  PfnFfxDestroyContext destroy = nullptr;
  PfnFfxConfigure configure = nullptr;
  PfnFfxQuery query = nullptr;
  PfnFfxDispatch dispatch = nullptr;
  unsigned char inicio_create[16]{};
  unsigned char inicio_dispatch[16]{};
  bool gancho_visto = false;
};
Runtime g_rt;
std::recursive_mutex g_mutex;

std::string estreito(const std::wstring& w) {
  std::string s;
  for (wchar_t c : w) s.push_back(c < 128 ? static_cast<char>(c) : '?');
  return s;
}

void copiar_inicio(void* fn, unsigned char (&destino)[16]) {
  if (fn) std::memcpy(destino, fn, sizeof destino);
}

bool carregar_ffx() {
  if (g_rt.tentou) return g_rt.modulo != nullptr;
  g_rt.tentou = true;
  const std::wstring pasta = xf::pasta_da_dll();
  std::vector<std::wstring> nomes;
  if (!xf::ini().ffx_dll.empty()) nomes.push_back(xf::ini().ffx_dll);
  nomes.push_back(L"amd_fidelityfx_loader_dx12.dll");
  nomes.push_back(L"amd_fidelityfx_dx12.dll");
  for (const auto& nome : nomes) {

    HMODULE m = GetModuleHandleW(nome.c_str());
    const std::wstring caminho = pasta + L"\\" + nome;
    if (!m) {
      if (GetFileAttributesW(caminho.c_str()) == INVALID_FILE_ATTRIBUTES) {
        logf("FFX: %s nao existe na pasta da DLL", estreito(nome).c_str());
        continue;
      }
      m = LoadLibraryExW(caminho.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
      if (!m) {
        logf("ERRO FFX: LoadLibrary(%s) falhou, Win32 %lu", estreito(caminho).c_str(), GetLastError());
        continue;
      }
    }
    g_rt.create = reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(m, "ffxCreateContext"));
    g_rt.destroy = reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(m, "ffxDestroyContext"));
    g_rt.configure = reinterpret_cast<PfnFfxConfigure>(GetProcAddress(m, "ffxConfigure"));
    g_rt.query = reinterpret_cast<PfnFfxQuery>(GetProcAddress(m, "ffxQuery"));
    g_rt.dispatch = reinterpret_cast<PfnFfxDispatch>(GetProcAddress(m, "ffxDispatch"));
    if (!g_rt.create || !g_rt.destroy || !g_rt.configure || !g_rt.query || !g_rt.dispatch) {
      logf("ERRO FFX: %s sem os exports ffx*", estreito(nome).c_str());
      continue;
    }
    g_rt.modulo = m;
    g_rt.nome = estreito(nome);
    copiar_inicio(reinterpret_cast<void*>(g_rt.create), g_rt.inicio_create);
    copiar_inicio(reinterpret_cast<void*>(g_rt.dispatch), g_rt.inicio_dispatch);
    logf("FFX: rota %s (%s), ffxCreateContext %p ffxDispatch %p, inicio %02X %02X %02X %02X %02X",
         g_rt.nome.c_str(), estreito(caminho).c_str(), reinterpret_cast<void*>(g_rt.create),
         reinterpret_cast<void*>(g_rt.dispatch), g_rt.inicio_create[0], g_rt.inicio_create[1],
         g_rt.inicio_create[2], g_rt.inicio_create[3], g_rt.inicio_create[4]);
    return true;
  }
  logf("ERRO FFX: nenhuma DLL da FidelityFX API na pasta %s (copiar amd_fidelityfx_loader_dx12.dll + "
       "amd_fidelityfx_upscaler_dx12.dll)", estreito(pasta).c_str());
  return false;
}

bool gancho_chegou() {
  if (!g_rt.modulo) return false;
  if (g_rt.gancho_visto) return true;
  const bool mudou = std::memcmp(reinterpret_cast<void*>(g_rt.create), g_rt.inicio_create, 16) != 0;
  if (mudou) {
    g_rt.gancho_visto = true;
    const auto* p = reinterpret_cast<const unsigned char*>(g_rt.create);
    const bool mudou_dispatch = std::memcmp(reinterpret_cast<void*>(g_rt.dispatch), g_rt.inicio_dispatch, 16) != 0;
    logf("gancho em %s!ffxCreateContext chegou (inicio agora %02X %02X %02X %02X %02X; ffxDispatch %s)",
         g_rt.nome.c_str(), p[0], p[1], p[2], p[3], p[4], mudou_dispatch ? "tambem" : "ainda nao");
  }
  return mudou;
}

void mensagem_ffx(uint32_t tipo, const wchar_t* msg) {
  logf("FFX %s: %s", tipo == FFX_API_MESSAGE_TYPE_ERROR ? "erro" : "aviso", msg ? estreito(msg).c_str() : "");
}

xess_result_t de_ffx(ffxReturnCode_t r) {
  switch (r) {
    case FFX_API_RETURN_OK: return XESS_RESULT_SUCCESS;
    case FFX_API_RETURN_ERROR_PARAMETER: return XESS_RESULT_ERROR_INVALID_ARGUMENT;
    case FFX_API_RETURN_ERROR_MEMORY: return XESS_RESULT_ERROR_DEVICE_OUT_OF_MEMORY;
    case FFX_API_RETURN_ERROR_RUNTIME_ERROR: return XESS_RESULT_ERROR_DEVICE;
    default: return XESS_RESULT_ERROR_UNKNOWN;
  }
}

uint32_t resolver_typeless(uint32_t f) {
  switch (f) {
    case FFX_API_SURFACE_FORMAT_R32_TYPELESS: return FFX_API_SURFACE_FORMAT_R32_FLOAT;
    case FFX_API_SURFACE_FORMAT_R16_TYPELESS: return FFX_API_SURFACE_FORMAT_R16_FLOAT;
    case FFX_API_SURFACE_FORMAT_R16G16_TYPELESS: return FFX_API_SURFACE_FORMAT_R16G16_FLOAT;
    case FFX_API_SURFACE_FORMAT_R32G32_TYPELESS: return FFX_API_SURFACE_FORMAT_R32G32_FLOAT;
    case FFX_API_SURFACE_FORMAT_R16G16B16A16_TYPELESS: return FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT;
    case FFX_API_SURFACE_FORMAT_R32G32B32A32_TYPELESS: return FFX_API_SURFACE_FORMAT_R32G32B32A32_FLOAT;
    case FFX_API_SURFACE_FORMAT_R8G8B8A8_TYPELESS: return FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case FFX_API_SURFACE_FORMAT_B8G8R8A8_TYPELESS: return FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM;
    case FFX_API_SURFACE_FORMAT_R10G10B10A2_TYPELESS: return FFX_API_SURFACE_FORMAT_R10G10B10A2_UNORM;
    case FFX_API_SURFACE_FORMAT_R8_TYPELESS: return FFX_API_SURFACE_FORMAT_R8_UNORM;
    case FFX_API_SURFACE_FORMAT_R8G8_TYPELESS: return FFX_API_SURFACE_FORMAT_R8G8_UNORM;
    default: return f;
  }
}

FfxApiResource recurso(ID3D12Resource* r, uint32_t estado, uint32_t uso_extra = 0) {
  FfxApiResource x = ffxApiGetResourceDX12(r, estado, uso_extra);
  x.description.format = resolver_typeless(x.description.format);
  return x;
}

struct Contexto {
  uint32_t magia = kMagia;
  ID3D12Device* device = nullptr;
  bool iniciado = false;
  xess_d3d12_init_params_t init{};
  uint32_t flags_ffx = 0;
  ffxContext ffx = nullptr;
  bool ffx_visto_pelo_gancho = false;
  bool reset_pendente = false;
  xf::Par jitter_scale{1.0f, 1.0f};
  xf::Par velocity_scale{1.0f, 1.0f};
  float exposicao_mult = 1.0f;
  float max_responsiva = 1.0f;
  xess_app_log_callback_t callback = nullptr;
  xess_logging_level_t nivel_callback = XESS_LOGGING_LEVEL_ERROR;
  int64_t qpc_anterior = 0;
  uint64_t execucoes = 0;
  uint64_t erros = 0;
  xf::JanelaExec janela;
  bool aviso_subrect = false, aviso_exposicao = false, aviso_heap = false;
  struct Aposentado { ffxContext ffx; uint64_t ate; };
  std::vector<Aposentado> aposentados;

  bool formato_logado = false;
  bool tecla_antes = false;
  uint64_t estencil_inicio = 0;
  uint32_t estencil_sessao = 0;
  struct Captura {
    ID3D12Resource* leitura = nullptr;
    uint64_t pronta = 0;
    uint64_t execucao = 0;
    uint32_t sessao = 0, indice = 0;
    uint32_t formato_prof = 0, formato_cor = 0;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT pe[3]{};
    UINT linhas[3]{};
    UINT64 bytes_linha[3]{};
  };
  std::vector<Captura> capturas;

  struct Publicacao {
    ID3D12Resource* buf = nullptr;
    HANDLE handle = nullptr;
    uint32_t geracao = 0, largura = 0, altura = 0, pitch = 0, formato = 0;
    uint64_t bytes = 0;
    uint64_t ate = 0;
  };
  Publicacao pub;
  std::vector<Publicacao> pub_velhas;
  bool pub_aviso = false;
  uint64_t pub_n = 0;
};
std::set<Contexto*> g_contextos;

Contexto* achar(xess_context_handle_t h) {
  auto* c = reinterpret_cast<Contexto*>(h);
  if (!c || !g_contextos.count(c) || c->magia != kMagia) return nullptr;
  return c;
}

void erro(Contexto* c, const char* texto) {
  logf("ERRO %s", texto);
  if (c) {
    ++c->erros;
    if (c->callback && XESS_LOGGING_LEVEL_ERROR >= c->nivel_callback) c->callback(texto, XESS_LOGGING_LEVEL_ERROR);
  }
}

void destruir_ffx(ffxContext& f) {
  if (f && g_rt.destroy) {
    const ffxReturnCode_t r = g_rt.destroy(&f, nullptr);
    if (r != FFX_API_RETURN_OK) logf("ERRO ffxDestroyContext devolveu %u", r);
  }
  f = nullptr;
}

void aposentar(Contexto* c) {
  if (!c->ffx) return;
  c->aposentados.push_back({c->ffx, c->execucoes + kQuadrosAteDestruir});
  c->ffx = nullptr;
}

void soltar_publicacao(Contexto::Publicacao& p) {
  if (p.buf) p.buf->Release();
  if (p.handle) CloseHandle(p.handle);
  p = {};
}
void despublicar();

void destruir_tudo(Contexto* c) {
  destruir_ffx(c->ffx);
  for (auto& a : c->aposentados) destruir_ffx(a.ffx);
  c->aposentados.clear();
  for (auto& k : c->capturas)
    if (k.leitura) k.leitura->Release();
  c->capturas.clear();
  if (c->pub.buf) despublicar();
  soltar_publicacao(c->pub);
  for (auto& v : c->pub_velhas) soltar_publicacao(v);
  c->pub_velhas.clear();
}

constexpr uint64_t kExecAteLer = 16;

void estencil_por_execucao(Contexto*, ID3D12GraphicsCommandList*, const xess_d3d12_execute_params_t*) {}

nrest::Publicado* g_pub = nullptr;
uint32_t g_pub_geracao = 0;

nrest::Publicado* memoria_publicada() {
  static bool tentou = false;
  if (g_pub || tentou) return g_pub;
  tentou = true;
  wchar_t nome[64];
  swprintf(nome, 64, L"Local\\NR_ESTENCIL_%lu", GetCurrentProcessId());
  HANDLE m = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(nrest::Publicado), nome);
  if (!m) { logf("ERRO estencil publicado: CreateFileMapping falhou, Win32 %lu", GetLastError()); return nullptr; }
  g_pub = static_cast<nrest::Publicado*>(MapViewOfFile(m, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(nrest::Publicado)));
  if (!g_pub) { logf("ERRO estencil publicado: MapViewOfFile falhou, Win32 %lu", GetLastError()); CloseHandle(m); return nullptr; }

  g_pub->seq = 0;
  g_pub->ok = 0;
  g_pub->versao = nrest::kVersao;
  MemoryBarrier();
  g_pub->magia = nrest::kMagia;
  logf("estencil publicado: memoria Local\\NR_ESTENCIL_%lu criada (%u bytes)", GetCurrentProcessId(),
       static_cast<unsigned>(sizeof(nrest::Publicado)));
  return g_pub;
}

void escrever_publicado(const Contexto::Publicacao& p, uint64_t execucao, LUID luid, bool ok) {
  nrest::Publicado* m = memoria_publicada();
  if (!m) return;
  InterlockedIncrement(&m->seq);
  MemoryBarrier();
  m->ok = ok ? 1u : 0u;
  m->handle = reinterpret_cast<uint64_t>(p.handle);
  m->geracao = p.geracao;
  m->largura = p.largura;
  m->altura = p.altura;
  m->pitch = p.pitch;
  m->execucao = execucao;
  m->tick_ms = GetTickCount64();
  m->luid_baixo = luid.LowPart;
  m->luid_alto = luid.HighPart;
  MemoryBarrier();
  InterlockedIncrement(&m->seq);
}

void despublicar() {
  if (!g_pub) return;
  InterlockedIncrement(&g_pub->seq);
  MemoryBarrier();
  g_pub->ok = 0;
  MemoryBarrier();
  InterlockedIncrement(&g_pub->seq);
}

void estencil_publicar(Contexto* c, ID3D12GraphicsCommandList* lista, const xess_d3d12_execute_params_t* p) {
  const auto& ini = xf::ini();
  if (!ini.publicar_estencil || !c->device) return;
  for (size_t n = 0; n < c->pub_velhas.size();) {
    if (c->execucoes >= c->pub_velhas[n].ate) {
      soltar_publicacao(c->pub_velhas[n]);
      c->pub_velhas.erase(c->pub_velhas.begin() + static_cast<std::ptrdiff_t>(n));
    } else {
      ++n;
    }
  }
  ID3D12Resource* prof = p->pDepthTexture;
  const D3D12_RESOURCE_DESC dp = prof ? prof->GetDesc() : D3D12_RESOURCE_DESC{};
  if (!prof || !xf::formato_com_estencil(static_cast<uint32_t>(dp.Format))) {
    if (!c->pub_aviso) {
      c->pub_aviso = true;
      logf("AVISO estencil publicado: profundidade %s (dxgi %u): nada publicado", prof ? "sem stencil" : "ausente",
           static_cast<unsigned>(dp.Format));
    }
    if (c->pub.buf) despublicar();
    return;
  }
  const UINT mips = dp.MipLevels ? dp.MipLevels : 1, fatias = dp.DepthOrArraySize ? dp.DepthOrArraySize : 1;
  const UINT sub = xf::subrecurso(0, 0, 1, mips, fatias);
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT pe{};
  UINT linhas = 0;
  UINT64 bytes_linha = 0, total = 0;
  c->device->GetCopyableFootprints(&dp, sub, 1, 0, &pe, &linhas, &bytes_linha, &total);
  if (total == UINT64(-1) || !total || !linhas) {
    if (!c->pub_aviso) { c->pub_aviso = true; logf("ERRO estencil publicado: footprint do plano 1 invalido"); }
    return;
  }
  const uint32_t fmt = static_cast<uint32_t>(dp.Format);
  if (!c->pub.buf || c->pub.largura != pe.Footprint.Width || c->pub.altura != linhas || c->pub.pitch != pe.Footprint.RowPitch ||
      c->pub.formato != fmt) {
    if (c->pub.buf) {
      Contexto::Publicacao v = c->pub;
      v.ate = c->execucoes + kQuadrosAteDestruir;
      c->pub_velhas.push_back(v);
      c->pub = {};
    }
    D3D12_HEAP_PROPERTIES hp{};
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC bd{};
    bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bd.Width = (total + 255) & ~UINT64(255);
    bd.Height = 1;
    bd.DepthOrArraySize = 1;
    bd.MipLevels = 1;
    bd.SampleDesc.Count = 1;
    bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Contexto::Publicacao n;
    HRESULT hr = c->device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_SHARED, &bd, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                    IID_PPV_ARGS(&n.buf));
    if (SUCCEEDED(hr)) hr = c->device->CreateSharedHandle(n.buf, nullptr, GENERIC_ALL, nullptr, &n.handle);
    if (FAILED(hr)) {
      if (!c->pub_aviso) {
        c->pub_aviso = true;
        logf("ERRO estencil publicado: buffer compartilhado de %llu bytes nao foi criado (0x%08lX)",
             static_cast<unsigned long long>(bd.Width), static_cast<unsigned long>(hr));
      }
      soltar_publicacao(n);
      despublicar();
      return;
    }
    n.buf->SetName(L"nr_estencil_publicado");
    n.geracao = ++g_pub_geracao;
    n.largura = pe.Footprint.Width;
    n.altura = linhas;
    n.pitch = pe.Footprint.RowPitch;
    n.formato = fmt;
    n.bytes = bd.Width;
    c->pub = n;
    logf("ESTENCIL_PUBLICADO geracao %u: buffer %llu bytes, stencil %ux%u pitch %u (depth dxgi %u, plano 1 dxgi %u), handle %p, "
         "execucao %llu", n.geracao, static_cast<unsigned long long>(n.bytes), n.largura, n.altura, n.pitch, fmt,
         static_cast<unsigned>(pe.Footprint.Format), static_cast<void*>(n.handle), static_cast<unsigned long long>(c->execucoes + 1));
  }
  const auto estado = static_cast<D3D12_RESOURCE_STATES>(ini.gravar_estencil_estado);
  D3D12_RESOURCE_BARRIER br[2]{};
  br[0].Type = br[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  br[0].Transition = {prof, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, estado, D3D12_RESOURCE_STATE_COPY_SOURCE};
  br[1].Transition = {c->pub.buf, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST};
  lista->ResourceBarrier(2, br);
  D3D12_TEXTURE_COPY_LOCATION de{}, para{};
  de.pResource = prof;
  de.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  de.SubresourceIndex = sub;
  para.pResource = c->pub.buf;
  para.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  para.PlacedFootprint = pe;
  lista->CopyTextureRegion(&para, 0, 0, 0, &de, nullptr);
  br[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
  br[0].Transition.StateAfter = estado;
  br[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  br[1].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  lista->ResourceBarrier(2, br);
  escrever_publicado(c->pub, c->execucoes + 1, c->device->GetAdapterLuid(), true);
  if (++c->pub_n == 1 || (c->pub_n % 3000) == 0)
    logf("estencil publicado: %llu copias (execucao %llu, geracao %u)", static_cast<unsigned long long>(c->pub_n),
         static_cast<unsigned long long>(c->execucoes + 1), c->pub.geracao);
}

uint64_t g_provedor_escolhido = 0;

void listar_provedores(ID3D12Device* device) {
  static bool feito = false;
  if (feito) return;
  feito = true;
  const std::string& pedido = xf::ini().provedor;
  uint64_t n = 0;
  ffxQueryDescGetVersions v{};
  v.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
  v.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
  v.device = device;
  v.outputCount = &n;
  if (g_rt.query(nullptr, &v.header) != FFX_API_RETURN_OK || n == 0 || n > 64) {
    logf("FFX: nao listou provedores (n=%llu)", static_cast<unsigned long long>(n));
    return;
  }
  std::vector<uint64_t> ids(static_cast<size_t>(n));
  std::vector<const char*> nomes(static_cast<size_t>(n));
  v.versionIds = ids.data();
  v.versionNames = nomes.data();
  if (g_rt.query(nullptr, &v.header) != FFX_API_RETURN_OK) return;
  for (uint64_t i = 0; i < n && i < ids.size(); ++i) {
    const char* nome = nomes[i] ? nomes[i] : "";
    const bool escolhido = !pedido.empty() && !g_provedor_escolhido && std::strncmp(nome, pedido.c_str(), pedido.size()) == 0;
    if (escolhido) g_provedor_escolhido = ids[i];
    logf("FFX: provedor %llu: id 0x%llX %s%s", static_cast<unsigned long long>(i),
         static_cast<unsigned long long>(ids[i]), nome, escolhido ? "  <- Provedor do INI" : "");
  }
  if (!pedido.empty() && !g_provedor_escolhido)
    logf("AVISO: Provedor=%s do INI nao existe; o loader escolhe", pedido.c_str());
}

xess_result_t criar_ffx(Contexto* c) {
  if (!carregar_ffx()) return XESS_RESULT_ERROR_CANT_LOAD_LIBRARY;
  listar_provedores(c->device);
  const bool com_gancho = gancho_chegou();
  ffxCreateBackendDX12Desc be{};
  be.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
  be.device = c->device;
  ffxCreateContextDescUpscaleVersion ver{};
  ver.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE_VERSION;
  ver.version = FFX_UPSCALER_VERSION;
  ffxCreateContextDescUpscale up{};
  up.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
  up.flags = c->flags_ffx;

  up.maxRenderSize = {c->init.outputResolution.x, c->init.outputResolution.y};
  up.maxUpscaleSize = {c->init.outputResolution.x, c->init.outputResolution.y};
  up.fpMessage = &mensagem_ffx;
  ffxOverrideVersion ov{};
  ov.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
  ov.versionId = g_provedor_escolhido;
  const bool usar_versao = xf::ini().descritor_versao != 0;

  up.header.pNext = &be.header;
  ffxApiHeader* fim = &be.header;
  if (g_provedor_escolhido) { fim->pNext = &ov.header; fim = &ov.header; }
  fim->pNext = usar_versao ? &ver.header : nullptr;
  ffxReturnCode_t r = g_rt.create(&c->ffx, &up.header, nullptr);
  if (r != FFX_API_RETURN_OK && usar_versao) {
    logf("ffxCreateContext com o descritor de versao devolveu %u: tentando sem ele", r);
    fim->pNext = nullptr;
    c->ffx = nullptr;
    r = g_rt.create(&c->ffx, &up.header, nullptr);
  }
  if (r != FFX_API_RETURN_OK || !c->ffx) {
    c->ffx = nullptr;
    char t[160];
    std::snprintf(t, sizeof t, "ffxCreateContext devolveu %u (flags 0x%X, %ux%u)", r, c->flags_ffx,
                  c->init.outputResolution.x, c->init.outputResolution.y);
    erro(c, t);
    return r == FFX_API_RETURN_OK ? XESS_RESULT_ERROR_UNKNOWN : de_ffx(r);
  }
  c->ffx_visto_pelo_gancho = com_gancho;
  c->reset_pendente = true;
  ffxQueryGetProviderVersion pv{};
  pv.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
  const ffxReturnCode_t rq = g_rt.query(&c->ffx, &pv.header);
  logf("ffxCreateContext ok: contexto %p, flags FFX 0x%X, max %ux%u, provedor %s (id 0x%llX)%s%s", c->ffx,
       c->flags_ffx, c->init.outputResolution.x, c->init.outputResolution.y,
       rq == FFX_API_RETURN_OK && pv.versionName ? pv.versionName : "?",
       static_cast<unsigned long long>(pv.versionId), usar_versao ? ", descritor de versao" : "",
       com_gancho ? ", com o gancho do proxy" : ", ANTES do gancho do proxy");
  return XESS_RESULT_SUCCESS;
}

const char* nome_qualidade(int q) {
  switch (q) {
    case 100: return "UltraPerformance";
    case 101: return "Performance";
    case 102: return "Balanced";
    case 103: return "Quality";
    case 104: return "UltraQuality";
    case 105: return "UltraQualityPlus";
    case 106: return "NativeAA";
    default: return "?";
  }
}

std::string nome_flags(uint32_t f) {
  static const char* n[] = {"HIGH_RES_MV", "INVERTED_DEPTH", "EXPOSURE_SCALE_TEXTURE", "RESPONSIVE_PIXEL_MASK",
                            "USE_NDC_VELOCITY", "EXTERNAL_DESCRIPTOR_HEAP", "LDR_INPUT_COLOR", "JITTERED_MV",
                            "ENABLE_AUTOEXPOSURE"};
  std::string s;
  for (int i = 0; i < 9; ++i)
    if (f & (1u << i)) { if (!s.empty()) s += '|'; s += n[i]; }
  if (f >> 9) s += "|outros";
  return s.empty() ? "nenhuma" : s;
}

const xfg::Ffx* ffx_para_fg() {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  static xfg::Ffx f;
  static bool tentou = false, ok = false;
  if (tentou) return ok ? &f : nullptr;
  if (xf::ini().fg_direto) {
    tentou = true;
    const wchar_t* nome = L"amd_fidelityfx_framegeneration_dx12.dll";
    HMODULE m = GetModuleHandleW(nome);
    const std::wstring caminho = xf::pasta_da_dll() + L"\\" + nome;
    if (!m) m = LoadLibraryExW(caminho.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (m) {
      f.create = reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(m, "ffxCreateContext"));
      f.destroy = reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(m, "ffxDestroyContext"));
      f.configure = reinterpret_cast<PfnFfxConfigure>(GetProcAddress(m, "ffxConfigure"));
      f.query = reinterpret_cast<PfnFfxQuery>(GetProcAddress(m, "ffxQuery"));
      f.dispatch = reinterpret_cast<PfnFfxDispatch>(GetProcAddress(m, "ffxDispatch"));
      ok = f.create && f.destroy && f.configure && f.query && f.dispatch;
    }
    logf("FG: FgDireto=1, provedor de FG direto (%s): %s", estreito(caminho).c_str(),
         ok ? "exports ok (o loader e o proxy nao veem o FG)" : "FALHOU (sem a DLL ou sem os exports): sem FG");
    return ok ? &f : nullptr;
  }
  if (!carregar_ffx()) return nullptr;
  tentou = ok = true;
  f.create = g_rt.create;
  f.destroy = g_rt.destroy;
  f.configure = g_rt.configure;
  f.query = g_rt.query;
  f.dispatch = g_rt.dispatch;
  logf("FG: FgDireto=0, FG pelo loader %s (o proxy ve o FG)", g_rt.nome.c_str());
  return &f;
}

void log_ini_uma_vez() {
  static bool feito = false;
  if (feito) return;
  feito = true;
  const auto& i = xf::ini();
  char exe[MAX_PATH]{};
  GetModuleFileNameA(nullptr, exe, MAX_PATH);
  logf("libxess (XeSS -> FSR, pesquisa) no processo %lu, exe %s", GetCurrentProcessId(), exe);
  logf("INI: Perto %g Longe %g FovGraus %g JitterSinal %g/%g AutoExposicao %d ProfundidadeInfinita %d "
       "UsarTexturaExposicao %d UsarMascaraResponsiva %d RecriarAoGanchar %d DescritorVersao %d Nitidez %g (RCAS %g: filtros de "
       "nitidez do helper %d, NitidezComFiltros %d) "
       "VersaoXess %d.%d.%d Provedor '%s' FfxDll '%s'",
       i.perto, i.longe, i.fov_graus, i.jitter_sinal_x, i.jitter_sinal_y, i.auto_exposicao,
       i.profundidade_infinita, i.usar_textura_exposicao, i.usar_mascara_responsiva, i.recriar_ao_ganchar,
       i.descritor_versao, i.nitidez, double(xf::nitidez_rcas(i.nitidez, i.filtros_nitidez_helper, i.nitidez_com_filtros)),
       i.filtros_nitidez_helper ? 1 : 0, i.nitidez_com_filtros, i.versao[0], i.versao[1], i.versao[2], i.provedor.c_str(),
       estreito(i.ffx_dll).c_str());
  if (i.gravar_estencil)
    logf("INI: GravarEstencil %d Tecla %d Execucao %d Quadros %d Intervalo %d Cor %d Estado 0x%X (pasta nr_estencil)",
         i.gravar_estencil, i.gravar_estencil_tecla, i.gravar_estencil_execucao, i.gravar_estencil_quadros,
         i.gravar_estencil_intervalo, i.gravar_estencil_cor, static_cast<unsigned>(i.gravar_estencil_estado));
  if (i.publicar_estencil)
    logf("INI: PublicarEstencil %d (stencil todo quadro para o helper em Local\\NR_ESTENCIL_%lu, estado do depth 0x%X)",
         i.publicar_estencil, GetCurrentProcessId(), static_cast<unsigned>(i.gravar_estencil_estado));
  if (i.geracao_quadros >= 0) {
    logf("INI: GeracaoQuadros %d FgProvedor '%s' FgPulaRepetido %d FgAssincrono %d FgDebug 0x%X FgLogPresents %d FgPrepareAntes %d FgDireto %d FgTesteLocal %d FgOrdem %d FgOrdemMs %d",
         i.geracao_quadros, i.fg_provedor.c_str(), i.fg_pula_repetido, i.fg_assincrono, static_cast<unsigned>(i.fg_debug),
         i.fg_log_presents, i.fg_prepare_antes, i.fg_direto, i.fg_teste_local, i.fg_ordem, i.fg_ordem_ms);
    xfg::instalar(&ffx_para_fg);
  }
}

}

extern "C" {

xess_result_t xessGetVersion(xess_version_t* pVersion) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  log_ini_uma_vez();
  if (!pVersion) return XESS_RESULT_ERROR_INVALID_ARGUMENT;
  const auto& i = xf::ini();
  pVersion->major = static_cast<uint16_t>(i.versao[0]);
  pVersion->minor = static_cast<uint16_t>(i.versao[1]);
  pVersion->patch = static_cast<uint16_t>(i.versao[2]);
  pVersion->reserved = 0;
  logf("xessGetVersion -> %d.%d.%d", i.versao[0], i.versao[1], i.versao[2]);
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessGetIntelXeFXVersion(xess_context_handle_t hContext, xess_version_t* pVersion) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  if (!achar(hContext)) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  if (!pVersion) return XESS_RESULT_ERROR_INVALID_ARGUMENT;
  *pVersion = {};
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessGetProperties(xess_context_handle_t hContext, const xess_2d_t* pOutputResolution,
                                xess_properties_t* pBindingProperties) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  if (!achar(hContext)) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  if (!pOutputResolution || !pBindingProperties) return XESS_RESULT_ERROR_INVALID_ARGUMENT;

  pBindingProperties->requiredDescriptorCount = 32;
  pBindingProperties->tempBufferHeapSize = 65536;
  pBindingProperties->tempTextureHeapSize = 65536;
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessGetInputResolution(xess_context_handle_t hContext, const xess_2d_t* pOutputResolution,
                                     xess_quality_settings_t qualitySettings, xess_2d_t* pInputResolution) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  if (!achar(hContext)) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  if (!pOutputResolution || !pInputResolution) return XESS_RESULT_ERROR_INVALID_ARGUMENT;
  uint32_t w = 0, h = 0;
  if (!xf::resolucao_entrada(pOutputResolution->x, pOutputResolution->y, qualitySettings, w, h)) {
    logf("ERRO xessGetInputResolution: preset %d desconhecido ou saida %ux%u", static_cast<int>(qualitySettings),
         pOutputResolution->x, pOutputResolution->y);
    return XESS_RESULT_ERROR_INVALID_ARGUMENT;
  }
  pInputResolution->x = w;
  pInputResolution->y = h;
  logf("xessGetInputResolution %ux%u %s -> %ux%u", pOutputResolution->x, pOutputResolution->y,
       nome_qualidade(qualitySettings), w, h);
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessGetOptimalInputResolution(xess_context_handle_t hContext, const xess_2d_t* pOutputResolution,
                                            xess_quality_settings_t qualitySettings,
                                            xess_2d_t* pInputResolutionOptimal, xess_2d_t* pInputResolutionMin,
                                            xess_2d_t* pInputResolutionMax) {
  xess_2d_t r{};
  const xess_result_t e = xessGetInputResolution(hContext, pOutputResolution, qualitySettings, &r);
  if (e != XESS_RESULT_SUCCESS) return e;
  if (pInputResolutionOptimal) *pInputResolutionOptimal = r;

  if (pInputResolutionMin) *pInputResolutionMin = r;
  if (pInputResolutionMax) *pInputResolutionMax = r;
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessGetJitterScale(xess_context_handle_t hContext, float* pX, float* pY) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  Contexto* c = achar(hContext);
  if (!c) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  if (!pX || !pY) return XESS_RESULT_ERROR_INVALID_ARGUMENT;
  *pX = c->jitter_scale.x;
  *pY = c->jitter_scale.y;
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessGetVelocityScale(xess_context_handle_t hContext, float* pX, float* pY) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  Contexto* c = achar(hContext);
  if (!c) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  if (!pX || !pY) return XESS_RESULT_ERROR_INVALID_ARGUMENT;
  *pX = c->velocity_scale.x;
  *pY = c->velocity_scale.y;
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessSetJitterScale(xess_context_handle_t hContext, float x, float y) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  Contexto* c = achar(hContext);
  if (!c) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  c->jitter_scale = {x, y};
  logf("xessSetJitterScale %g %g", x, y);
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessSetVelocityScale(xess_context_handle_t hContext, float x, float y) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  Contexto* c = achar(hContext);
  if (!c) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  c->velocity_scale = {x, y};
  logf("xessSetVelocityScale %g %g", x, y);
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessSetExposureMultiplier(xess_context_handle_t hContext, float scale) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  Contexto* c = achar(hContext);
  if (!c) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  c->exposicao_mult = scale;
  logf("xessSetExposureMultiplier %g", scale);
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessGetExposureMultiplier(xess_context_handle_t hContext, float* pScale) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  Contexto* c = achar(hContext);
  if (!c) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  if (!pScale) return XESS_RESULT_ERROR_INVALID_ARGUMENT;
  *pScale = c->exposicao_mult;
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessSetMaxResponsiveMaskValue(xess_context_handle_t hContext, float value) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  Contexto* c = achar(hContext);
  if (!c) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  c->max_responsiva = value;
  logf("xessSetMaxResponsiveMaskValue %g (sem equivalente no FFX)", value);
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessGetMaxResponsiveMaskValue(xess_context_handle_t hContext, float* pValue) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  Contexto* c = achar(hContext);
  if (!c) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  if (!pValue) return XESS_RESULT_ERROR_INVALID_ARGUMENT;
  *pValue = c->max_responsiva;
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessSetLoggingCallback(xess_context_handle_t hContext, xess_logging_level_t loggingLevel,
                                     xess_app_log_callback_t loggingCallback) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  Contexto* c = achar(hContext);
  if (!c) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  c->callback = loggingCallback;
  c->nivel_callback = loggingLevel;
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessIsOptimalDriver(xess_context_handle_t hContext) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  return achar(hContext) ? XESS_RESULT_SUCCESS : XESS_RESULT_ERROR_INVALID_CONTEXT;
}

xess_result_t xessForceLegacyScaleFactors(xess_context_handle_t hContext, bool force) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  if (!achar(hContext)) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  logf("xessForceLegacyScaleFactors %d (a tabela ja e a do 1.0-1.2)", force ? 1 : 0);
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessGetPipelineBuildStatus(xess_context_handle_t hContext) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  return achar(hContext) ? XESS_RESULT_SUCCESS : XESS_RESULT_ERROR_INVALID_CONTEXT;
}

xess_result_t xessSetContextParameterF(xess_context_handle_t hContext, uint32_t parametro, float valor) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  if (!achar(hContext)) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  static unsigned chamadas = 0;
  if (chamadas++ < 16) logf("xessSetContextParameterF parametro %u valor %g (sem equivalente no FFX)", parametro, valor);
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessSelectNetworkModel(xess_context_handle_t hContext, xess_network_model_t network) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  if (!achar(hContext)) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  logf("xessSelectNetworkModel %d (ignorado)", static_cast<int>(network));
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessStartDump(xess_context_handle_t hContext, const xess_dump_parameters_t* dump_parameters) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  if (!achar(hContext)) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  (void)dump_parameters;
  logf("xessStartDump (ignorado)");
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessD3D12GetResourcesToDump(xess_context_handle_t hContext, xess_resources_to_dump_t** pResourcesToDump) {
  static xess_resources_to_dump_t vazio{};
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  if (!achar(hContext)) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  if (pResourcesToDump) *pResourcesToDump = &vazio;
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessD3D12GetProfilingData(xess_context_handle_t hContext, xess_profiling_data_t** pProfilingData) {
  static xess_profiling_data_t vazio{};
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  if (!achar(hContext)) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  if (pProfilingData) *pProfilingData = &vazio;
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessD3D12CreateContext(ID3D12Device* pDevice, xess_context_handle_t* phContext) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  log_ini_uma_vez();
  if (!pDevice || !phContext) {
    logf("ERRO xessD3D12CreateContext: device %p, saida %p", static_cast<void*>(pDevice), static_cast<void*>(phContext));
    return XESS_RESULT_ERROR_INVALID_ARGUMENT;
  }
  auto* c = new Contexto();
  pDevice->AddRef();
  c->device = pDevice;
  g_contextos.insert(c);
  *phContext = reinterpret_cast<xess_context_handle_t>(c);
  logf("xessD3D12CreateContext: device %p -> contexto %p", static_cast<void*>(pDevice), static_cast<void*>(c));

  if (!carregar_ffx()) {
    g_contextos.erase(c);
    c->device->Release();
    delete c;
    *phContext = nullptr;
    return XESS_RESULT_ERROR_CANT_LOAD_LIBRARY;
  }
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessD3D12BuildPipelines(xess_context_handle_t hContext, ID3D12PipelineLibrary* pPipelineLibrary,
                                      bool blocking, uint32_t initFlags) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  if (!achar(hContext)) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  logf("xessD3D12BuildPipelines (sem efeito): biblioteca %p, blocking %d, flags 0x%X", static_cast<void*>(pPipelineLibrary),
       blocking ? 1 : 0, initFlags);
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessD3D12Init(xess_context_handle_t hContext, const xess_d3d12_init_params_t* pInitParams) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  Contexto* c = achar(hContext);
  if (!c) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  if (!pInitParams || pInitParams->outputResolution.x == 0 || pInitParams->outputResolution.y == 0) {
    erro(c, "xessD3D12Init: parametros nulos ou saida 0");
    return XESS_RESULT_ERROR_INVALID_ARGUMENT;
  }
  const float razao = xf::razao_preset(pInitParams->qualitySetting);
  if (razao <= 0.0f) {
    char t2[96];
    std::snprintf(t2, sizeof t2, "xessD3D12Init: preset %d desconhecido", static_cast<int>(pInitParams->qualitySetting));
    erro(c, t2);
    return XESS_RESULT_ERROR_INVALID_ARGUMENT;
  }
  aposentar(c);
  c->init = *pInitParams;
  xf::Ajustes a;
  a.auto_exposicao = xf::ini().auto_exposicao;
  a.profundidade_infinita = xf::ini().profundidade_infinita;
  c->flags_ffx = xf::flags_ffx(pInitParams->initFlags, a);
  uint32_t ew = 0, eh = 0;
  xf::resolucao_entrada(pInitParams->outputResolution.x, pInitParams->outputResolution.y, pInitParams->qualitySetting, ew, eh);
  logf("xessD3D12Init: saida %ux%u, preset %s (razao %.2f -> entrada %ux%u), flags XeSS 0x%X [%s] -> flags FFX 0x%X, "
       "heaps %p/%p, biblioteca %p",
       pInitParams->outputResolution.x, pInitParams->outputResolution.y, nome_qualidade(pInitParams->qualitySetting),
       razao, ew, eh, pInitParams->initFlags, nome_flags(pInitParams->initFlags).c_str(), c->flags_ffx,
       static_cast<void*>(pInitParams->pTempBufferHeap), static_cast<void*>(pInitParams->pTempTextureHeap),
       static_cast<void*>(pInitParams->pPipelineLibrary));
  c->iniciado = true;
  const xess_result_t r = criar_ffx(c);
  if (r != XESS_RESULT_SUCCESS) c->iniciado = false;
  return r;
}

xess_result_t xessD3D12GetInitParams(xess_context_handle_t hContext, xess_d3d12_init_params_t* pInitParams) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  Contexto* c = achar(hContext);
  if (!c) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  if (!pInitParams) return XESS_RESULT_ERROR_INVALID_ARGUMENT;
  if (!c->iniciado) return XESS_RESULT_ERROR_UNINITIALIZED;
  *pInitParams = c->init;
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessD3D12Execute(xess_context_handle_t hContext, ID3D12GraphicsCommandList* pCommandList,
                               const xess_d3d12_execute_params_t* p) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  Contexto* c = achar(hContext);
  if (!c) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  if (!c->iniciado) { erro(c, "xessD3D12Execute antes do Init"); return XESS_RESULT_ERROR_UNINITIALIZED; }
  if (!pCommandList || !p || !p->pColorTexture || !p->pVelocityTexture || !p->pOutputTexture) {
    erro(c, "xessD3D12Execute: lista, parametros, cor, vetores ou saida nulos");
    return XESS_RESULT_ERROR_INVALID_ARGUMENT;
  }
  const auto& ini = xf::ini();

  if (c->ffx && !c->ffx_visto_pelo_gancho && ini.recriar_ao_ganchar && gancho_chegou()) {
    logf("recriando o contexto FFX %p para o proxy ver a criacao (execucao %llu)", c->ffx,
         static_cast<unsigned long long>(c->execucoes));
    aposentar(c);
  }
  if (!c->ffx) {
    const xess_result_t r = criar_ffx(c);
    if (r != XESS_RESULT_SUCCESS) return r;
  }

  for (size_t i = 0; i < c->aposentados.size();) {
    if (c->execucoes >= c->aposentados[i].ate) {
      logf("contexto FFX velho %p destruido (execucao %llu)", c->aposentados[i].ffx,
           static_cast<unsigned long long>(c->execucoes));
      destruir_ffx(c->aposentados[i].ffx);
      c->aposentados.erase(c->aposentados.begin() + static_cast<std::ptrdiff_t>(i));
    } else {
      ++i;
    }
  }

  const uint32_t sw = c->init.outputResolution.x, sh = c->init.outputResolution.y;
  uint32_t ew = p->inputWidth, eh = p->inputHeight;
  if (ew == 0 || eh == 0) xf::resolucao_entrada(sw, sh, c->init.qualitySetting, ew, eh);
  if (ew > sw) ew = sw;
  if (eh > sh) eh = sh;
  const uint32_t fx = c->init.initFlags;

  if (!c->aviso_subrect && (p->inputColorBase.x || p->inputColorBase.y || p->inputMotionVectorBase.x ||
                            p->inputMotionVectorBase.y || p->inputDepthBase.x || p->inputDepthBase.y ||
                            p->inputResponsiveMaskBase.x || p->inputResponsiveMaskBase.y || p->outputColorBase.x ||
                            p->outputColorBase.y)) {
    c->aviso_subrect = true;
    logf("AVISO: o jogo usa base != (0,0) nas texturas (cor %u,%u vetores %u,%u prof %u,%u saida %u,%u); o FFX nao tem "
         "sub-retangulo: ignorado",
         p->inputColorBase.x, p->inputColorBase.y, p->inputMotionVectorBase.x, p->inputMotionVectorBase.y,
         p->inputDepthBase.x, p->inputDepthBase.y, p->outputColorBase.x, p->outputColorBase.y);
  }
  if (!c->aviso_heap && (fx & XESS_INIT_FLAG_EXTERNAL_DESCRIPTOR_HEAP)) {
    c->aviso_heap = true;
    logf("AVISO: EXTERNAL_DESCRIPTOR_HEAP (heap %p, offset %u): o FFX usa os proprios descritores",
         static_cast<void*>(p->pDescriptorHeap), p->descriptorHeapOffset);
  }

  ffxDispatchDescUpscale d{};
  d.header.type = FFX_API_DISPATCH_DESC_TYPE_UPSCALE;
  d.commandList = pCommandList;

  d.color = recurso(p->pColorTexture, FFX_API_RESOURCE_STATE_COMPUTE_READ);
  d.depth = recurso(p->pDepthTexture, FFX_API_RESOURCE_STATE_COMPUTE_READ);
  d.motionVectors = recurso(p->pVelocityTexture, FFX_API_RESOURCE_STATE_COMPUTE_READ);
  const bool autoexp = (c->flags_ffx & FFX_UPSCALE_ENABLE_AUTO_EXPOSURE) != 0;
  if (p->pExposureScaleTexture && (fx & XESS_INIT_FLAG_EXPOSURE_SCALE_TEXTURE) && !autoexp && ini.usar_textura_exposicao) {
    const auto fmt = static_cast<uint32_t>(p->pExposureScaleTexture->GetDesc().Format);
    if (xf::formato_exposicao_ok(fmt)) {
      d.exposure = recurso(p->pExposureScaleTexture, FFX_API_RESOURCE_STATE_COMPUTE_READ);
    } else if (!c->aviso_exposicao) {
      c->aviso_exposicao = true;
      logf("AVISO: textura de exposicao em formato DXGI %u (nao e float de 1 canal): ignorada (AutoExposicao=1 no INI "
           "liga a exposicao automatica)", fmt);
    }
  }
  if (p->pResponsivePixelMaskTexture && (fx & XESS_INIT_FLAG_RESPONSIVE_PIXEL_MASK) && ini.usar_mascara_responsiva)
    d.reactive = recurso(p->pResponsivePixelMaskTexture, FFX_API_RESOURCE_STATE_COMPUTE_READ);
  d.output = recurso(p->pOutputTexture, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, FFX_API_RESOURCE_USAGE_UAV);

  const xf::Par j = xf::jitter({p->jitterOffsetX, p->jitterOffsetY}, c->jitter_scale, {ini.jitter_sinal_x, ini.jitter_sinal_y});
  d.jitterOffset = {j.x, j.y};
  const xf::Par mv = xf::escala_vetores(fx, c->velocity_scale, ew, eh, sw, sh);
  d.motionVectorScale = {mv.x, mv.y};
  d.renderSize = {ew, eh};
  d.upscaleSize = {sw, sh};
  {
    const float nz = xf::nitidez_rcas(ini.nitidez, ini.filtros_nitidez_helper, ini.nitidez_com_filtros);
    d.enableSharpening = nz > 0.0f;
    d.sharpness = nz;
  }
  LARGE_INTEGER agora{}, freq{};
  QueryPerformanceCounter(&agora);
  QueryPerformanceFrequency(&freq);
  d.frameTimeDelta = xf::delta_ms(agora.QuadPart, c->qpc_anterior, freq.QuadPart);
  c->qpc_anterior = agora.QuadPart;
  d.preExposure = xf::pre_exposicao(p->exposureScale, c->exposicao_mult);
  d.reset = p->resetHistory != 0 || c->reset_pendente;
  const xf::Camera cam = xf::sanear_camera(ini.perto, ini.longe, ini.fov_graus);
  d.cameraNear = cam.perto;
  d.cameraFar = cam.longe;
  d.cameraFovAngleVertical = cam.fov_vertical_rad;
  d.viewSpaceToMetersFactor = 1.0f;
  d.flags = 0;

  if (!c->formato_logado) {
    c->formato_logado = true;
    const auto fp = p->pDepthTexture ? static_cast<uint32_t>(p->pDepthTexture->GetDesc().Format) : 0u;
    logf("ENTRADAS_FORMATO profundidade dxgi %u (%s), cor dxgi %u, vetores dxgi %u; mascara responsiva: flag %s, textura %p "
         "(dxgi %u)",
         fp, xf::formato_com_estencil(fp) ? "COM stencil no plano 1" : "sem stencil",
         static_cast<uint32_t>(p->pColorTexture->GetDesc().Format), static_cast<uint32_t>(p->pVelocityTexture->GetDesc().Format),
         (fx & XESS_INIT_FLAG_RESPONSIVE_PIXEL_MASK) ? "sim" : "nao", static_cast<void*>(p->pResponsivePixelMaskTexture),
         p->pResponsivePixelMaskTexture ? static_cast<uint32_t>(p->pResponsivePixelMaskTexture->GetDesc().Format) : 0u);
  }
  estencil_por_execucao(c, pCommandList, p);
  estencil_publicar(c, pCommandList, p);

  auto preparar_fg = [&] {
    xfg::Quadro fq;
    fq.device = c->device;
    fq.lista = pCommandList;
    fq.profundidade = p->pDepthTexture;
    fq.vetores = p->pVelocityTexture;
    fq.ew = ew;
    fq.eh = eh;
    fq.sw = sw;
    fq.sh = sh;
    fq.flags_upscale = c->flags_ffx;
    fq.jx = j.x;
    fq.jy = j.y;
    fq.mvx = mv.x;
    fq.mvy = mv.y;
    fq.delta_ms = d.frameTimeDelta;
    fq.perto = cam.perto;
    fq.longe = cam.longe;
    fq.fov_rad = cam.fov_vertical_rad;
    fq.reset = d.reset;
    xfg::por_execucao(fq);
  };
  if (ini.geracao_quadros >= 0 && ini.fg_prepare_antes) preparar_fg();

  const ffxReturnCode_t r = g_rt.dispatch(&c->ffx, &d.header);
  ++c->execucoes;
  c->reset_pendente = false;
  if (c->execucoes <= static_cast<uint64_t>(ini.log_execucoes < 0 ? 0 : ini.log_execucoes) || r != FFX_API_RETURN_OK) {
    logf("execucao %llu: ffxDispatch %u; entrada %ux%u -> %ux%u; cor fmt %u, prof %p fmt %u, vetores fmt %u, saida fmt %u, "
         "exposicao %p, reactive %p; jitter %.4f %.4f (XeSS %.4f %.4f), escala vetores %g %g, preExposure %g, delta %.2f ms, "
         "reset %d, camera %g/%g/%.3f rad",
         static_cast<unsigned long long>(c->execucoes), r, ew, eh, sw, sh, d.color.description.format,
         static_cast<void*>(p->pDepthTexture), d.depth.description.format, d.motionVectors.description.format,
         d.output.description.format, d.exposure.resource, d.reactive.resource, j.x, j.y, p->jitterOffsetX,
         p->jitterOffsetY, mv.x, mv.y, d.preExposure, d.frameTimeDelta, d.reset ? 1 : 0, d.cameraNear, d.cameraFar,
         d.cameraFovAngleVertical);
  }

  c->janela.adicionar(p->jitterOffsetX, p->jitterOffsetY, d.reset, ew, eh, agora.QuadPart);
  if (c->janela.n >= kExecPorLinha) {
    const auto& w = c->janela;
    logf("execucoes %llu-%llu: %.1f por segundo; jitter XeSS x %.4f..%.4f y %.4f..%.4f (%u de %llu diferentes de 0); entrada "
         "%ux%u (%u trocas); reset %u; escala jitter %g %g; preExposure %g",
         static_cast<unsigned long long>(c->execucoes - w.n + 1), static_cast<unsigned long long>(c->execucoes),
         w.por_segundo(freq.QuadPart), w.jx_min, w.jx_max, w.jy_min, w.jy_max, w.nao_zero, static_cast<unsigned long long>(w.n),
         w.w, w.h, w.trocas_entrada, w.resets, c->jitter_scale.x, c->jitter_scale.y, d.preExposure);
    c->janela.zerar();
  }
  if (r == FFX_API_RETURN_OK && ini.geracao_quadros >= 0 && !ini.fg_prepare_antes) preparar_fg();
  if (r != FFX_API_RETURN_OK) {
    char t2[96];
    std::snprintf(t2, sizeof t2, "ffxDispatch devolveu %u na execucao %llu", r, static_cast<unsigned long long>(c->execucoes));
    erro(c, t2);
    return de_ffx(r);
  }
  return XESS_RESULT_SUCCESS;
}

xess_result_t xessDestroyContext(xess_context_handle_t hContext) {
  std::lock_guard<std::recursive_mutex> t(g_mutex);
  Contexto* c = achar(hContext);
  if (!c) return XESS_RESULT_ERROR_INVALID_CONTEXT;
  logf("xessDestroyContext %p (%llu execucoes, %llu erros)", static_cast<void*>(c),
       static_cast<unsigned long long>(c->execucoes), static_cast<unsigned long long>(c->erros));
  destruir_tudo(c);
  g_contextos.erase(c);
  if (g_contextos.empty()) xfg::soltar_contexto();
  c->magia = 0;
  if (c->device) c->device->Release();
  delete c;
  return XESS_RESULT_SUCCESS;
}

}

BOOL WINAPI DllMain(HINSTANCE modulo, DWORD motivo, LPVOID) {
  if (motivo == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(modulo);
    xf::log_iniciar(modulo);
  }
  return TRUE;
}
