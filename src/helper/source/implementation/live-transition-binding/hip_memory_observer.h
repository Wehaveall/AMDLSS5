#pragma once
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstddef>
#include <memory>
namespace nrmem {
namespace detail {
struct PatchOps {void* context;bool (*protect)(void*,bool restore) noexcept;void* (*exchange)(void*,unsigned,void*,void*) noexcept;};
struct PatchResult {bool installed{},restored{},rollback{true};};
PatchResult patch(PatchOps,void* const* originals,void* const* hooks,unsigned count) noexcept;
}
enum class Operation:unsigned { Malloc,Free,Import,Map,Destroy };
struct Event {std::uint64_t sequence{},time{},pointer{},related{},bytes{},caller{};unsigned thread{},operation{};int result{};};
struct Summary {
    std::array<std::uint64_t,5> success{},failure{};
    std::uint64_t live_malloc_bytes{},live_mapped_bytes{},live_import_bytes{},freed_malloc_bytes{},freed_mapped_bytes{},destroyed_import_bytes{};
    std::uint64_t unknown_free{},unknown_destroy{},duplicate_pointer{},ledger_overflow{},events_dropped{},unreadable{},sequence{};
};
class Ledger {
public:
    static constexpr std::size_t capacity=16384,event_capacity=32768;
    void record(Operation,std::uintptr_t pointer,std::uintptr_t related,std::uint64_t bytes,int result,unsigned thread,std::uintptr_t caller) noexcept;
    void unreadable() noexcept;
    Summary summary() noexcept;
    std::size_t drain(Event*,std::size_t) noexcept;
private:
    struct Allocation {std::uintptr_t pointer{};std::uint64_t bytes{};unsigned kind{},state{};};
    SRWLOCK lock_=SRWLOCK_INIT;
    std::array<Allocation,capacity> allocations_{};
    std::array<Event,event_capacity> events_{};
    std::size_t event_read_{},event_count_{};
    Summary summary_{};
};
using Allocate=int(__cdecl*)(void**,std::size_t);
using Release=int(__cdecl*)(void*);
using Import=int(__cdecl*)(void**,const void*);
using Map=int(__cdecl*)(void**,void*,const void*);
struct Calls {Allocate allocate{};Release free{};Import import_memory{};Map map{};Release destroy{};};
class Forwarder {
public:
    Forwarder(Ledger& ledger,Calls calls):ledger_(ledger),calls_(calls){}
    int allocate(void**,std::size_t,unsigned,std::uintptr_t) noexcept;
    int free(void*,unsigned,std::uintptr_t) noexcept;
    int import_memory(void**,const void*,unsigned,std::uintptr_t) noexcept;
    int map(void**,void*,const void*,unsigned,std::uintptr_t) noexcept;
    int destroy(void*,unsigned,std::uintptr_t) noexcept;
private:
    Ledger& ledger_;Calls calls_;
    std::uint64_t read(const void*,std::size_t) noexcept;
};

bool install_if_requested(void* original_base) noexcept;
bool validate(void* original_base) noexcept;
void flush(bool force=false) noexcept;
}
