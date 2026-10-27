#pragma once
#include "../../../runtime/nr_publico.h"
#include <d3d12.h>
#include <cstddef>
#include <cstdio>
namespace nrformato {

inline DXGI_FORMAT vista_cor(DXGI_FORMAT f){switch(f){
 case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:case DXGI_FORMAT_R16G16B16A16_FLOAT:case DXGI_FORMAT_R11G11B10_FLOAT:
 case DXGI_FORMAT_R32G32B32A32_FLOAT:case DXGI_FORMAT_R10G10B10A2_UNORM:case DXGI_FORMAT_R8G8B8A8_UNORM:
 case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:case DXGI_FORMAT_B8G8R8A8_UNORM:case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:return f;
 case DXGI_FORMAT_R16G16B16A16_TYPELESS:return DXGI_FORMAT_R16G16B16A16_FLOAT;
 case DXGI_FORMAT_R32G32B32A32_TYPELESS:return DXGI_FORMAT_R32G32B32A32_FLOAT;
 case DXGI_FORMAT_R10G10B10A2_TYPELESS:return DXGI_FORMAT_R10G10B10A2_UNORM;
 case DXGI_FORMAT_R8G8B8A8_TYPELESS:return DXGI_FORMAT_R8G8B8A8_UNORM;
 case DXGI_FORMAT_B8G8R8A8_TYPELESS:return DXGI_FORMAT_B8G8R8A8_UNORM;
 default:return DXGI_FORMAT_UNKNOWN;}}

inline unsigned bits_cor(DXGI_FORMAT vista){switch(vista){
 case DXGI_FORMAT_R10G10B10A2_UNORM:case DXGI_FORMAT_R8G8B8A8_UNORM:case DXGI_FORMAT_B8G8R8A8_UNORM:return 1u<<14;
 case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:return (1u<<14)|(1u<<15);
 default:return 0u;}}

inline DXGI_FORMAT vista_profundidade(DXGI_FORMAT f){switch(f){
 case DXGI_FORMAT_R32_FLOAT:case DXGI_FORMAT_R32_TYPELESS:return DXGI_FORMAT_R32_FLOAT;
 case DXGI_FORMAT_R32G8X24_TYPELESS:return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
 case DXGI_FORMAT_R24G8_TYPELESS:return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
 case DXGI_FORMAT_R16_TYPELESS:case DXGI_FORMAT_R16_UNORM:return DXGI_FORMAT_R16_UNORM;
 case DXGI_FORMAT_R16_FLOAT:return DXGI_FORMAT_R16_FLOAT;
 default:return DXGI_FORMAT_UNKNOWN;}}

inline DXGI_FORMAT vista_movimento(DXGI_FORMAT f){switch(f){
 case DXGI_FORMAT_R16G16_FLOAT:case DXGI_FORMAT_R16G16_TYPELESS:return DXGI_FORMAT_R16G16_FLOAT;
 case DXGI_FORMAT_R16G16B16A16_FLOAT:case DXGI_FORMAT_R16G16B16A16_TYPELESS:return DXGI_FORMAT_R16G16B16A16_FLOAT;
 case DXGI_FORMAT_R32G32_FLOAT:case DXGI_FORMAT_R32G32_TYPELESS:return DXGI_FORMAT_R32G32_FLOAT;
 default:return DXGI_FORMAT_UNKNOWN;}}

inline constexpr unsigned lado_maximo=16384u;

inline bool entrada(ID3D12Resource* r,DXGI_FORMAT vista){if(!r||vista==DXGI_FORMAT_UNKNOWN)return false;const auto d=r->GetDesc();
 return d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Width&&d.Height&&d.Width<=lado_maximo&&d.Height<=lado_maximo&&d.DepthOrArraySize==1&&
        d.MipLevels>=1&&d.SampleDesc.Count==1&&!(d.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);}

inline bool motivo_papel(const char* papel,const D3D12_RESOURCE_DESC* d,DXGI_FORMAT vista,char* out,std::size_t n){
 if(!d){NRLOGF(out,n,"%s ausente",papel);return true;}
 if(vista==DXGI_FORMAT_UNKNOWN){NRLOGF(out,n,"%s fmt %u",papel,unsigned(d->Format));return true;}
 if(d->Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||!d->Width||!d->Height||d->DepthOrArraySize!=1||d->MipLevels<1||d->SampleDesc.Count!=1){
  NRLOGF(out,n,"%s dim %u array %u amostras %u",papel,unsigned(d->Dimension),unsigned(d->DepthOrArraySize),unsigned(d->SampleDesc.Count));return true;}
 if(d->Width>lado_maximo||d->Height>lado_maximo){NRLOGF(out,n,"%s %llux%u > %u",papel,static_cast<unsigned long long>(d->Width),d->Height,lado_maximo);return true;}
 if(d->Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE){NRLOGF(out,n,"%s DENY_SRV",papel);return true;}
 return false;
}
}
