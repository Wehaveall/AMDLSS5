#pragma once
#include "host_menu.h"
#include "settings.h"
namespace nradapter {
enum class AppendResult { Appended, AlreadyPresent, NotReady, WrongThread, Busy, InvalidHost, Failed };

AppendResult append_menu(const NativeHost& host) noexcept;

AppendResult append_from_base(void* base) noexcept;

enum class GatewayStatus { Ok, NotReady, Ambiguous, WrongThread, Busy, Stale, Failed };
struct GatewayResult {
    GatewayStatus status;
    std::optional<nr::Snapshot> state;
};
GatewayResult runtime_snapshot(void* registered_base) noexcept;
GatewayResult runtime_reload(void* registered_base) noexcept;
GatewayResult runtime_request_scale(void* registered_base,unsigned percent) noexcept;

GatewayResult runtime_acknowledge(void* registered_base,std::uint64_t generation) noexcept;
GatewayResult runtime_fail(void* registered_base,std::uint64_t generation,const std::string& diagnostic) noexcept;
}
extern "C" __declspec(dllexport) void NRMenuAppend_v1(void* base) noexcept;
