#pragma once
#include "../../../runtime/nr_publico.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <mutex>
namespace nrpresenttrace {
struct State {
    std::mutex mutex;
    void* present_seen[32]{};void* queue_seen[32]{};void* overlay_seen[32]{};
    std::uint64_t types_seen[16]{};
    unsigned lines{};
    std::atomic<std::uint64_t> presents{0},queues{0},overlays{0};
};
inline State& state(){static auto* s=new State;return *s;}
template<class... A> inline void line(const char*,A&&...) noexcept {}

inline bool first(void* (&seen)[32],void* p) noexcept {
    auto& s=state();std::lock_guard<std::mutex> lock(s.mutex);
    for(auto*& slot:seen){if(slot==p)return false;if(!slot){slot=p;return true;}}
    return false;
}
inline void pulse(std::uint64_t n,void* swapchain) noexcept {
    if(n%3000)return;
    auto& s=state();
    line("pulse presents=%llu queues=%llu overlays=%llu swapchain=%p",static_cast<unsigned long long>(n),
         static_cast<unsigned long long>(s.queues.load()),static_cast<unsigned long long>(s.overlays.load()),swapchain);
}
inline void ffx_type(std::uint64_t type) noexcept {
    auto& s=state();
    {std::lock_guard<std::mutex> lock(s.mutex);
     for(auto& slot:s.types_seen){if(slot==type)return;if(!slot){slot=type;break;}}}
    line("ffx_dispatch_first type=0x%llx",static_cast<unsigned long long>(type));
}
}
