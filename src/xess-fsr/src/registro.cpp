#include "registro.h"
#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <cwchar>
#include <mutex>

namespace xf {
namespace {
std::mutex g_mutex;
std::wstring g_pasta;
FILE* g_arquivo = nullptr;
bool g_falhou = false;
unsigned g_linhas = 0;
constexpr unsigned kMaxLinhas = 50000;

float ler_float(const wchar_t* chave, float padrao, const std::wstring& caminho) {
  wchar_t buf[64]{};
  GetPrivateProfileStringW(L"XessFsr", chave, L"", buf, 64, caminho.c_str());
  if (!buf[0]) return padrao;
  wchar_t* fim = nullptr;
  const float v = std::wcstof(buf, &fim);
  return fim != buf ? v : padrao;
}
int ler_int(const wchar_t* chave, int padrao, const std::wstring& caminho) {
  return static_cast<int>(GetPrivateProfileIntW(L"XessFsr", chave, padrao, caminho.c_str()));
}
}

void log_iniciar(void* modulo) {
  wchar_t caminho[MAX_PATH * 2]{};
  const DWORD n = GetModuleFileNameW(static_cast<HMODULE>(modulo), caminho, MAX_PATH * 2);
  std::wstring s(caminho, n);
  const auto barra = s.find_last_of(L"\\/");
  g_pasta = barra == std::wstring::npos ? L"." : s.substr(0, barra);
}

std::wstring pasta_da_dll() { return g_pasta; }

void logf(const char*, ...) {}

const Ini& ini() {
  static const Ini lido = [] {
    Ini i;
    const std::wstring c = pasta_da_dll() + L"\\nr_xess_fsr.ini";
    if (GetFileAttributesW(c.c_str()) == INVALID_FILE_ATTRIBUTES) return i;
    i.perto = ler_float(L"Perto", i.perto, c);
    i.longe = ler_float(L"Longe", i.longe, c);
    i.fov_graus = ler_float(L"FovGraus", i.fov_graus, c);
    i.jitter_sinal_x = ler_float(L"JitterSinalX", i.jitter_sinal_x, c) < 0.0f ? -1.0f : 1.0f;
    i.jitter_sinal_y = ler_float(L"JitterSinalY", i.jitter_sinal_y, c) < 0.0f ? -1.0f : 1.0f;
    i.auto_exposicao = ler_int(L"AutoExposicao", i.auto_exposicao, c);
    i.profundidade_infinita = ler_int(L"ProfundidadeInfinita", i.profundidade_infinita, c);
    i.usar_textura_exposicao = ler_int(L"UsarTexturaExposicao", i.usar_textura_exposicao, c);
    i.usar_mascara_responsiva = ler_int(L"UsarMascaraResponsiva", i.usar_mascara_responsiva, c);
    i.recriar_ao_ganchar = ler_int(L"RecriarAoGanchar", i.recriar_ao_ganchar, c);
    i.descritor_versao = ler_int(L"DescritorVersao", i.descritor_versao, c);
    i.log_execucoes = ler_int(L"LogExecucoes", i.log_execucoes, c);
    i.gravar_estencil = ler_int(L"GravarEstencil", i.gravar_estencil, c);
    i.gravar_estencil_tecla = ler_int(L"GravarEstencilTecla", i.gravar_estencil_tecla, c);
    i.gravar_estencil_execucao = ler_int(L"GravarEstencilExecucao", i.gravar_estencil_execucao, c);
    i.gravar_estencil_quadros = ler_int(L"GravarEstencilQuadros", i.gravar_estencil_quadros, c);
    if (i.gravar_estencil_quadros > 16) i.gravar_estencil_quadros = 16;
    i.gravar_estencil_intervalo = ler_int(L"GravarEstencilIntervalo", i.gravar_estencil_intervalo, c);
    i.gravar_estencil_cor = ler_int(L"GravarEstencilCor", i.gravar_estencil_cor, c);
    i.gravar_estencil_estado = ler_int(L"GravarEstencilEstado", i.gravar_estencil_estado, c);
    i.publicar_estencil = ler_int(L"PublicarEstencil", i.publicar_estencil, c);
    i.nitidez = ler_float(L"Nitidez", i.nitidez, c);
    i.nitidez_com_filtros = ler_int(L"NitidezComFiltros", i.nitidez_com_filtros, c);
    {
      const std::wstring h = pasta_da_dll() + L"\\dlssnr_on_amd.ini";
      i.filtros_nitidez_helper = GetPrivateProfileIntW(L"DlssNrOnAmd", L"NeuralFiltroCas", 0, h.c_str()) == 1 ||
                                 GetPrivateProfileIntW(L"DlssNrOnAmd", L"NeuralFiltroLuma", 0, h.c_str()) == 1;
    }
    wchar_t buf[MAX_PATH]{};
    GetPrivateProfileStringW(L"XessFsr", L"VersaoXess", L"", buf, MAX_PATH, c.c_str());
    if (buf[0]) {
      int a = 0, b = 0, d = 0;
      if (swscanf_s(buf, L"%d.%d.%d", &a, &b, &d) >= 2) { i.versao[0] = a; i.versao[1] = b; i.versao[2] = d; }
    }
    buf[0] = 0;
    GetPrivateProfileStringW(L"XessFsr", L"FfxDll", L"", buf, MAX_PATH, c.c_str());
    i.ffx_dll = buf;
    wchar_t prov[64]{};
    GetPrivateProfileStringW(L"XessFsr", L"Provedor", L"", prov, 64, c.c_str());
    for (const wchar_t* q = prov; *q; ++q) i.provedor.push_back(*q < 128 ? static_cast<char>(*q) : '?');
    i.geracao_quadros = ler_int(L"GeracaoQuadros", i.geracao_quadros, c);
    i.fg_pula_repetido = ler_int(L"FgPulaRepetido", i.fg_pula_repetido, c);
    i.fg_assincrono = ler_int(L"FgAssincrono", i.fg_assincrono, c);
    i.fg_debug = ler_int(L"FgDebug", i.fg_debug, c);
    i.fg_log_presents = ler_int(L"FgLogPresents", i.fg_log_presents, c);
    i.fg_prepare_antes = ler_int(L"FgPrepareAntes", i.fg_prepare_antes, c);
    i.fg_direto = ler_int(L"FgDireto", i.fg_direto, c);
    i.fg_teste_local = ler_int(L"FgTesteLocal", i.fg_teste_local, c);
    i.fg_ordem = ler_int(L"FgOrdem", i.fg_ordem, c);
    i.fg_ordem_ms = ler_int(L"FgOrdemMs", i.fg_ordem_ms, c);
    prov[0] = 0;
    GetPrivateProfileStringW(L"XessFsr", L"FgProvedor", L"", prov, 64, c.c_str());
    for (const wchar_t* q = prov; *q; ++q) i.fg_provedor.push_back(*q < 128 ? static_cast<char>(*q) : '?');
    return i;
  }();
  return lido;
}

}
