#pragma once
#include <cstdint>
#include <memory>

namespace nrfence {
using Value = std::uint64_t;
struct Identity { std::uintptr_t device{}; std::uintptr_t queue{}; };
enum class Status { Ready, Pending, Completed, Failed };
struct Result { Status status{Status::Failed}; Value generation{}; Value target{}; Identity identity{}; };

class Backend {
public:
    virtual ~Backend() = default;
    virtual bool initialize() noexcept = 0;
    virtual Identity identity() const noexcept = 0;
    virtual bool healthy() noexcept = 0;
    virtual bool signal(Value target) noexcept = 0;
    virtual Value completed_value() noexcept = 0;

    virtual bool arm_event(Value target, void* event) noexcept { (void)target; (void)event; return false; }

    virtual long removed_reason() noexcept { return 0; }
};
class OwnedFence {
public:
    OwnedFence(std::unique_ptr<Backend> backend, Value generation) noexcept;
    ~OwnedFence();
    OwnedFence(const OwnedFence&) = delete;
    OwnedFence& operator=(const OwnedFence&) = delete;
    OwnedFence(OwnedFence&&) = delete;
    OwnedFence& operator=(OwnedFence&&) = delete;
    Result signal(Value target) noexcept;
    Result poll() noexcept;

    bool arm(void* event) noexcept;
    Result snapshot() const noexcept;
    const char* failure() const noexcept;

    Value last_read() const noexcept { return last_read_; }
    long removed_reason() noexcept;
private:
    std::unique_ptr<Backend> backend_;
    Result result_;
    Value last_observed_{},last_read_{};
    const char* failure_{""};
    bool validate() noexcept;
    Result fail(const char* reason) noexcept;
};
}
