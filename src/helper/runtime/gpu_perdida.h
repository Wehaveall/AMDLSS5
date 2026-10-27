#pragma once

#include "nr_publico.h"
#include <windows.h>
#include <atomic>
#include <cstdint>
namespace nrgpu {
inline std::atomic<bool> perdida_{false};
inline std::atomic<long> codigo_{0};
inline bool perdida() noexcept {return perdida_.load(std::memory_order_acquire);}
inline long codigo() noexcept {return codigo_.load(std::memory_order_relaxed);}
constexpr bool hip_grudento(int status) noexcept {return status==700||status==702||status==709||status==719;}

inline bool marcar(const char* origem,long c) noexcept {
    if(perdida_.exchange(true,std::memory_order_acq_rel))return false;
    codigo_.store(c,std::memory_order_relaxed);
    (void)origem;
    return true;
}

inline bool hip(int status,const char* origem) noexcept {return hip_grudento(status)&&marcar(origem,status);}
}
