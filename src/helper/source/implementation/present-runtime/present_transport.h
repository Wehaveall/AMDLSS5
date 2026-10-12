#pragma once
#include <cstdint>
#include <string>

namespace nrpresentgpu {
using Evaluate = bool(*)(void*, const std::uint16_t*, std::uint16_t*, unsigned, unsigned, std::string&);

bool process(void* swapchain, void* queue, Evaluate evaluate, void* context, std::string& diagnostic);
}
