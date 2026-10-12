#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string>
namespace nrpresentnative {
struct Controls {
    float structure=1,skin=1,tone=0;
    unsigned style=0;
    bool auto_mask=true,tone_channels=true;
    float strength=1;
    unsigned passes=1;
};
bool validate(unsigned width,unsigned height,const Controls&,std::string& error) noexcept;
std::array<float,5> parameters(const Controls&) noexcept;
class Engine {
public:
    Engine();
    ~Engine();
    Engine(const Engine&)=delete;
    Engine& operator=(const Engine&)=delete;
    void set_adapter_luid(std::uint64_t) noexcept;
    bool evaluate(void* proxy_base,const std::uint16_t* rgba,std::uint16_t* output,
                  unsigned width,unsigned height,const Controls&,std::string& error) noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
