#pragma once
#include <memory>
#include <cstdint>
namespace nrstate {

bool observe(void* command) noexcept;

const char* observe_refusal() noexcept;
void* observed_table() noexcept;

std::size_t private_table_count() noexcept;
void reset(void* command,void* initial_pipeline) noexcept;

bool tracked(void* command) noexcept;
bool forget(void* command) noexcept;
std::size_t count() noexcept;
class Saved {
public:
    virtual ~Saved()=default;
    virtual bool suspend_predication() noexcept=0;
    virtual bool restore() noexcept=0;
};
std::unique_ptr<Saved> save(void* command) noexcept;

struct DispatchState {
    void* root{}; void* pipeline{};
    std::uint64_t uav0{},uav2{};
    std::uint32_t constants[4]{};
    bool complete{};
};
using DispatchInterceptor=bool(*)(void* command,const DispatchState&,unsigned x,unsigned y,unsigned z) noexcept;
bool set_dispatch_interceptor(DispatchInterceptor) noexcept;

std::unique_ptr<Saved> save_graphics(void* command) noexcept;
std::uint32_t readiness(void* command) noexcept;
}
