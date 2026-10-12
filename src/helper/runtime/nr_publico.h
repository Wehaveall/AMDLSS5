#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstddef>
#include <cstdio>

namespace nrpub {

template<class... A> inline int sem_log(char* b,std::size_t n,A&&...) noexcept {if(b&&n)b[0]=0;return 0;}
}

namespace nrcfg {
UINT ler_int(LPCWSTR secao,LPCWSTR chave,INT padrao,LPCWSTR arquivo) noexcept;
DWORD ler_texto(LPCWSTR secao,LPCWSTR chave,LPCWSTR padrao,LPWSTR saida,DWORD n,LPCWSTR arquivo) noexcept;
BOOL escrever(LPCWSTR secao,LPCWSTR chave,LPCWSTR valor,LPCWSTR arquivo) noexcept;

void definir_ini(const wchar_t* caminho) noexcept;

unsigned completar_ini_proxy(const wchar_t* ini,const wchar_t* jogo=nullptr) noexcept;
}

#define NRCFG_INT ::nrcfg::ler_int
#define NRCFG_STR ::nrcfg::ler_texto
#define NRCFG_ESCREVER ::nrcfg::escrever
#define NRD(s) ""
#define NR_LOG_ATIVO 0
#define NRDIAG(...)
#define NRLOGF ::nrpub::sem_log
