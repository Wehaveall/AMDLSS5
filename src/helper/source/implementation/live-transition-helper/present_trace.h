#pragma once
#include "../../../runtime/nr_publico.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include "../../../runtime/log_fila.h"
#include "../../../runtime/pasta_helper.h"
namespace nrpresenttrace {
struct State {
    std::mutex mutex;
    void* present_seen[32]{};void* queue_seen[32]{};void* overlay_seen[32]{};
    std::uint64_t types_seen[16]{};
    unsigned lines{};
    std::atomic<std::uint64_t> presents{0},queues{0},overlays{0};

    std::atomic<std::uint64_t> fsr{0},aninhados{0};
    std::uint64_t fsr_ult{},h_fsr[4]{},h_dxgi[3]{},dxgi_falhou{};unsigned dxgi_ult{};bool dxgi_tem{};
    char padrao[129]{};unsigned padrao_n{};bool padrao_logado{};
};
inline State& state(){static auto* s=new State;return *s;}
template<class... A> inline void line(const char*,A&&...) noexcept {}

inline bool first(void* (&)[32],void*) noexcept {return false;}
inline void pulse(std::uint64_t,void*) noexcept {}
inline void ffx_type(std::uint64_t) noexcept {}
inline void nota_fsr() noexcept {}
inline void depois_present(std::uint64_t,bool,unsigned) noexcept {}
}
