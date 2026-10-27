#include "fsr2_ffx_traducao.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace nrffx {
namespace fsr2ffx {
namespace {
template<class T>T load(const void* p,std::size_t o){T v{};std::memcpy(&v,static_cast<const std::byte*>(p)+o,sizeof(v));return v;}
bool tamanho_ok(std::uint32_t v) noexcept{return v>=16&&v<=16384;}

FfxApiResource recurso(const void* s,std::size_t o,const fsr2::Leiaute& L,std::uint32_t uso,Descrever descrever,void* dado){
  void* r=load<void*>(s,o);if(!r)return FfxApiResource{};
  return descrever(r,fsr2_estado_para_api(load<std::uint32_t>(s,o+L.res_state)),uso,dado);
}
}
std::uint32_t flags_para_ffx(std::uint32_t fsr2) noexcept{
  std::uint32_t f=fsr2&0x7Fu;
  if(fsr2&(1u<<8))f|=FFX_UPSCALE_ENABLE_DEBUG_CHECKING;
  return f;
}
bool descricao_plausivel(const void* inicio,Descricao& d) noexcept{
  if(!inicio)return false;
  Descricao x{};std::memcpy(&x,inicio,sizeof x);
  if(x.flags&~0x1FFu)return false;
  if(!tamanho_ok(x.max_w)||!tamanho_ok(x.max_h)||!tamanho_ok(x.tela_w)||!tamanho_ok(x.tela_h))return false;
  d=x;return true;
}
void criar_upscale(const Descricao& d,ffxCreateContextDescUpscale& up) noexcept{
  up.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
  up.flags=flags_para_ffx(d.flags);
  up.maxRenderSize={d.max_w,d.max_h};
  up.maxUpscaleSize={d.tela_w,d.tela_h};
}
bool traduzir(const void* s,std::size_t legivel,const fsr2::Leiaute& L,const Descricao& ctx,Descrever descrever,void* dado,
              ffxDispatchDescUpscale& d) noexcept{
  if(!s||!descrever||legivel<L.dispatch_prefix)return false;
  if(!load<void*>(s,L.commandList)||!load<void*>(s,L.color)||!load<void*>(s,L.motionVectors)||!load<void*>(s,L.output))return false;
  d=ffxDispatchDescUpscale{};
  d.header.type=FFX_API_DISPATCH_DESC_TYPE_UPSCALE;
  d.commandList=load<void*>(s,L.commandList);
  d.color=recurso(s,L.color,L,0,descrever,dado);
  d.depth=recurso(s,L.depth,L,0,descrever,dado);
  d.motionVectors=recurso(s,L.motionVectors,L,0,descrever,dado);
  d.exposure=recurso(s,L.exposure,L,0,descrever,dado);
  d.reactive=recurso(s,L.reactive,L,0,descrever,dado);
  d.transparencyAndComposition=recurso(s,L.transparencyAndComposition,L,0,descrever,dado);
  d.output=recurso(s,L.output,L,FFX_API_RESOURCE_USAGE_UAV,descrever,dado);

  d.jitterOffset={load<float>(s,L.jitterOffset),load<float>(s,L.jitterOffset+4)};
  d.motionVectorScale={load<float>(s,L.jitterOffset+8),load<float>(s,L.jitterOffset+12)};
  d.renderSize={load<std::uint32_t>(s,L.jitterOffset+0x10),load<std::uint32_t>(s,L.jitterOffset+0x14)};
  if(ctx.tela_w&&ctx.tela_h)d.upscaleSize={ctx.tela_w,ctx.tela_h};
  else d.upscaleSize={load<std::uint32_t>(s,L.output+L.res_description+8),load<std::uint32_t>(s,L.output+L.res_description+12)};
  d.enableSharpening=load<std::uint8_t>(s,L.enableSharpening)!=0;
  d.sharpness=load<float>(s,L.sharpness);
  d.frameTimeDelta=load<float>(s,L.frameTimeDelta);
  d.preExposure=load<float>(s,L.preExposure);
  d.reset=load<std::uint8_t>(s,L.reset)!=0;
  d.cameraNear=load<float>(s,L.cameraNear);d.cameraFar=load<float>(s,L.cameraFar);d.cameraFovAngleVertical=load<float>(s,L.cameraFovAngleVertical);
  d.viewSpaceToMetersFactor=1.f;
  if(&L==&fsr2::v21&&legivel>=v22_viewSpaceToMetersFactor+sizeof(float)){
    const float v=load<float>(s,v22_viewSpaceToMetersFactor);
    if(std::isfinite(v)&&v>0.f&&v<=10000.f)d.viewSpaceToMetersFactor=v;
  }
  d.flags=0;
  return true;
}

bool auto_reativo_22(const void* s,std::size_t legivel,const fsr2::Leiaute& L,Auto22& a) noexcept{
  if(!s)return false;
  a=Auto22{};
  if(&L!=&fsr2::v21||legivel<v22_fim)return true;
  a.lido=true;
  a.ligado=load<std::uint8_t>(s,v22_enableAutoReactive)!=0;
  a.opaco=load<void*>(s,v22_colorOpaqueOnly)!=nullptr;
  a.tc_limiar=load<float>(s,v22_autoTcThreshold);a.tc_escala=load<float>(s,v22_autoTcThreshold+4);
  a.reativo_escala=load<float>(s,v22_autoTcThreshold+8);a.reativo_max=load<float>(s,v22_autoTcThreshold+12);
  return true;
}
namespace {

int opcional(char* o,std::size_t n,const char* nome,const char* nulo,const FfxApiResource& r){
  if(!r.resource)return std::snprintf(o,n,"%s %s",nome,nulo);
  return std::snprintf(o,n,"%s fmt %u %ux%u estado 0x%x",nome,r.description.format,r.description.width,r.description.height,r.state);
}
}
void texto_entradas(const ffxDispatchDescUpscale& d,const Auto22& a,char* out,std::size_t n) noexcept{
  if(!out||!n)return;
  *out=0;std::size_t k=0;
  auto mais=[&](int w){if(w>0)k+=std::size_t(w);if(k>=n)k=n-1;};
  mais(opcional(out+k,n-k,"exposicao","nula",d.exposure));mais(std::snprintf(out+k,n-k,"; "));
  mais(opcional(out+k,n-k,"reativo","nulo",d.reactive));mais(std::snprintf(out+k,n-k,"; "));
  mais(opcional(out+k,n-k,"composicao","nula",d.transparencyAndComposition));
  if(a.lido)mais(std::snprintf(out+k,n-k,"; auto-reativo %u (cor opaca %u, tc %.3g/%.3g, reativo %.3g/%.3g)",a.ligado?1u:0u,a.opaco?1u:0u,
                            double(a.tc_limiar),double(a.tc_escala),double(a.reativo_escala),double(a.reativo_max)));
  else mais(std::snprintf(out+k,n-k,"; auto-reativo 0 (sem o bloco do 2.2)"));
  mais(std::snprintf(out+k,n-k,"; MV escala %.6g,%.6g; jitter %.6g,%.6g; nitidez %u %.3g; dt %.3g ms; pre %.6g; near %.6g far %.6g fov %.4g; metros %.4g",
       double(d.motionVectorScale.x),double(d.motionVectorScale.y),double(d.jitterOffset.x),double(d.jitterOffset.y),d.enableSharpening?1u:0u,
       double(d.sharpness),double(d.frameTimeDelta),double(d.preExposure),double(d.cameraNear),double(d.cameraFar),double(d.cameraFovAngleVertical),
       double(d.viewSpaceToMetersFactor)));
}
std::uint64_t chave_entradas(const ffxDispatchDescUpscale& d,const Auto22& a) noexcept{
  auto r=[](const FfxApiResource& x)->std::uint64_t{return x.resource?(std::uint64_t{1}<<15)|(x.description.format&0x7FFFu):0u;};
  return r(d.exposure)|(r(d.reactive)<<16)|(r(d.transparencyAndComposition)<<32)|
         (std::uint64_t{a.lido}<<48)|(std::uint64_t{a.ligado}<<49)|(std::uint64_t{a.opaco}<<50);
}
}
}
