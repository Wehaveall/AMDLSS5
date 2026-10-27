#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>

namespace nrprint {
inline std::atomic<unsigned> pedido{0};
inline std::atomic<unsigned> salvando{0};
inline std::atomic<unsigned> salvos{0};
inline std::atomic<unsigned> ultima_falhou{0};
inline void pedir() noexcept { pedido.store(1,std::memory_order_release); }

struct TelaArmado { std::uint64_t quadro_ffx{}; unsigned indice{}; };
inline constexpr unsigned tela_cap=64;
inline std::mutex tela_mutex;
inline TelaArmado tela_fila[tela_cap]{};
inline unsigned tela_n{0};
inline std::atomic<unsigned> tela_pendentes{0};
inline std::atomic<unsigned> tela_descartados{0},tela_gravados{0},tela_salvando{0},tela_falhas{0};

inline std::atomic<void(*)(bool criar,std::uint64_t bytes)> tela_conta{nullptr};
inline void armar_quadro(std::uint64_t quadro_ffx,unsigned indice) noexcept {
    std::lock_guard<std::mutex> trava(tela_mutex);
    if(tela_n>=tela_cap){tela_descartados.fetch_add(1,std::memory_order_relaxed);return;}
    tela_fila[tela_n++]=TelaArmado{quadro_ffx,indice};tela_pendentes.store(tela_n,std::memory_order_release);
}

inline unsigned tomar_armados(TelaArmado* saida,unsigned max) noexcept {
    std::lock_guard<std::mutex> trava(tela_mutex);
    const unsigned n=tela_n<max?tela_n:max;
    for(unsigned k=0;k<n;++k)saida[k]=tela_fila[k];
    for(unsigned k=n;k<tela_n;++k)tela_fila[k-n]=tela_fila[k];
    tela_n-=n;tela_pendentes.store(tela_n,std::memory_order_release);return n;
}
}
