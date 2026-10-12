#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace nrmptiming {
inline constexpr std::size_t capacity=128;
enum class Route {not_reached,direct_primary,prescale_unattributed};
struct Sample {
    std::int64_t wrapper_wall_ns{},primary_blocking_wall_ns{},remainder_host_wall_ns{};
    int width{},height{},input_pitch{},output_pitch{},input_format{},output_format{},tone{};
    unsigned request{},effective_config{};
    bool metadata_present{},request_known{},effective_config_known{},primary_returned{},wrapper_threw{};
    Route route=Route::not_reached;
};
struct Snapshot {std::array<Sample,capacity> samples{};std::size_t count{};};
void arm_current_thread(std::size_t limit) noexcept;
Snapshot snapshot_current_thread() noexcept;
}

extern "C" bool NRMultipassInstall_v1(void* proxy_base) noexcept;

extern "C" __declspec(dllexport) void NRMultipassEval_v1(void* context,void* source,int input_pitch,int input_format,
    int height,int width,void* destination,int output_pitch,int output_format,int tone,void* metadata) noexcept(false);
