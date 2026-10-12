#pragma once
#include <atomic>

namespace nrprint {
inline std::atomic<unsigned> pedido{0};
inline std::atomic<unsigned> salvando{0};
inline std::atomic<unsigned> salvos{0};
inline std::atomic<unsigned> ultima_falhou{0};
inline void pedir() noexcept { pedido.store(1,std::memory_order_release); }
}
