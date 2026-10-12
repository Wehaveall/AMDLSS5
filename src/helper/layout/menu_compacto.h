#pragma once

#include <atomic>
namespace nrmenucompacto {
inline std::atomic<int> modo{-1};
inline std::atomic<bool> recolhido{false};
inline std::atomic<bool> desenhando{false};
inline std::atomic<bool> ja_aberto{false};
inline thread_local bool pedido=false;
}
