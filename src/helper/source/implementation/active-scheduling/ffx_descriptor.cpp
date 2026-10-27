#include "ffx_descriptor.h"

#include <cmath>
#include <cstdio>
#include <atomic>
#include <cstring>

namespace nrffx {
namespace {
constexpr std::uint64_t upscale_type=0x10001;
std::atomic<bool> use_game_exposure{true};
std::atomic<FonteTamanhoSaida> fonte_tamanho_saida{nullptr};
template<class T>T load(const void* base,std::size_t offset) noexcept{
  T value{};std::memcpy(&value,static_cast<const std::byte*>(base)+offset,sizeof(value));return value;
}
template<class T>void store(std::byte* base,std::size_t offset,T value) noexcept{
  std::memcpy(base+offset,&value,sizeof(value));
}
const void* find_upscale(const void* head) noexcept{
  auto* current=head;
  for(unsigned i=0;current&&i<64;++i){
    if(load<std::uint64_t>(current,0)==upscale_type)return current;
    current=load<void*>(current,8);
  }
  return nullptr;
}
bool geometry(std::uint32_t rw,std::uint32_t rh,std::uint32_t ow,std::uint32_t oh) noexcept{
  return rw&&rh&&ow&&oh&&rw<=16384&&rh<=16384&&ow<=16384&&oh<=16384;
}
}
bool host_reset(const void* head) noexcept{
  const auto* p=find_upscale(head);return p&&load<std::uint8_t>(p,0x198)!=0;
}
void set_use_game_exposure(bool enabled) noexcept{use_game_exposure.store(enabled,std::memory_order_relaxed);}
void set_fonte_tamanho_saida(FonteTamanhoSaida fonte) noexcept{fonte_tamanho_saida.store(fonte,std::memory_order_release);}

static void fields(const void* p,Descriptor& d) noexcept{
  d.command=load<void*>(p,0x10);d.input=load<void*>(p,0x18);d.input_state=load<std::uint32_t>(p,0x40);
  d.depth=load<void*>(p,0x48);d.depth_state=load<std::uint32_t>(p,0x70);
  d.motion=load<void*>(p,0x78);d.motion_state=load<std::uint32_t>(p,0xA0);
  d.exposure=load<void*>(p,0xA8);d.exposure_state=load<std::uint32_t>(p,0xD0);
  if(!use_game_exposure.load(std::memory_order_relaxed)){d.exposure=nullptr;d.exposure_state=0;}
  d.output=load<void*>(p,0x138);d.output_state=load<std::uint32_t>(p,0x160);
  d.motion_x=load<float>(p,0x170);d.motion_y=load<float>(p,0x174);
  d.jitter_x=load<float>(p,0x168);d.jitter_y=load<float>(p,0x16C);
  d.render_w=load<std::uint32_t>(p,0x178);d.render_h=load<std::uint32_t>(p,0x17C);
  d.output_w=load<std::uint32_t>(p,0x180);d.output_h=load<std::uint32_t>(p,0x184);
  if(!d.output_w&&!d.output_h&&d.output)if(auto f=fonte_tamanho_saida.load(std::memory_order_acquire)){
    std::uint32_t w{},h{};if(f(d.output,w,h)){d.output_w=w;d.output_h=h;}}
  d.camera_near=load<float>(p,0x19C);d.camera_far=load<float>(p,0x1A0);d.camera_fov=load<float>(p,0x1A4);
  d.ffx_flags=load<std::uint32_t>(p,0x1AC);
}

static const char* check(const void* head,Descriptor& d) noexcept{
  d={};const auto* p=find_upscale(head);
  if(!p)return "ffx_upscale_type_0x10001";
  if(load<std::uint8_t>(p,0x198)!=0)return "ffx_reset_flag";
  if(load<float>(p,0x194)!=1.0f)return "ffx_pre_exposure_1";
  fields(p,d);
  if(!(d.command&&d.input&&d.output&&d.motion&&d.depth&&d.input!=d.output))return "ffx_required_pointers";
  if(!geometry(d.render_w,d.render_h,d.output_w,d.output_h))return "ffx_geometry_16384";
  if(!(std::isfinite(d.motion_x)&&std::isfinite(d.motion_y)&&d.motion_x>0&&d.motion_y>0))return "ffx_motion_scale_positive";
  return nullptr;
}
bool decode(const void* head,Descriptor& d) noexcept{return check(head,d)==nullptr;}
const char* refusal(const void* head,char* detalhe,std::size_t n,char* campo,std::size_t m) noexcept{
  Descriptor d{};const char* id=check(head,d);
  auto put=[](char* out,std::size_t size,const char* format,auto... values){if(out&&size)std::snprintf(out,size,format,values...);};
  put(detalhe,n,"%s","");put(campo,m,"%s","");
  if(!id)return nullptr;
  const auto* p=find_upscale(head);
  if(!p){put(detalhe,n,"sem cabecalho 0x%llx nos 64 primeiros elos da cadeia (tipo do primeiro=0x%llx)",static_cast<unsigned long long>(upscale_type),
      static_cast<unsigned long long>(head?load<std::uint64_t>(head,0):0));put(campo,m,"%s","tipo FFX");return id;}
  const float pre=load<float>(p,0x194);const unsigned reset=load<std::uint8_t>(p,0x198);
  put(detalhe,n,"reset=%u preExposure=%.6g command=%p input=%p output=%p motion=%p depth=%p exposure=%p render=%ux%u output=%ux%u motionVectorScale=%.6g,%.6g",
      reset,pre,load<void*>(p,0x10),load<void*>(p,0x18),load<void*>(p,0x138),load<void*>(p,0x78),load<void*>(p,0x48),load<void*>(p,0xA8),
      load<std::uint32_t>(p,0x178),load<std::uint32_t>(p,0x17C),load<std::uint32_t>(p,0x180),load<std::uint32_t>(p,0x184),
      static_cast<double>(load<float>(p,0x170)),static_cast<double>(load<float>(p,0x174)));
  if(!std::strcmp(id,"ffx_reset_flag"))put(campo,m,"reset=%u",reset);
  else if(!std::strcmp(id,"ffx_pre_exposure_1"))put(campo,m,"preExposure %.3g",static_cast<double>(pre));
  else if(!std::strcmp(id,"ffx_required_pointers"))put(campo,m,"%s",!d.command?"command nulo":!d.input?"input nulo":!d.output?"output nulo":
      !d.motion?"motion nulo":!d.depth?"depth nulo":"input=output");
  else if(!std::strcmp(id,"ffx_geometry_16384"))put(campo,m,"render %ux%u saida %ux%u",d.render_w,d.render_h,d.output_w,d.output_h);
  else put(campo,m,"mvScale %.4g,%.4g",static_cast<double>(d.motion_x),static_cast<double>(d.motion_y));
  return id;
}
bool decode_raw(const void* head,RawDescriptor& r) noexcept{
  r={};const auto* p=find_upscale(head);
  if(!p)return false;
  r.reset=load<std::uint8_t>(p,0x198)!=0;r.pre_exposure=load<float>(p,0x194);
  auto& d=static_cast<Descriptor&>(r);fields(p,d);
  return d.command&&d.input&&d.output&&d.motion&&d.depth&&d.input!=d.output&&
    geometry(d.render_w,d.render_h,d.output_w,d.output_h)&&std::isfinite(d.motion_x)&&std::isfinite(d.motion_y)&&
    std::isfinite(r.pre_exposure);
}
bool encode_adapted_copy(const void* source,std::size_t source_size,const AdaptedFields& f,
    std::byte* destination,std::size_t destination_size) noexcept{
  if(!source||!destination||source_size<descriptor_size||destination_size<descriptor_size)return false;
  const auto* upscale=find_upscale(source);RawDescriptor original{};
  if(!upscale||!decode_raw(upscale,original)||original.reset)return false;
  std::memcpy(destination,upscale,descriptor_size);
  if(f.input){store(destination,0x18,f.input);store(destination,0x40,f.input_state);}
  if(f.motion){store(destination,0x78,f.motion);store(destination,0xA0,f.motion_state);}
  if(f.depth){store(destination,0x48,f.depth);store(destination,0x70,f.depth_state);}
  if(f.output){store(destination,0x138,f.output);store(destination,0x160,f.output_state);}
  if(f.exposure){store(destination,0xA8,f.exposure);store(destination,0xD0,f.exposure_state);}
  store(destination,0x170,static_cast<float>(original.render_w));store(destination,0x174,static_cast<float>(original.render_h));
  store(destination,0x194,1.0f);
  Descriptor strict{};return decode(destination,strict);
}
bool encode_scaled_copy(const void* source,std::size_t source_size,const ScaledFields& f,
    std::byte* destination,std::size_t destination_size) noexcept{
  if(!source||!destination||source_size<descriptor_size||destination_size<descriptor_size||
     !f.input||!f.output||!f.motion||!f.depth||f.input==f.output||
     !geometry(f.render_w,f.render_h,f.output_w,f.output_h)||
     !std::isfinite(f.motion_x)||!std::isfinite(f.motion_y)||f.motion_x!=static_cast<float>(f.render_w)||f.motion_y!=static_cast<float>(f.render_h))return false;
  Descriptor original{};const auto* upscale=find_upscale(source);if(!upscale||!decode(upscale,original))return false;
  std::memcpy(destination,upscale,descriptor_size);
  store(destination,0x18,f.input);store(destination,0x48,f.depth);store(destination,0x78,f.motion);store(destination,0x138,f.output);
  store(destination,0x170,f.motion_x);store(destination,0x174,f.motion_y);
  store(destination,0x178,f.render_w);store(destination,0x17C,f.render_h);store(destination,0x180,f.output_w);store(destination,0x184,f.output_h);
  return true;
}
}
