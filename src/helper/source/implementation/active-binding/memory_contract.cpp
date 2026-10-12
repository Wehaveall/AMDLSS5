#include "memory_contract.h"
#include <cstring>
#include <windows.h>
#include <intrin.h>
#include <algorithm>
namespace nractive {
bool publish_capture_bookkeeping(void* image,std::size_t size) noexcept{
    if(!image||size<0x94BA0)return false;
    const auto start=reinterpret_cast<std::uintptr_t>(image);
    if(start>UINTPTR_MAX-0x94BA0||start%8)return false;
    auto* b=static_cast<std::uint8_t*>(image);
    for(auto rva:{0x94A78u,0x949E0u,0x94B9Cu}){
        MEMORY_BASIC_INFORMATION m{};if(!VirtualQuery(b+rva,&m,sizeof(m))||m.State!=MEM_COMMIT||m.Protect&(PAGE_GUARD|PAGE_NOACCESS)||!(m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return false;
        const auto offset=reinterpret_cast<std::uintptr_t>(b+rva)-reinterpret_cast<std::uintptr_t>(m.BaseAddress);
        if(offset>m.RegionSize||4>m.RegionSize-offset)return false;
    }
    _InterlockedExchange8(reinterpret_cast<volatile char*>(b+0x94A78),1);
    const auto sequence=_InterlockedCompareExchange(reinterpret_cast<volatile long*>(b+0x949E0),0,0);
    _InterlockedExchange(reinterpret_cast<volatile long*>(b+0x94B9C),sequence);return true;
}
namespace {
template<class T>T read(const Globals& g,std::uint32_t rva) noexcept {T v{};std::memcpy(&v,g.data()+rva-globals_begin,sizeof(v));return v;}
bool known_cache(std::uint32_t c) noexcept{return c==0x94298||c==0x94398||c==0x94498||c==0x94598;}
}
bool exclusive_signatures(const void* image,std::size_t size) noexcept{
    if(!image||size<0x732000)return false;
    const auto* b=static_cast<const std::uint8_t*>(image);
    auto matches=[&](std::size_t off,std::initializer_list<std::uint8_t> bytes){return off<=size&&bytes.size()<=size-off&&std::memcmp(b+off,bytes.begin(),bytes.size())==0;};
    std::uint32_t pe=0;std::memcpy(&pe,b+0x3c,4);
    if(pe<0x40||pe>0x200||!matches(0,{0x4d,0x5a})||!matches(pe,{0x50,0x45,0,0})||!matches(pe+4,{0x64,0x86,15,0})||!matches(pe+20,{0xf0,0})||!matches(pe+24,{0x0b,0x02})||!matches(pe+24+56,{0,0x50,0x71,0}))return false;
    for(auto rva:{0xB273u,0x9A54u,0x9D34u,0x11249u})if(!matches(rva,{0x90,0x90,0x90,0x90,0x90}))return false;
    return matches(0x1112A,{0xe9,0xd1,0xee,0x6f,0,0x90})&&matches(0x1117C,{0xe9,0xbf,0xee,0x6f,0,0x90,0x90})&&
        matches(0x148E9,{0xe8,0x35,0x3e,0x70,0})&&matches(0x14BD9,{0xe8,0x5b,0x38,0x70,0})&&matches(0x1F97A,{0xe8,0x01,0xb7,0x6f,0})&&matches(0x6C63,{0xe8,0xc2,0xd8,0x70,0})&&matches(0x9BAC,{0xe8,0x5f,0xb7,0x70,0})&&
        matches(0x11AC0,{0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x56,0x57,0x53,0x48,0x81,0xec,0x58})&&
        matches(0x72D000,{0x48,0x89,0x4d,0x20,0x4c,0x89,0xc6,0xc7,0x45,0x28,0,0,0,0});
}
bool first_use_empty(const Globals& g) noexcept{
    for(auto r:{0x94850u,0x94858u,0x94860u,0x94868u})if(read<std::uint64_t>(g,r))return false;
    if(read<std::uint16_t>(g,0x94870)||read<std::uint64_t>(g,0x94B80)||read<std::uint8_t>(g,0x94B88))return false;

    for(auto rva:{0x949A8u,0x948E0u,0x949D0u,0x949C0u,0x94800u,0x94708u,0x94718u,0x94720u,0x94728u,0x94758u,0x94760u,0x94A00u,0x94A30u,0x94A38u,0x94A40u,0x94A48u,0x94A50u})if(read<std::uint64_t>(g,rva))return false;
    for(auto rva:{0x949B0u,0x948E8u,0x949DCu,0x94740u,0x94744u,0x94A58u,0x94A5Cu})if(read<std::uint32_t>(g,rva))return false;
    for(auto rva:{0x949B8u,0x94711u,0x948F5u,0x94B78u})if(read<std::uint8_t>(g,rva))return false;
    for(auto rva:{0x949BCu,0x949C8u,0x949D8u,0x949E4u,0x94A60u,0x94808u})if(read<std::int32_t>(g,rva)!=-1)return false;
    for(auto c:{0x94298u,0x94398u,0x94498u,0x94598u}){
        for(auto off:{0x00u,0x08u,0x10u,0x18u,0x40u,0x80u,0x88u,0x90u,0x98u,0xA0u,0xA8u,0xB0u,0xB8u,0xC0u,0xC8u,0xD0u,0xD8u,0xE0u,0xE8u,0xF0u,0xF8u})if(read<std::uint64_t>(g,c+off))return false;
        if(read<std::uint32_t>(g,c+0x30)||read<std::uint32_t>(g,c+0x34)||read<std::int32_t>(g,c+0x38)!=-1||read<std::uint8_t>(g,c+0x78))return false;
    }
    for(auto c:{0x946C0u,0x946E0u,0x948F8u,0x94918u,0x94938u,0x94958u,0x94A08u})for(unsigned i=0;i<4;++i)if(read<std::uint64_t>(g,c+8*i))return false;
    for(unsigned i=0;i<4;++i)if(read<std::uint32_t>(g,0x94A28+4*i))return false;
    return true;
}
Config configuration(const Globals& g) noexcept{
    Config c{};unsigned i=0;

    for(auto rva:{0x94A8Du,0x94A8Eu,0x94A8Fu,0x948F4u,0x94710u})c[i++]=read<std::uint8_t>(g,rva);
    for(auto rva:{0x94A90u,0x94840u,0x94AA0u,0x94AA4u,0x94AA8u,0x94AACu,0x94AB0u,0x94AB4u,0x94BA4u})c[i++]=read<std::uint32_t>(g,rva);
    return c;
}
bool cache_matches(const Globals& g,std::uint32_t c,const NrResourceDesc* d,bool require_copy) noexcept{
    if(!known_cache(c))return false;
    if(!d)return read<std::int32_t>(g,c+0x38)<0;
    if(!d->width||d->width>16384||!d->height||d->height>16384||read<std::int32_t>(g,c+0x38)<0||read<std::uint32_t>(g,c+0x3C)!=d->format)return false;
    const auto job=read<std::int32_t>(g,0x949DC);if(job<0||(require_copy&&job==0))return false;
    const unsigned slot=static_cast<unsigned>(job)&3u;

    unsigned width=static_cast<unsigned>(d->width),height=d->height;
    if(c==0x94398||c==0x94498){width=(std::max)(width,read<unsigned>(g,0x942C8));height=(std::max)(height,read<unsigned>(g,0x942CC));}
    if(width>16384||height>16384||read<unsigned>(g,c+0x30)!=width||read<unsigned>(g,c+0x34)!=height)return false;
    if(!require_copy&&job==0)return true;
    return read<unsigned>(g,c+0x48+4*slot)==d->width&&read<unsigned>(g,c+0x58+4*slot)==d->height;
}
}
