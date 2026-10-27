#pragma once
#include "ffx_descriptor.h"

namespace nrffx {
namespace fsr2 {

constexpr std::size_t res_resource=0x00,res_description=0x88,res_state=0xA4,res_size=0xB8;

constexpr std::size_t commandList=0x000,color=0x008,depth=0x0C0,motionVectors=0x178,exposure=0x230,reactive=0x2E8,
  transparencyAndComposition=0x3A0,output=0x458,jitterOffset=0x510,motionVectorScale=0x518,renderSize=0x520,enableSharpening=0x528,
  sharpness=0x52C,frameTimeDelta=0x530,preExposure=0x534,reset=0x538,cameraNear=0x53C,cameraFar=0x540,cameraFovAngleVertical=0x544;

constexpr std::size_t dispatch_prefix=0x548;

struct Leiaute{std::size_t res_description,res_state,commandList,color,depth,motionVectors,exposure,reactive,transparencyAndComposition,
  output,jitterOffset,enableSharpening,sharpness,frameTimeDelta,preExposure,reset,cameraNear,cameraFar,cameraFovAngleVertical,
  dispatch_prefix;const char* nome;};
inline constexpr Leiaute v21{res_description,res_state,commandList,color,depth,motionVectors,exposure,reactive,transparencyAndComposition,
  output,jitterOffset,enableSharpening,sharpness,frameTimeDelta,preExposure,reset,cameraNear,cameraFar,cameraFovAngleVertical,
  dispatch_prefix,"2.1-2.2"};
inline constexpr Leiaute v20{0x08,0x24,0x000,0x008,0x040,0x078,0x0B0,0x0E8,0x120,0x158,0x190,0x1A8,0x1AC,0x1B0,0x1B4,0x1B8,0x1BC,0x1C0,
  0x1C4,0x1C8,"2.0"};

constexpr std::uint32_t flag_hdr=1u<<0,flag_mv_display=1u<<1,flag_mv_jitter=1u<<2,flag_depth_inverted=1u<<3,flag_depth_infinite=1u<<4,
  flag_auto_exposure=1u<<5,flag_dynamic_resolution=1u<<6;
}

std::uint32_t fsr2_estado_para_api(std::uint32_t s) noexcept;
std::uint32_t api_estado_para_fsr2(std::uint32_t s) noexcept;

bool fsr2_to_generic(const void* source,std::byte* destination,std::size_t size,const fsr2::Leiaute& L=fsr2::v21) noexcept;

const fsr2::Leiaute* leiaute_plausivel(const void* source,bool legivel_v21) noexcept;

void generic_to_fsr2(const std::byte* generic,const void* original,std::byte* destination,std::size_t size,const fsr2::Leiaute& L=fsr2::v21) noexcept;
}
