#pragma once
#include <cstdint>
#include <cstddef>
namespace nrhost {
enum class Status : std::uint32_t { Pending, Ready, Failed, DisabledReady };
struct Request { std::uint32_t size=sizeof(Request),version=1,percent=100,stage=1;std::uint64_t request_id{},current_generation{}; };
struct Ticket {
    std::uint32_t size=sizeof(Ticket),version=1,percent=100,stage=1;
    std::uint64_t request_id{},old_generation{},serial{};
    std::uint32_t scene_width{},scene_height{},working_width{},working_height{},final_width{},final_height{};
};
struct Resources { std::uint32_t size=sizeof(Resources),version=1;void* context{};void* input{};void* output{};void* motion{};void* depth{}; };
struct Api {
    std::uint32_t size=sizeof(Api),version=1;
    Status (*request)(Request*) noexcept;
    Status (*set_percent)(std::uint32_t) noexcept;
    Status (*prepare)(Ticket*) noexcept;
    Status (*install)(const Ticket*,const Resources*) noexcept;
    Status (*poll)(std::uint64_t) noexcept;
    Status (*cancel)(std::uint64_t) noexcept;
};
using GetApi=const Api*(*)() noexcept;
static_assert(sizeof(Request)==32&&sizeof(Ticket)==64&&sizeof(Resources)==48&&sizeof(Api)==56);
static_assert(offsetof(Request,current_generation)==24);
static_assert(offsetof(Api,request)==8&&offsetof(Ticket,request_id)==16);
}
