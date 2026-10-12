#pragma once
#include "../active-scheduling/active_scheduling.h"
#include <array>
#include <cstddef>
#include <cstdint>
namespace nractive {
constexpr std::uint32_t globals_begin=0x93E00,globals_end=0x94D00;
using Globals=std::array<std::uint8_t,globals_end-globals_begin>;
using Config=std::array<std::uint32_t,17>;
bool exclusive_signatures(const void* mapped_image,std::size_t size) noexcept;
bool first_use_empty(const Globals& globals) noexcept;
Config configuration(const Globals& globals) noexcept;
bool cache_matches(const Globals& globals,std::uint32_t cache,const NrResourceDesc* resource,bool require_copy=true) noexcept;

bool publish_capture_bookkeeping(void* mapped_image,std::size_t size) noexcept;
}
