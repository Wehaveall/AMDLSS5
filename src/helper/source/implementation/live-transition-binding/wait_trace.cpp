#include "../../../runtime/nr_publico.h"
#include "wait_trace.h"
#include <Windows.h>
#include <cstdio>
namespace nr::wait_trace {
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
static_assert(std::atomic<bool>::is_always_lock_free);
namespace {
Slot storage[kCapacity];
Buffer buffer(storage, kCapacity);
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
    if (Enabled()) buffer.Record(event, job, queue, fence, value, flags);
}
Stats GetStats() noexcept {
    const auto count = buffer.Attempted();
    return {count, count > kCapacity ? count - kCapacity : 0};
}
bool Flush(const wchar_t* path) noexcept {
    if (!path || !*path) return false;
    AcquireSRWLockExclusive(&flushLock);
    FILE* file = nullptr;
    bool ok = _wfopen_s(&file, path, L"ab+") == 0 && file;
    auto next = flushed;
    if (ok) {
        LARGE_INTEGER frequency{};
        QueryPerformanceFrequency(&frequency);
        ok = _fseeki64(file, 0, SEEK_END) == 0;
        if (ok && _ftelli64(file) == 0)
            ok = std::fprintf(file, "index,qpc,qpc_frequency,thread,event,job,queue,fence,value,flags\n") > 0;
        Entry entry;

        while (ok && buffer.Read(next, entry)) {
            ok = std::fprintf(file, "%zu,%lld,%lld,%u,%s,%llu,%llu,%llu,%llu,%u\n",
                next, static_cast<long long>(entry.qpc), static_cast<long long>(frequency.QuadPart),
                entry.thread, Name(entry.event), static_cast<unsigned long long>(entry.job),
                static_cast<unsigned long long>(entry.queue), static_cast<unsigned long long>(entry.fence),
                static_cast<unsigned long long>(entry.value), entry.flags) > 0;
            if (ok) ++next;
        }
        if (ok) {
            const auto stats = GetStats();
            ok = std::fprintf(file, "# attempted=%llu,dropped=%llu,published_prefix=%zu\n",
                static_cast<unsigned long long>(stats.attempted), static_cast<unsigned long long>(stats.dropped), next) > 0;
        }
        if (std::fflush(file) != 0) ok = false;
        if (std::fclose(file) != 0) ok = false;
        if (ok) flushed = next;

    }
    ReleaseSRWLockExclusive(&flushLock);
    return ok;
}
}
