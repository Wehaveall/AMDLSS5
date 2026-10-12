#pragma once

#include <atomic>
namespace nrreusostate {
inline std::atomic<unsigned> refresco{0};
inline thread_local bool cena=false;
inline void pedir_cheio() noexcept {refresco.fetch_add(1,std::memory_order_relaxed);}

#include <cstdint>
inline std::atomic<std::uint64_t> quadros{1};
inline void contar_quadro() noexcept {quadros.fetch_add(1,std::memory_order_relaxed);}
struct Carimbo {std::atomic<int> job{0};std::atomic<std::uint64_t> quadro{0};};
inline Carimbo carimbos[16];
inline void carimbar(int job) noexcept {
 if(job<=0)return;auto& c=carimbos[unsigned(job)&15u];
 c.job.store(0,std::memory_order_relaxed);c.quadro.store(quadros.load(std::memory_order_relaxed),std::memory_order_relaxed);c.job.store(job,std::memory_order_release);
}
inline std::uint64_t quadro_do_job(int job) noexcept {
 if(job<=0)return 0;auto& c=carimbos[unsigned(job)&15u];
 if(c.job.load(std::memory_order_acquire)!=job)return 0;const auto q=c.quadro.load(std::memory_order_relaxed);
 return c.job.load(std::memory_order_acquire)==job?q:0;
}
inline thread_local std::uint64_t quadro_cena=0;
struct EscopoCena {
 bool antes;
 explicit EscopoCena(bool v) noexcept:antes(cena){cena=v;}
 ~EscopoCena(){cena=antes;}
 EscopoCena(const EscopoCena&)=delete;EscopoCena& operator=(const EscopoCena&)=delete;
};
}
