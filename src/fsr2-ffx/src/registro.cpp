#include "registro.h"
#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace f2f {
namespace {
std::wstring g_pasta;
int ler_int(const wchar_t* chave, int padrao, const std::wstring& c) {
  return static_cast<int>(GetPrivateProfileIntW(L"Fsr2Ffx", chave, padrao, c.c_str()));
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
    const std::wstring c = pasta_da_dll() + L"\\nr_fsr2_ffx.ini";
    if (GetFileAttributesW(c.c_str()) == INVALID_FILE_ATTRIBUTES) return i;
    wchar_t prov[64]{};
    GetPrivateProfileStringW(L"Fsr2Ffx", L"Provedor", L"", prov, 64, c.c_str());
    for (const wchar_t* q = prov; *q; ++q) i.provedor.push_back(*q < 128 ? static_cast<char>(*q) : '?');
    i.recriar_ao_ganchar = ler_int(L"RecriarAoGanchar", i.recriar_ao_ganchar, c);
    i.log_despachos = ler_int(L"LogDespachos", i.log_despachos, c);
    i.geracao_quadros = ler_int(L"GeracaoQuadros", i.geracao_quadros, c);
    i.fg_ordem_ms = ler_int(L"FgOrdemMs", i.fg_ordem_ms, c);
    return i;
  }();
  return lido;
}

}
