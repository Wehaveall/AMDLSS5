#include "legacy_fsr3_descriptor.h"
#include <cstring>
#include <utility>
namespace nrffx {
namespace {
template<class T>T load(const void* p,std::size_t o){T v{};std::memcpy(&v,static_cast<const std::byte*>(p)+o,sizeof(v));return v;}
template<class T>void store(std::byte* p,std::size_t o,T v){std::memcpy(p+o,&v,sizeof(v));}
}
bool legacy_fsr3_to_generic(const void* source,std::byte* destination,std::size_t size) noexcept {
 if(!source||!destination||size<descriptor_size)return false;
 std::memset(destination,0,descriptor_size);store(destination,0,std::uint64_t{0x10001});
 store(destination,0x10,load<void*>(source,0));
 for(auto pair:{std::pair{0x008u,0x018u},std::pair{0x0b8u,0x048u},std::pair{0x168u,0x078u},std::pair{0x218u,0x0a8u},std::pair{0x428u,0x138u}})
  std::memcpy(destination+pair.second,static_cast<const std::byte*>(source)+pair.first,0x2c);
 std::memcpy(destination+0x170,static_cast<const std::byte*>(source)+0x4e0,0x10);

 store(destination,0x180,load<std::uint32_t>(source,0x438));store(destination,0x184,load<std::uint32_t>(source,0x43c));
 store(destination,0x194,load<float>(source,0x4fc));store(destination,0x198,load<std::uint8_t>(source,0x500));
 Descriptor d{};return decode(destination,d);
}
}
