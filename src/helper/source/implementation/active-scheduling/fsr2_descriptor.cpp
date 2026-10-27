#include "fsr2_descriptor.h"
#include <cstring>
namespace nrffx {
namespace {
template<class T>T load(const void* p,std::size_t o){T v{};std::memcpy(&v,static_cast<const std::byte*>(p)+o,sizeof(v));return v;}
template<class T>void store(std::byte* p,std::size_t o,T v){std::memcpy(p+o,&v,sizeof(v));}

struct Par{std::size_t generico;std::size_t fsr2::Leiaute::* campo;};
constexpr Par recursos[]{{0x018,&fsr2::Leiaute::color},{0x048,&fsr2::Leiaute::depth},{0x078,&fsr2::Leiaute::motionVectors},
                         {0x0A8,&fsr2::Leiaute::exposure},{0x0D8,&fsr2::Leiaute::reactive},
                         {0x108,&fsr2::Leiaute::transparencyAndComposition},{0x138,&fsr2::Leiaute::output}};
bool plausivel(const void* s,const fsr2::Leiaute& L) noexcept{
  const auto rw=load<std::uint32_t>(s,L.jitterOffset+0x10),rh=load<std::uint32_t>(s,L.jitterOffset+0x14);
  if(rw<16||rh<16||rw>16384||rh>16384)return false;
  if(!load<void*>(s,L.color)||!load<void*>(s,L.motionVectors)||!load<void*>(s,L.output))return false;
  const std::size_t tam[2]{L.color+L.res_description+8,L.output+L.res_description+8};
  for(const std::size_t t:tam){
    const std::uint32_t w=load<std::uint32_t>(s,t);const std::uint32_t h=load<std::uint32_t>(s,t+4);
    if(w<rw||h<rh||w>16384||h>16384)return false;}
  return true;
}
}
const fsr2::Leiaute* leiaute_plausivel(const void* source,bool legivel_v21) noexcept{
  if(!source)return nullptr;
  if(legivel_v21&&plausivel(source,fsr2::v21))return &fsr2::v21;
  return plausivel(source,fsr2::v20)?&fsr2::v20:nullptr;
}
std::uint32_t fsr2_estado_para_api(std::uint32_t s) noexcept{
  switch(s){case 6:return 0x14;case 1:return 0x2;case 2:return 0x4;case 4:return 0x10;case 8:return 0x20;default:return 0x1;}
}
std::uint32_t api_estado_para_fsr2(std::uint32_t s) noexcept{
  switch(s){case 0x14:return 6;case 0x2:return 1;case 0x4:return 2;case 0x10:return 4;case 0x20:return 8;default:return 0;}
}
bool fsr2_to_generic(const void* source,std::byte* destination,std::size_t size,const fsr2::Leiaute& L) noexcept{
  if(!source||!destination||size<descriptor_size)return false;
  std::memset(destination,0,descriptor_size);store(destination,0,std::uint64_t{0x10001});
  store(destination,0x10,load<void*>(source,L.commandList));
  for(const auto& r:recursos){
    const auto* s=static_cast<const std::byte*>(source)+L.*r.campo;
    store(destination,r.generico,load<void*>(s,fsr2::res_resource));

    std::memcpy(destination+r.generico+8,s+L.res_description,0x1C);store(destination,r.generico+8+4,std::uint32_t{0});
    store(destination,r.generico+0x28,fsr2_estado_para_api(load<std::uint32_t>(s,L.res_state)));
  }
  std::memcpy(destination+0x168,static_cast<const std::byte*>(source)+L.jitterOffset,0x18);

  store(destination,0x180,load<std::uint32_t>(source,L.output+L.res_description+8));
  store(destination,0x184,load<std::uint32_t>(source,L.output+L.res_description+12));
  store(destination,0x188,load<std::uint8_t>(source,L.enableSharpening));
  store(destination,0x18C,load<float>(source,L.sharpness));store(destination,0x190,load<float>(source,L.frameTimeDelta));
  store(destination,0x194,load<float>(source,L.preExposure));store(destination,0x198,load<std::uint8_t>(source,L.reset));
  store(destination,0x19C,load<float>(source,L.cameraNear));store(destination,0x1A0,load<float>(source,L.cameraFar));
  store(destination,0x1A4,load<float>(source,L.cameraFovAngleVertical));
  RawDescriptor d{};return decode_raw(destination,d);
}
void generic_to_fsr2(const std::byte* generic,const void* original,std::byte* destination,std::size_t size,const fsr2::Leiaute& L) noexcept{
  std::memcpy(destination,original,size);
  for(const auto& r:recursos){
    auto* d=destination+L.*r.campo;void* novo=load<void*>(generic,r.generico);
    if(novo==load<void*>(d,fsr2::res_resource))continue;
    store(d,fsr2::res_resource,novo);store(d,L.res_state,api_estado_para_fsr2(load<std::uint32_t>(generic,r.generico+0x28)));
  }
  std::memcpy(destination+L.jitterOffset,generic+0x168,0x18);
  store(destination,L.preExposure,load<float>(generic,0x194));store(destination,L.reset,load<std::uint8_t>(generic,0x198));
}
}
