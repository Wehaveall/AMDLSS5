#include "input_adapter.h"

#include <cmath>

namespace nrscale {
namespace {

constexpr std::uint32_t kR32G32B32A32Typeless=1,kR32G32B32A32Float=2,kR16G16B16A16Typeless=9,kR16G16B16A16Float=10,
  kR32G32Typeless=15,kR32G32Float=16,kR10G10B10A2Typeless=23,kR10G10B10A2Unorm=24,kR11G11B10Float=26,
  kR8G8B8A8Typeless=27,kR8G8B8A8Unorm=28,kR8G8B8A8UnormSrgb=29,kR16G16Typeless=33,kR16G16Float=34,
  kR9G9B9E5=67,kB8G8R8A8Unorm=87,kB8G8R8A8Typeless=90,kB8G8R8A8UnormSrgb=91,
  kR32G8X24Typeless=19,kR32Typeless=39,kR32Float=41,kR24G8Typeless=44,kR24UnormX8Typeless=46,kR16Typeless=53,kR16Float=54,kR16Unorm=56;
constexpr std::uint32_t kTexture2d=3,kAllowUav=4,kDenySrv=8;
bool plain_texture(const NrResourceDesc& r,std::uint64_t w,std::uint32_t h) noexcept{
  return r.dimension==kTexture2d&&r.width==w&&r.height==h&&r.depth_or_array_size==1&&r.mip_levels>=1&&
    r.sample_count==1&&!(r.flags&kDenySrv);
}

bool native_color(const NrResourceDesc& r) noexcept{
  return (r.format==kR16G16B16A16Float||r.format==kR9G9B9E5)&&r.mip_levels==1&&(r.flags&kAllowUav);
}
}
std::uint32_t adapter_color_view(std::uint32_t f) noexcept{
  switch(f){
    case kR9G9B9E5:case kR16G16B16A16Float:case kR11G11B10Float:case kR32G32B32A32Float:case kR10G10B10A2Unorm:
    case kR8G8B8A8Unorm:case kR8G8B8A8UnormSrgb:case kB8G8R8A8Unorm:case kB8G8R8A8UnormSrgb:return f;
    case 88:case 93:return f;
    case 92:return 88;
    case kR16G16B16A16Typeless:return kR16G16B16A16Float;
    case kR32G32B32A32Typeless:return kR32G32B32A32Float;
    case kR10G10B10A2Typeless:return kR10G10B10A2Unorm;
    case kR8G8B8A8Typeless:return kR8G8B8A8Unorm;
    case kB8G8R8A8Typeless:return kB8G8R8A8Unorm;
    default:return 0;
  }
}
std::uint32_t adapter_output_view(std::uint32_t f) noexcept{
  switch(f){
    case kR11G11B10Float:case kR10G10B10A2Unorm:case kR8G8B8A8Unorm:case kR32G32B32A32Float:case kR16G16B16A16Float:return f;
    case kR10G10B10A2Typeless:return kR10G10B10A2Unorm;
    case kR8G8B8A8Typeless:return kR8G8B8A8Unorm;
    case kR16G16B16A16Typeless:return kR16G16B16A16Float;
    case kR32G32B32A32Typeless:return kR32G32B32A32Float;
    default:return 0;
  }
}
std::uint32_t bits_cor_exibicao(std::uint32_t f) noexcept{
  switch(f){
    case kR10G10B10A2Unorm:case kR10G10B10A2Typeless:case kR8G8B8A8Unorm:case kR8G8B8A8Typeless:case kB8G8R8A8Unorm:case kB8G8R8A8Typeless:
    case 88:case 92:return 1u<<14;
    case kR8G8B8A8UnormSrgb:case kB8G8R8A8UnormSrgb:case 93:return (1u<<14)|(1u<<15);
    default:return 0u;
  }
}
std::uint32_t adapter_exposure_view(std::uint32_t f) noexcept{
  switch(f){
    case kR32Float:case kR16Float:case kR16G16Float:case kR16G16B16A16Float:case kR32G32B32A32Float:case kR32G32Float:return f;
    case kR32Typeless:return kR32Float;
    case kR16G16Typeless:return kR16G16Float;
    case kR16G16B16A16Typeless:return kR16G16B16A16Float;
    case kR32G32B32A32Typeless:return kR32G32B32A32Float;
    default:return 0;
  }
}
std::uint32_t adapter_motion_view(std::uint32_t f) noexcept{
  switch(f){
    case kR16G16Float:case kR16G16Typeless:return kR16G16Float;
    case kR32G32Float:case kR32G32Typeless:return kR32G32Float;
    case kR16G16B16A16Float:case kR16G16B16A16Typeless:return kR16G16B16A16Float;
    case kR32G32B32A32Float:case kR32G32B32A32Typeless:return kR32G32B32A32Float;
    default:return 0;
  }
}
std::uint32_t adapter_depth_view(std::uint32_t f) noexcept{
  switch(f){
    case kR24G8Typeless:return kR24UnormX8Typeless;
    case kR16Typeless:case kR16Unorm:return kR16Unorm;
    case kR16Float:return kR16Float;
    default:return 0;
  }
}
bool inputs_larger_than_render(const nrffx::RawDescriptor& d,const NrResourceDesc& input,const NrResourceDesc& motion,const NrResourceDesc& depth) noexcept{
  const NrResourceDesc* r[3]{&input,&motion,&depth};bool maior=false;
  for(const auto* x:r){if(x->width<d.render_w||x->height<d.render_h)return false;maior|=x->width>d.render_w||x->height>d.render_h;}
  return maior;
}
AdapterPlan plan_adapter(const nrffx::RawDescriptor& d,const NrResourceDesc& input,const NrResourceDesc& output,
                         const NrResourceDesc& motion,const NrResourceDesc& depth,unsigned sticky,bool subrect,
                         const NrResourceDesc* exposure) noexcept{
  AdapterPlan p{};
  if(d.reset){p.reason="adapt_reset_frame";return p;}

  const bool mv_tela=(d.output_w!=d.render_w||d.output_h!=d.render_h)&&plain_texture(motion,d.output_w,d.output_h)&&
    plain_texture(input,d.render_w,d.render_h)&&plain_texture(depth,d.render_w,d.render_h);
  const bool sub=!mv_tela&&subrect&&inputs_larger_than_render(d,input,motion,depth);
  auto cabe=[&](const NrResourceDesc& r){return sub?plain_texture(r,r.width,r.height)&&r.width>=d.render_w&&r.height>=d.render_h
                                                   :plain_texture(r,d.render_w,d.render_h);};
  if(!cabe(input)||!(mv_tela||cabe(motion))||!plain_texture(output,d.output_w,d.output_h)||!cabe(depth)){p.reason="adapt_geometry";return p;}
  if(mv_tela){p.mv_tela=true;sticky|=kAdaptMotion;}

  if(sub){p.subrect=true;sticky|=kAdaptColor|kAdaptMotion|kAdaptDepth;}
  const float pre=d.pre_exposure;
  if(!std::isfinite(pre)||!(pre>=1.f/65504.f)||!(pre<=65504.f)){p.reason="adapt_pre_exposure_invalid";return p;}
  const float mx=d.motion_x,my=d.motion_y;
  if(!std::isfinite(mx)||!std::isfinite(my)||mx==0.f||my==0.f||std::fabs(mx)>1048576.f||std::fabs(my)>1048576.f){
    p.reason="adapt_motion_scale_invalid";return p;}
  const bool exposed=pre!=1.f;
  p.color=exposed||!native_color(input)||(sticky&kAdaptColor);
  if(p.color){
    p.color_view=adapter_color_view(input.format);
    if(!p.color_view){p.reason="adapt_color_format";return p;}
  }

  p.output=!native_color(output)||(sticky&kAdaptOutput);
  if(p.output){
    p.output_view=adapter_output_view(output.format);
    if(!p.output_view||!(output.flags&kAllowUav)||output.mip_levels!=1){p.reason="adapt_output_format";return p;}
  }
  if(exposed){

    if(!p.output&&(output.format!=kR16G16B16A16Float||!(output.flags&kAllowUav)||output.mip_levels!=1)){p.reason="adapt_output_not_rgba16f_uav";return p;}
    p.color_scale=1.f/pre;p.output_scale=pre;
  }
  const bool native_motion=(motion.format==kR16G16Float||motion.format==kR16G16B16A16Float)&&
    mx==static_cast<float>(d.render_w)&&my==static_cast<float>(d.render_h);
  p.motion=!native_motion||(sticky&kAdaptMotion);
  if(p.motion){
    p.motion_view=adapter_motion_view(motion.format);
    if(!p.motion_view){p.reason="adapt_motion_format";return p;}
    p.motion_x=mx/static_cast<float>(d.render_w);p.motion_y=my/static_cast<float>(d.render_h);
    if(p.mv_tela){
      p.motion_x=mx/static_cast<float>(d.output_w);p.motion_y=my/static_cast<float>(d.output_h);
      p.motion_amostra_x=static_cast<float>(d.output_w)/static_cast<float>(d.render_w);
      p.motion_amostra_y=static_cast<float>(d.output_h)/static_cast<float>(d.render_h);
    }
  }

  const bool native_depth=depth.format==kR32Typeless||depth.format==kR32G8X24Typeless||depth.format==kR32Float;
  p.depth=!native_depth||(sticky&kAdaptDepth);
  if(p.depth){

    p.depth_view=native_depth?(depth.format==kR32G8X24Typeless?20u:kR32Float):adapter_depth_view(depth.format);
    if(!p.depth_view){p.reason="adapt_depth_format";return p;}
  }

  if(d.exposure&&exposure){
    const bool nativa=exposure->format==kR32Float&&exposure->width==1&&exposure->height==1&&exposure->mip_levels==1&&
      exposure->dimension==kTexture2d&&exposure->sample_count==1&&!(exposure->flags&kDenySrv);
    p.exposure=!nativa||(sticky&kAdaptExposure);
    if(p.exposure){
      p.exposure_view=adapter_exposure_view(exposure->format);
      if(!p.exposure_view||exposure->dimension!=kTexture2d||exposure->sample_count!=1||(exposure->flags&kDenySrv)){p.reason="adapt_exposure_format";return p;}
    }
  }
  p.adapt=p.color||p.motion||p.depth||p.output||p.exposure;
  return p;
}
}
