#pragma once
#include <windows.h>
#include <cstdint>
#include <limits>
#include <memory>

namespace nrtransition {
using HipSetDevice = int (__cdecl*)(int);
using HipSynchronize = int (__cdecl*)();
struct HipApi {
    HipSetDevice set_device{};
    HipSynchronize synchronize{};
    int device{-1};
    std::uint64_t generation{};
};
enum class HipState { Pending, Succeeded, Failed };
struct HipResult {
    HipState state{HipState::Failed};
    int device{-1};
    std::uint64_t generation{};
    int set_device_status{std::numeric_limits<int>::min()};
    int synchronize_status{std::numeric_limits<int>::min()};
    DWORD host_error{ERROR_INVALID_PARAMETER};
};

class HipProof final {
public:
    explicit HipProof(HipApi api) noexcept;
    ~HipProof() = default;
    HipProof(const HipProof&) = delete;
    HipProof& operator=(const HipProof&) = delete;
    HipProof(HipProof&&) = delete;
    HipProof& operator=(HipProof&&) = delete;
    HipResult poll() const noexcept;
private:
    struct Shared;
    std::shared_ptr<Shared> state_;
    static unsigned __stdcall run(void* payload) noexcept;
};
}
