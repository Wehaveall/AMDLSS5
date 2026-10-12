#pragma once
#include "recording_observer.h"
namespace nrrecord::detail {
using Close=std::int32_t(*)(void*);
using Reset=std::int32_t(*)(void*,void*,void*);
struct Targets { void** table{};Close close{};Reset reset{}; };
bool table_protection_matches(std::uint32_t initial,std::uint32_t actual,bool checked_write_restore) noexcept;

enum class TableRegion { Refused, Image, Private };
TableRegion classify_table_region(std::uint32_t state,std::uint32_t type,std::uint32_t protect,bool hooked_slots_in_d3d12) noexcept;

bool table_inside_object(std::uintptr_t object,std::uintptr_t table) noexcept;

bool same_private_offset(std::uintptr_t object,std::uintptr_t table,std::uintptr_t offset) noexcept;

bool d3d12_runtime_or_driver_code(const void* address) noexcept;

class Platform {
public:
    virtual ~Platform()=default;
    virtual bool prepare(void* command,std::uint64_t device,std::uint32_t owner,Targets&) noexcept=0;
    virtual std::uint32_t thread() noexcept=0;
    virtual bool writable(void** table,std::uint32_t& previous) noexcept=0;
    virtual bool restore(void** table,std::uint32_t previous) noexcept=0;
    virtual bool exchange(void** slot,void* expected,void* desired) noexcept=0;
    virtual bool intact(void* command,void** table,void* close,void* reset) noexcept=0;
    virtual const char* diagnostic() const noexcept{return "platform_detail_unavailable";}

    virtual bool private_table() const noexcept{return false;}

    virtual const char* drift_reason(void* command,void** table) noexcept{(void)command;(void)table;return nullptr;}

    virtual void release() noexcept{}
};

std::unique_ptr<Observer> install(std::unique_ptr<Platform>,void* command,std::uint64_t device,std::uint32_t owner) noexcept;

bool refuse(const char* why) noexcept;
}
