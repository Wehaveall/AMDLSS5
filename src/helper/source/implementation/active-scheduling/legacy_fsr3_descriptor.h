#pragma once
#include "ffx_descriptor.h"
namespace nrffx {
constexpr std::size_t legacy_fsr3_descriptor_size=0x518;
bool legacy_fsr3_to_generic(const void*,std::byte*,std::size_t) noexcept;
}
