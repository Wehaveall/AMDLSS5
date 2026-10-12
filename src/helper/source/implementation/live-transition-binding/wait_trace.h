#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace nr::wait_trace {
enum class Event : std::uint32_t {
    CaptureRecorded, QueueBegin, QueueEnd, HipComplete, FenceSignal,
    FenceComplete, CpuWaitBegin, CpuWaitEnd, Failure,
    CaptureStateSkipped, CaptureStateRestored
};
struct Entry {
    std::int64_t qpc = 0;
    std::uint64_t job = 0, queue = 0, fence = 0, value = 0;
    std::uint32_t thread = 0, flags = 0;
    Event event = Event::Failure;
};
struct Slot { std::atomic<bool> published{false}; Entry entry{}; };

class Buffer {
public:
    Buffer(Slot* slots, std::size_t capacity) noexcept : slots_(slots), capacity_(capacity) {}
    void Record(Event event, std::uint64_t job = 0, std::uint64_t queue = 0,
                std::uint64_t fence = 0, std::uint64_t value = 0,
                std::uint32_t flags = 0) noexcept;
    bool Read(std::size_t index, Entry& output) const noexcept;
    std::uint64_t Attempted() const noexcept { return attempted_.load(std::memory_order_relaxed); }
    std::uint64_t Dropped() const noexcept { const auto n = Attempted(); return n > capacity_ ? n - capacity_ : 0; }
    std::size_t Capacity() const noexcept { return capacity_; }
private:
    Slot* slots_;
    std::size_t capacity_;
    std::atomic<std::uint64_t> attempted_{0};
};
constexpr std::size_t kCapacity = 65536;

void InitializeFromEnvironment() noexcept;
bool Enabled() noexcept;
void Record(Event event, std::uint64_t job = 0, std::uint64_t queue = 0,
            std::uint64_t fence = 0, std::uint64_t value = 0,
            std::uint32_t flags = 0) noexcept;
struct Stats { std::uint64_t attempted, dropped; };
Stats GetStats() noexcept;

bool Flush(const wchar_t* path) noexcept;
}
