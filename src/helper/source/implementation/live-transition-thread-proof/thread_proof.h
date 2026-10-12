#pragma once
#include <windows.h>
#include <cstdint>

namespace nrtransition {
struct ThreadPairSnapshot {
    HANDLE worker;
    DWORD worker_id;
    HANDLE watchdog;
    DWORD watchdog_id;
    std::uint64_t generation;
    bool watchdog_required = true;
};
enum class ThreadPoll { Pending, Terminated, Failed };
class ThreadProof final {
public:

    explicit ThreadProof(const ThreadPairSnapshot&) noexcept;
    ~ThreadProof() noexcept;
    ThreadProof(const ThreadProof&) = delete;
    ThreadProof& operator=(const ThreadProof&) = delete;
    bool valid() const noexcept;
    DWORD last_error() const noexcept;
    bool matches(const ThreadPairSnapshot&) const noexcept;
    ThreadPoll poll() noexcept;
private:
    HANDLE worker_ = nullptr;
    HANDLE watchdog_ = nullptr;
    DWORD worker_id_ = 0;
    DWORD watchdog_id_ = 0;
    std::uint64_t generation_ = 0;
    bool watchdog_required_ = true;
    DWORD error_ = ERROR_INVALID_DATA;
};
}
