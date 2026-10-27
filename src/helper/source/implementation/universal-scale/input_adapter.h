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
  bool output{};

  std::uint32_t output_view{};
  bool exposure{};
  std::uint32_t exposure_view{};
  std::uint32_t color_view{},motion_view{},depth_view{};
  float color_scale{1.f};
  float motion_x{1.f},motion_y{1.f};
  float output_scale{1.f};
  const char* reason{""};
  bool subrect{};

  bool mv_tela{};
  float motion_amostra_x{1.f},motion_amostra_y{1.f};
  unsigned roles()const noexcept{return (color?1u:0u)|(motion?2u:0u)|(depth?4u:0u)|(output?8u:0u)|(exposure?16u:0u);}
};

constexpr unsigned kAdaptColor=1u,kAdaptMotion=2u,kAdaptDepth=4u,kAdaptOutput=8u,kAdaptExposure=16u;
std::uint32_t adapter_exposure_view(std::uint32_t format) noexcept;

std::uint32_t bits_cor_exibicao(std::uint32_t format) noexcept;

std::uint32_t adapter_output_view(std::uint32_t format) noexcept;

AdapterPlan plan_adapter(const nrffx::RawDescriptor& d,const NrResourceDesc& input,const NrResourceDesc& output,
                         const NrResourceDesc& motion,const NrResourceDesc& depth,unsigned sticky=0,bool subrect=false,
                         const NrResourceDesc* exposure=nullptr) noexcept;

bool inputs_larger_than_render(const nrffx::RawDescriptor& d,const NrResourceDesc& input,const NrResourceDesc& motion,const NrResourceDesc& depth) noexcept;
}
