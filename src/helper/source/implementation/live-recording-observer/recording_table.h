#pragma once
#include "recording_observer.h"
#include "compute_state.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
namespace nrrecord {

class RecordingTable {
public:
    static constexpr std::size_t cap=nrstate::teto_listas;

    struct Result {
        Observer* observer{};
        const char* stage{};
        const char* reason{};
        bool installed{};
        unsigned evicted{};
        bool rearmada{};
    };
    explicit RecordingTable(bool despejar=true) noexcept:despejar_(despejar){}

    enum class Origem{Fsr,Reset};
    void intervalo_reset_ms(std::uint64_t ms) noexcept{std::lock_guard<std::mutex> l(mutex_);intervalo_reset_ms_=ms;}

    static constexpr std::uint64_t teto_instalados_reset_padrao=8u*cap;
    void teto_instalados_reset(std::uint64_t n) noexcept{std::lock_guard<std::mutex> l(mutex_);teto_instalados_reset_=n;}
    std::uint64_t instalados() const noexcept{std::lock_guard<std::mutex> l(mutex_);return instalados_;}
    std::uint64_t rearmadas() const noexcept{std::lock_guard<std::mutex> l(mutex_);return rearmadas_;}
    Result find_or_install(void* command,std::uint64_t device,std::uint32_t owner,Origem origem=Origem::Fsr) noexcept;

    void in_use(void* const* commands,unsigned count) noexcept;
    std::size_t size() const noexcept{std::lock_guard<std::mutex> l(mutex_);return entries_.size();}
    std::uint64_t evicted() const noexcept{std::lock_guard<std::mutex> l(mutex_);return evicted_;}
private:
    unsigned evict_dead() noexcept;
    unsigned evict_lru(bool so_sem_fsr) noexcept;
    bool despejar_;
    mutable std::mutex mutex_;
    std::uint64_t intervalo_reset_ms_=2000,ultimo_despejo_reset_{};
    std::unordered_set<void*> recusadas_reset_;

    struct Item{std::unique_ptr<Observer> observer;std::uint64_t uso{};bool fsr{};};
    std::unordered_map<void*,Item> entries_;
    std::uint64_t teto_instalados_reset_=teto_instalados_reset_padrao,instalados_{},rearmadas_{};
    bool rearmar_falha(void* command,Item& item) noexcept;

    std::vector<std::unique_ptr<Observer>> graveyard_;
    std::array<void*,16> in_use_{};unsigned in_use_n_{};
    std::uint64_t evicted_{};
};
}
