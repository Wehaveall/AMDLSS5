#pragma once
#include <cstddef>
#include <cstdint>

struct NRLifecycleCallbacks {
    std::uint64_t (*queue_begin)(void* queue, std::uint32_t count, void** lists) noexcept;
    void (*queue_end)(std::uint64_t ticket, void* queue, std::uint32_t count, void** lists) noexcept;
    void (*hip_complete)(std::int32_t status, std::int32_t job) noexcept;
    std::uint32_t (*reset_gate)(std::uint32_t kind) noexcept;
};
static_assert(sizeof(void*) == 8);
static_assert(sizeof(NRLifecycleCallbacks) == 32);
static_assert(offsetof(NRLifecycleCallbacks, queue_begin) == 0);
static_assert(offsetof(NRLifecycleCallbacks, queue_end) == 8);
static_assert(offsetof(NRLifecycleCallbacks, hip_complete) == 16);
static_assert(offsetof(NRLifecycleCallbacks, reset_gate) == 24);
inline constexpr std::uintptr_t NR_LIFECYCLE_CALLBACK_RVA = 0x72CFF0;
