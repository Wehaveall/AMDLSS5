#pragma once
#include "../../../runtime/nr_publico.h"

#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string>
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
auto& e=st();if(e.gravado.exchange(true))return;
 LARGE_INTEGER f{};QueryPerformanceFrequency(&f);FILE* a{};
 if(fopen_s(&a,"nr_fg_tempo.csv","w")!=0||!a)return;
 std::fprintf(a,"tipo,us,valor\n");const unsigned n=(std::min)(e.idx.load(),teto);
 for(unsigned i=0;i<n;++i)std::fprintf(a,"%c,%.1f,%d\n",e.ev[i].tipo,double(e.ev[i].qpc)*1e6/double(f.QuadPart),e.ev[i].valor);
 std::fclose(a);
}
inline void evento(char tipo,int valor) noexcept {
 auto& e=st();const int n=e.n_pedido.load(std::memory_order_relaxed);if(n<=0||e.gravado.load(std::memory_order_relaxed))return;
 if(!nrreproject::fg_ativo()){e.presents_fg.store(0,std::memory_order_relaxed);return;}
 if(tipo=='P'&&e.presents_fg.fetch_add(1,std::memory_order_relaxed)<600)return;
 if(e.presents_fg.load(std::memory_order_relaxed)<600)return;
 LARGE_INTEGER t{};QueryPerformanceCounter(&t);
 const unsigned i=e.idx.fetch_add(1,std::memory_order_relaxed);
 if(i<unsigned(n)&&i<teto){e.ev[i]={t.QuadPart,valor,tipo};if(i+1==unsigned(n)||i+1==teto)gravar();}
}
}
