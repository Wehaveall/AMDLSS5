#pragma once
#include "../../../runtime/nr_publico.h"

#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string>
#include "../../../runtime/pasta_helper.h"
#include "../async-reprojection/reproject_controls.h"
namespace nrfgtempo {
struct Ev{std::int64_t qpc;std::int32_t valor;char tipo;};
inline constexpr unsigned teto=20000;
struct Estado{std::atomic<int> n_pedido{-1};std::atomic<unsigned> idx{0};std::atomic<long long> presents_fg{0};std::atomic<bool> gravado{false};Ev ev[teto];};
inline Estado& st(){static Estado e;return e;}
inline void ler(const std::wstring& ini){auto& e=st();int esperado=-1;
 const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralFgTempo",0,ini.c_str()));
 e.n_pedido.compare_exchange_strong(esperado,v<0?0:(v>int(teto)?int(teto):v));}
inline void gravar(){
}
inline void evento(char tipo,int valor) noexcept {
(void)tipo;(void)valor;
}
}
