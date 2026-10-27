#pragma once
#include <d3d12.h>
#include <ffx_api.h>
#include <ffx_api_types.h>
#include <dx12/ffx_api_dx12.h>
#include <cstdint>

namespace nrffx {
namespace fsr2ffx {
inline FfxApiResource descrever_dx12(void* r,std::uint32_t estado,std::uint32_t uso,void*) {
  FfxApiResource x=ffxApiGetResourceDX12(static_cast<ID3D12Resource*>(r),estado,uso);
  switch(x.description.format){
    case FFX_API_SURFACE_FORMAT_R32_TYPELESS:x.description.format=FFX_API_SURFACE_FORMAT_R32_FLOAT;break;
    case FFX_API_SURFACE_FORMAT_R16_TYPELESS:x.description.format=FFX_API_SURFACE_FORMAT_R16_FLOAT;break;
    case FFX_API_SURFACE_FORMAT_R16G16_TYPELESS:x.description.format=FFX_API_SURFACE_FORMAT_R16G16_FLOAT;break;
    case FFX_API_SURFACE_FORMAT_R32G32_TYPELESS:x.description.format=FFX_API_SURFACE_FORMAT_R32G32_FLOAT;break;
    case FFX_API_SURFACE_FORMAT_R16G16B16A16_TYPELESS:x.description.format=FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT;break;
    case FFX_API_SURFACE_FORMAT_R32G32B32A32_TYPELESS:x.description.format=FFX_API_SURFACE_FORMAT_R32G32B32A32_FLOAT;break;
    case FFX_API_SURFACE_FORMAT_R8G8B8A8_TYPELESS:x.description.format=FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM;break;
    case FFX_API_SURFACE_FORMAT_B8G8R8A8_TYPELESS:x.description.format=FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM;break;
    case FFX_API_SURFACE_FORMAT_R10G10B10A2_TYPELESS:x.description.format=FFX_API_SURFACE_FORMAT_R10G10B10A2_UNORM;break;
    case FFX_API_SURFACE_FORMAT_R8_TYPELESS:x.description.format=FFX_API_SURFACE_FORMAT_R8_UNORM;break;
    case FFX_API_SURFACE_FORMAT_R8G8_TYPELESS:x.description.format=FFX_API_SURFACE_FORMAT_R8G8_UNORM;break;
    default:break;
  }
  return x;
}

inline constexpr char marca_export[]="nrFsr2FfxVersao";
}
}
