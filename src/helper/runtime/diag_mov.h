#pragma once

#include <cstdint>
#include <cstring>

namespace nrdiagmov {
inline constexpr std::uint32_t rva_cmp=0x19155;
inline constexpr std::uint8_t esperado[10]{0x41,0x83,0xFC,0x32,0x0F,0x85,0xA3,0xE4,0xFF,0xFF};
inline constexpr std::uint8_t nunca=0x7F;

inline int estado(const std::uint8_t* lugar) noexcept {
    if(!lugar)return 0;
    if(std::memcmp(lugar,esperado,sizeof esperado)==0)return 1;
    std::uint8_t d[sizeof esperado];std::memcpy(d,esperado,sizeof d);d[3]=nunca;
    return std::memcmp(lugar,d,sizeof d)==0?2:0;
}
}

#if defined(_WIN32) && !defined(NRDIAGMOV_CPU_TEST)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <intrin.h>
namespace nrdiagmov {

inline bool desligar(void* base) noexcept {
    if(!base)return false;
    auto* lugar=static_cast<std::uint8_t*>(base)+rva_cmp;
    MEMORY_BASIC_INFORMATION r{};
    if(!VirtualQuery(lugar,&r,sizeof r)||r.State!=MEM_COMMIT||r.Type!=MEM_IMAGE||r.AllocationBase!=base||(r.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    const int e=estado(lugar);
    if(e==2)return true;
    if(e!=1)return false;
    DWORD velho{};
    if(!VirtualProtect(lugar+3,1,PAGE_EXECUTE_READWRITE,&velho))return false;
    _InterlockedCompareExchange8(reinterpret_cast<volatile char*>(lugar+3),static_cast<char>(nunca),static_cast<char>(esperado[3]));
    DWORD ignorado{};VirtualProtect(lugar+3,1,velho,&ignorado);
    FlushInstructionCache(GetCurrentProcess(),lugar,sizeof esperado);
    return estado(lugar)==2;
}
}
#endif
