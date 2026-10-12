#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <limits>
#include <intrin.h>
#include "native_ini_hook.h"

namespace nrini {
namespace {
std::recursive_mutex reload_mutex;
std::mutex installation_mutex;
std::atomic<unsigned char*> native_base{};
std::atomic<void*> completed_base{};
bool attempted{};

void* trampoline{};
constexpr std::size_t call_rva=0x72d484;
constexpr unsigned char expected[]{0xe8,0x07,0xa7,0x8d,0xff};
constexpr unsigned char expected_block[]{0x48,0x83,0xec,0x28,0xe8,0x07,0xa7,0x8d,
    0xff,0x48,0x89,0x44,0x24,0x20,0xe8,0x6d};

std::uintptr_t __cdecl reload_serialized() {
    std::lock_guard<std::recursive_mutex> guard(reload_mutex);
    using Reload=std::uintptr_t(__cdecl*)();
    return reinterpret_cast<Reload>(native_base.load(std::memory_order_acquire)+0x7b90)();
}

bool read_bytes(const void* source,void* target,SIZE_T bytes) noexcept {
    SIZE_T copied{};
    return ReadProcessMemory(GetCurrentProcess(),source,target,bytes,&copied)&&copied==bytes;
}
}

std::recursive_mutex& mutex() noexcept {return reload_mutex;}

bool reler_agora(void* address,unsigned inline_ini) noexcept {
    try{
        if(!address||inline_ini>1)return false;
        auto* b=static_cast<unsigned char*>(address);
        static constexpr unsigned char prologo[]{0x55,0x41,0x56,0x56,0x57,0x53,0x48,0x81,0xec,0xa0,0x00,0x00,0x00};
        unsigned char atual[sizeof(prologo)]{};
        if(!read_bytes(b+0x7b90,atual,sizeof(atual))||std::memcmp(atual,prologo,sizeof(prologo)))return false;
        std::lock_guard<std::recursive_mutex> guard(reload_mutex);
        std::uint64_t a{},c{},e{};unsigned d{};int bc{};unsigned char lido{};
        if(!read_bytes(b+0x949A8,&a,8)||!read_bytes(b+0x948E0,&c,8)||!read_bytes(b+0x949DC,&d,4)||
           !read_bytes(b+0x949BC,&bc,4)||!read_bytes(b+0x949D0,&e,8)||!read_bytes(b+0x94BD0,&lido,1))return false;
        if(a||c||d||bc!=-1||e||lido!=1)return false;
        _InterlockedExchange8(reinterpret_cast<volatile char*>(b+0x94710),static_cast<char>(inline_ini));
        using Reload=std::uintptr_t(__cdecl*)();
        (void)reinterpret_cast<Reload>(b+0x7b90)();
        return true;
    }catch(...){return false;}
}

bool install(void* address) noexcept {
    if(!address)return false;
    if(completed_base.load(std::memory_order_acquire)==address)return true;
    try {
        std::lock_guard<std::mutex> guard(installation_mutex);
        if(attempted)return completed_base.load(std::memory_order_acquire)==address;
        attempted=true;
        auto* base=static_cast<unsigned char*>(address);
        const auto base_value=reinterpret_cast<std::uintptr_t>(address);
        if(base_value>UINTPTR_MAX-call_rva-sizeof(expected))return false;
        auto* site=base+call_rva;
        auto* block=site-4;
        unsigned char actual[sizeof(expected_block)]{};
        MEMORY_BASIC_INFORMATION region{};
        if(!VirtualQuery(site,&region,sizeof(region))||region.State!=MEM_COMMIT||
           region.AllocationBase!=address||
           (region.Protect&(PAGE_GUARD|PAGE_NOACCESS))||
           !read_bytes(block,actual,sizeof(actual))||std::memcmp(actual,expected_block,sizeof(expected_block))||
           (reinterpret_cast<std::uintptr_t>(block)&15)||!IsProcessorFeaturePresent(PF_COMPARE_EXCHANGE128))return false;

#ifndef NRINI_CPU_TEST
        if(region.Type!=MEM_IMAGE)return false;
        HMODULE original{},helper{};
        constexpr DWORD pin_flags=GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN;
        if(!GetModuleHandleExW(pin_flags,reinterpret_cast<LPCWSTR>(address),&original)||
           original!=static_cast<HMODULE>(address)||
           !GetModuleHandleExW(pin_flags,reinterpret_cast<LPCWSTR>(&reload_serialized),&helper))return false;
#endif

        SYSTEM_INFO info{};GetSystemInfo(&info);
        const std::uintptr_t gran=info.dwAllocationGranularity;
        if(!gran||(gran&(gran-1)))return false;
        const auto site_value=reinterpret_cast<std::uintptr_t>(site);
        const auto center=site_value&~(gran-1);
        unsigned char* memory{};
        for(std::uintptr_t delta=gran;delta<0x70000000&&!memory;delta+=gran){
            if(center>delta)memory=static_cast<unsigned char*>(VirtualAlloc(
                reinterpret_cast<void*>(center-delta),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
            if(!memory&&center<=UINTPTR_MAX-delta)memory=static_cast<unsigned char*>(VirtualAlloc(
                reinterpret_cast<void*>(center+delta),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        }
        if(!memory)return false;

        unsigned char code[14]{0xff,0x25,0,0,0,0};
        const auto target=reinterpret_cast<std::uintptr_t>(&reload_serialized);
        std::memcpy(code+6,&target,sizeof(target));
        std::memcpy(memory,code,sizeof(code));
        const auto distance=static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(memory))-
            static_cast<std::int64_t>(site_value+sizeof(expected));
        if(distance<(std::numeric_limits<std::int32_t>::min)()||
           distance>(std::numeric_limits<std::int32_t>::max)()){
            VirtualFree(memory,0,MEM_RELEASE);return false;
        }
        DWORD old{};
        if(!VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old)||
           !FlushInstructionCache(GetCurrentProcess(),memory,sizeof(code))){
            VirtualFree(memory,0,MEM_RELEASE);return false;
        }
        alignas(16) long long replacement[2]{},comparison[2]{};
        std::memcpy(replacement,expected_block,sizeof(expected_block));
        std::memcpy(comparison,expected_block,sizeof(expected_block));
        const auto relative=static_cast<std::int32_t>(distance);
        std::memcpy(reinterpret_cast<unsigned char*>(replacement)+5,&relative,sizeof(relative));
        if(!VirtualProtect(block,sizeof(expected_block),PAGE_EXECUTE_READWRITE,&old)){
            VirtualFree(memory,0,MEM_RELEASE);return false;
        }

        native_base.store(base,std::memory_order_release);trampoline=memory;
        const bool patched=_InterlockedCompareExchange128(reinterpret_cast<volatile long long*>(block),
            replacement[1],replacement[0],comparison)!=0;
        DWORD ignored{};
        const bool restored=VirtualProtect(block,sizeof(expected_block),old,&ignored)!=0;
        const bool flushed=FlushInstructionCache(GetCurrentProcess(),block,sizeof(expected_block))!=0;
        if(!patched||!restored||!flushed)return false;
        completed_base.store(address,std::memory_order_release);
        return true;
    }catch(...){return false;}
}
}
