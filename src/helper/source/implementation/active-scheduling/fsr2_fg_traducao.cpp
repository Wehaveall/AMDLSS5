#include "fsr2_fg_traducao.h"

namespace nrffx {
namespace fsr2fg {
static_assert(FFX_UPSCALE_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS==FFX_FRAMEGENERATION_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS&&
              FFX_UPSCALE_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION==FFX_FRAMEGENERATION_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION&&
              FFX_UPSCALE_ENABLE_DEPTH_INVERTED==FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED&&
              FFX_UPSCALE_ENABLE_DEPTH_INFINITE==FFX_FRAMEGENERATION_ENABLE_DEPTH_INFINITE,"bits 1-4: os mesmos no upscale e no FG");
std::uint32_t flags_fg(std::uint32_t f) noexcept{
  return f&(FFX_FRAMEGENERATION_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS|FFX_FRAMEGENERATION_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION|
            FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED|FFX_FRAMEGENERATION_ENABLE_DEPTH_INFINITE);
}
bool backbuffer_hdr(std::uint32_t f) noexcept{
  return f==FFX_API_SURFACE_FORMAT_R10G10B10A2_UNORM||f==FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT;
}
void criar_fg(std::uint32_t flags_upscale,std::uint32_t tela_w,std::uint32_t tela_h,std::uint32_t render_w,std::uint32_t render_h,
              std::uint32_t formato_ffx,ffxCreateContextDescFrameGeneration& fd) noexcept{
  fd.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;
  fd.flags=flags_fg(flags_upscale)|(backbuffer_hdr(formato_ffx)?std::uint32_t(FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE):0u);
  fd.displaySize={tela_w,tela_h};
  fd.maxRenderSize={render_w>tela_w?render_w:tela_w,render_h>tela_h?render_h:tela_h};
  fd.backBufferFormat=formato_ffx;
}
void configurar(void* swapchain,bool ligado,std::uint64_t frame_id,std::uint32_t tela_w,std::uint32_t tela_h,
                FfxApiFrameGenerationDispatchFunc callback,void* contexto_callback,ffxConfigureDescFrameGeneration& c) noexcept{
  c=ffxConfigureDescFrameGeneration{};
  c.header.type=FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;
  c.swapChain=swapchain;
  c.frameGenerationCallback=ligado?callback:nullptr;
  c.frameGenerationCallbackUserContext=ligado?contexto_callback:nullptr;
  c.frameGenerationEnabled=ligado;
  c.allowAsyncWorkloads=false;
  c.flags=0;
  c.onlyPresentGenerated=false;
  c.generationRect={0,0,static_cast<int32_t>(tela_w),static_cast<int32_t>(tela_h)};
  c.frameID=frame_id;
}
bool preparar(const ffxDispatchDescUpscale& u,std::uint64_t frame_id,bool reset_extra,ffxDispatchDescFrameGenerationPrepareV2& p) noexcept{
  if(!u.commandList||!u.depth.resource||!u.motionVectors.resource||!u.renderSize.width||!u.renderSize.height)return false;
  p=ffxDispatchDescFrameGenerationPrepareV2{};
  p.header.type=FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_V2;
  p.frameID=frame_id;
  p.flags=0;
  p.commandList=u.commandList;
  p.renderSize=u.renderSize;
  p.jitterOffset=u.jitterOffset;
  p.motionVectorScale=u.motionVectorScale;
  p.frameTimeDelta=u.frameTimeDelta;
  p.reset=u.reset||reset_extra;
  p.cameraNear=u.cameraNear;
  p.cameraFar=u.cameraFar;
  p.cameraFovAngleVertical=u.cameraFovAngleVertical;
  p.viewSpaceToMetersFactor=u.viewSpaceToMetersFactor;
  p.depth=u.depth;
  p.motionVectors=u.motionVectors;

  p.cameraUp[1]=1.f;p.cameraRight[0]=1.f;p.cameraForward[2]=1.f;
  return true;
}
void ao_upscale(Ritmo& r) noexcept{++r.upscales_desde_present;}
AoPresent ao_present(Ritmo& r,bool fg_ligado) noexcept{
  const bool novo=r.upscales_desde_present>0;
  r.upscales_desde_present=0;
  const AoPresent a=(!novo&&!r.anterior_novo&&fg_ligado)?AoPresent::Desliga:AoPresent::Segue;
  r.anterior_novo=novo;
  return a;
}
bool aplicar_pedido(long pedido,bool atual) noexcept{return pedido==0||pedido==1?pedido==1:atual;}
bool precisa_recriar(const Chave& a,const Chave& n) noexcept{
  return a.device!=n.device||a.tela_w!=n.tela_w||a.tela_h!=n.tela_h||a.flags!=n.flags||a.formato!=n.formato||
         n.render_w>a.render_w||n.render_h>a.render_h;
}
}
}
