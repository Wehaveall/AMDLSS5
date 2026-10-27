#include "../../../runtime/nr_publico.h"
#include "wait_trace.h"
#include <Windows.h>
#include <cstdio>
#include <new>
namespace nr::wait_trace {
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
static_assert(std::atomic<bool>::is_always_lock_free);
namespace {

std::atomic<Buffer*> buffer{nullptr};
std::atomic<bool> enabled{false};
SRWLOCK flushLock = SRWLOCK_INIT;
std::size_t flushed = 0;
const char* Name(Event event) noexcept {
    switch (event) {
    case Event::CaptureRecorded: return "CaptureRecorded";
    case Event::QueueBegin: return "QueueBegin";
    case Event::QueueEnd: return "QueueEnd";
    case Event::HipComplete: return "HipComplete";
    case Event::FenceSignal: return "FenceSignal";
    case Event::FenceComplete: return "FenceComplete";
    case Event::CpuWaitBegin: return "CpuWaitBegin";
    case Event::CpuWaitEnd: return "CpuWaitEnd";
    case Event::Failure: return "Failure";
    case Event::CaptureStateSkipped: return "CaptureStateSkipped";
    case Event::CaptureStateRestored: return "CaptureStateRestored";
    default: return "Unknown";
    }
}
}
void Buffer::Record(Event event, std::uint64_t job, std::uint64_t queue,
                    std::uint64_t fence, std::uint64_t value, std::uint32_t flags) noexcept {

    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    const auto index = attempted_.fetch_add(1, std::memory_order_relaxed);
    if (index >= capacity_) return;
    auto& slot = slots_[static_cast<std::size_t>(index)];
    slot.entry = Entry{now.QuadPart, job, queue, fence, value,
                       GetCurrentThreadId(), flags, event};
    slot.published.store(true, std::memory_order_release);
}
bool Buffer::Read(std::size_t index, Entry& output) const noexcept {
    if (index >= capacity_ || !slots_[index].published.load(std::memory_order_acquire)) return false;
    output = slots_[index].entry;
    return true;
}
void InitializeFromEnvironment() noexcept {
}
bool Enabled() noexcept { return enabled.load(std::memory_order_relaxed); }
void Record(Event event, std::uint64_t job, std::uint64_t queue,
            std::uint64_t fence, std::uint64_t value, std::uint32_t flags) noexcept {
    if (!Enabled()) return;
    if (Buffer* b = buffer.load(std::memory_order_acquire)) b->Record(event, job, queue, fence, value, flags);
}
Stats GetStats() noexcept {
    const Buffer* b = buffer.load(std::memory_order_acquire);
    const auto count = b ? b->Attempted() : 0;
    return {count, count > kCapacity ? count - kCapacity : 0};
}
bool Flush(const wchar_t* path) noexcept {
    (void)path; (void)flushLock; (void)flushed;
    return false;
}
}
