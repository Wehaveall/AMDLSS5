#pragma once

#include "scale_policy.h"
#include <array>

namespace nrscale {
enum class Step {OriginalFsr,Downsample,Neural,Reconstruct};
struct Order {std::array<Step,4> steps{};std::uint32_t count{};};
Order order(std::uint32_t stage,std::uint32_t percent) noexcept;
}
