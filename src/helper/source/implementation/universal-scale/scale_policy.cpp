#include "scale_policy.h"

#include <algorithm>

namespace nrscale {
bool supported_percent(std::uint32_t percent) noexcept{
  for(const auto value:{100u,85u,70u,65u,50u,40u,30u})if(value==percent)return true;
  return false;
}
std::optional<Target> target(Geometry original,std::uint32_t percent) noexcept{
  constexpr std::uint32_t maximum=16384;
  if(!supported_percent(percent)||!original.render_w||!original.render_h||!original.output_w||!original.output_h||
     original.render_w>maximum||original.render_h>maximum||original.output_w>maximum||original.output_h>maximum)return {};
  const auto scaled=[percent](std::uint32_t value){
    return std::max<std::uint32_t>(1,static_cast<std::uint32_t>((static_cast<std::uint64_t>(value)*percent+50)/100));
  };
  return Target{original,{scaled(original.render_w),scaled(original.render_h),scaled(original.output_w),scaled(original.output_h)},percent};
}
}
