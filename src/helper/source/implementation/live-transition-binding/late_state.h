#pragma once

#include <atomic>
#include <cstdint>
#include <emmintrin.h>
namespace nrlate {
constexpr unsigned flag_slots=16;
constexpr unsigned flag_stride_u64=32;
inline std::atomic<std::uint64_t*> flag_map{nullptr};

inline std::atomic<bool> desligada{false};
inline volatile std::uint64_t* flag_slot(std::uint64_t* map,int job) noexcept{
    return reinterpret_cast<volatile std::uint64_t*>(map)+std::size_t(unsigned(job)%flag_slots)*flag_stride_u64;
}

inline void publish(int job) noexcept{
    auto* map=flag_map.load(std::memory_order_acquire);
    if(!map||job<=0)return;
    *flag_slot(map,job)=std::uint64_t(unsigned(job));
    _mm_sfence();
}
}
