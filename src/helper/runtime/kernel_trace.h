#pragma once

#include <cstdint>
#include <vector>

namespace nrkerneltrace {

struct Launch {
    std::uintptr_t function{};
    unsigned grid_x{},grid_y{},grid_z{};
    unsigned block_x{},block_y{},block_z{};
    std::size_t shared_bytes{};
};

struct Record {
    int job{};
    std::uintptr_t function{};
    unsigned grid_x{},grid_y{},grid_z{};
    unsigned block_x{},block_y{},block_z{};
    std::size_t shared_bytes{};
    std::uint64_t begin_tick{},end_tick{};
};

class Recorder {
    bool enabled_{};
    int job_{};
    std::uint64_t finish_{};
    std::vector<Record> records_;
public:
    explicit Recorder(bool enabled) noexcept:enabled_(enabled){}
    void begin(int job,std::uint64_t) noexcept {if(enabled_&&job>0&&!job_)job_=job;}
    void observe(const Launch& launch,std::uint64_t tick) {
        if(enabled_&&job_&&launch.function)records_.push_back({job_,launch.function,launch.grid_x,launch.grid_y,launch.grid_z,launch.block_x,launch.block_y,launch.block_z,launch.shared_bytes,tick,0});
    }
    void end(std::uint64_t tick) noexcept {if(!job_)return;finish_=tick;for(auto& record:records_)if(!record.end_tick)record.end_tick=finish_;job_=0;}
    std::vector<Record> take(){auto result=std::move(records_);records_.clear();return result;}
};

}
