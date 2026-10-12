#include "dispatch_flow.h"

namespace nrscale {
Order order(std::uint32_t stage,std::uint32_t percent) noexcept{
  if((stage!=1&&stage!=2)||!supported_percent(percent))return {};
  if(stage==1){
    if(percent==100)return {{{Step::Neural,Step::OriginalFsr}},2};
    return {{{Step::Downsample,Step::Neural,Step::Reconstruct,Step::OriginalFsr}},4};
  }
  if(percent==100)return {{{Step::OriginalFsr,Step::Neural}},2};
  return {{{Step::OriginalFsr,Step::Downsample,Step::Neural,Step::Reconstruct}},4};
}
}
