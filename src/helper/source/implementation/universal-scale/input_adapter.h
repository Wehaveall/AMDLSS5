#pragma once

#include "active_scheduling.h"
#include "ffx_descriptor.h"
#include <cstdint>

namespace nrscale {

std::uint32_t adapter_color_view(std::uint32_t format) noexcept;
std::uint32_t adapter_motion_view(std::uint32_t format) noexcept;
std::uint32_t adapter_depth_view(std::uint32_t format) noexcept;
struct AdapterPlan {
  bool adapt{};
  bool color{};
  bool motion{};
  bool depth{};
  std::uint32_t color_view{},motion_view{},depth_view{};
  float color_scale{1.f};
  float motion_x{1.f},motion_y{1.f};
  float output_scale{1.f};
  const char* reason{""};
  unsigned roles()const noexcept{return (color?1u:0u)|(motion?2u:0u)|(depth?4u:0u);}
};

constexpr unsigned kAdaptColor=1u,kAdaptMotion=2u,kAdaptDepth=4u;

AdapterPlan plan_adapter(const nrffx::RawDescriptor& d,const NrResourceDesc& input,const NrResourceDesc& output,
                         const NrResourceDesc& motion,const NrResourceDesc& depth,unsigned sticky=0) noexcept;
}
