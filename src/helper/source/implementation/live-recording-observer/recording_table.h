#pragma once
#include "recording_observer.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
namespace nrrecord {

class RecordingTable {
public:
    static constexpr std::size_t cap=64;
    struct Result {
        Observer* observer{};
        const char* stage{};
        const char* reason{};
        bool installed{};
        unsigned evicted{};
    };
    explicit RecordingTable(bool despejar=true) noexcept:despejar_(despejar){}
    Result find_or_install(void* command,std::uint64_t device,std::uint32_t owner) noexcept;

    void in_use(void* const* commands,unsigned count) noexcept;
    std::size_t size() const noexcept{return entries_.size();}
    std::uint64_t evicted() const noexcept{return evicted_;}
private:
    unsigned evict_dead() noexcept;
    bool despejar_;
    std::vector<std::pair<void*,std::unique_ptr<Observer>>> entries_;

    std::vector<std::unique_ptr<Observer>> graveyard_;
    std::array<void*,16> in_use_{};unsigned in_use_n_{};
    std::uint64_t evicted_{};
};
}
