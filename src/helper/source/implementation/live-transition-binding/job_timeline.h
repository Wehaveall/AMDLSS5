#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace nrjob {

enum Stamp : unsigned { Capturado, Submetido, EvalInicio, EvalFim, Publicado, Notado, PrimeiroUso,
                        GpuAntesBegin, GpuAntesNativa, GpuFimCopia, GpuDepoisNativa, CercaPronta, StampCount };
const char* name(Stamp s) noexcept;

std::int64_t qpc() noexcept;
std::int64_t qpc_frequency() noexcept;

struct Tuning { unsigned short_wait_ms{}, exclusive_ms{}; unsigned age_log_frames{1024}; };

constexpr unsigned age_log_frames_default = 1024, age_log_frames_min = 128, age_log_frames_max = 4096;
constexpr unsigned age_log_frames(long long ini_value) noexcept {
    return ini_value <= 0 ? age_log_frames_default
         : ini_value < age_log_frames_min ? age_log_frames_min
         : ini_value > age_log_frames_max ? age_log_frames_max : static_cast<unsigned>(ini_value);
}

struct WaitStats {
    unsigned short_waits{}, short_early{}, short_timeouts{};
    double short_ms{};
    unsigned window_waits{}, window_early{};
    double window_ms{};
};

struct LateTotals {
    std::uint64_t armed{}, recorded{}, resolved{}, executed{}, refused{}, b_failed{}, inconsistent{};
    double gain_frames{}, gain_ms{};
    bool desligada{};
};

class Timeline {
public:
    explicit Timeline(std::int64_t frequency) noexcept : frequency_(frequency) {}
    void stamp(int job, Stamp s, std::int64_t t) noexcept;
    std::int64_t when(int job, Stamp s) const noexcept;

    std::int64_t eval_ema_us() const noexcept { return eval_ema_us_.load(std::memory_order_relaxed); }
    std::int64_t frequency() const noexcept { return frequency_; }
    double ms(std::int64_t ticks) const noexcept { return frequency_ > 0 ? double(ticks) * 1000.0 / double(frequency_) : 0.0; }
private:
    struct Slot { std::atomic<int> job{0}; std::atomic<std::int64_t> t[StampCount]{}; };
    Slot slots_[16];
    std::int64_t frequency_;
    std::atomic<std::int64_t> eval_ema_us_{0};
};

class Ages {
public:
    Ages(const Timeline& timeline, unsigned period = 1024) : timeline_(timeline), period_(period ? period : 1) {
        ages_ms_.reserve(period_); job_ms_.reserve(512);
    }

    void frame() noexcept { ++frames_; }
    void captured() noexcept { ++captured_; }

    void composed(int job, unsigned age_frames, std::int64_t now, bool first);
    WaitStats& waits() noexcept { return waits_; }

    void late(const LateTotals& totals) noexcept { late_now_ = totals; }
    bool due() const noexcept { return frames_ >= period_; }

    void set_period(unsigned period) noexcept { period_ = period ? period : 1; }
    unsigned period() const noexcept { return period_; }

    std::string take(const Tuning& tuning);

    std::uint64_t job_bytes() const noexcept { return job_bytes_; }
    static constexpr std::uint64_t job_bytes_limit = 8ull * 1024 * 1024;
    static constexpr unsigned max_age = 31;
private:
    const Timeline& timeline_;
    unsigned period_;
    unsigned frames_{}, captured_{}, composed_{}, first_uses_{};
    unsigned hist_[max_age + 1]{};
    double age_sum_{}, first_age_sum_{};
    std::vector<double> ages_ms_, job_ms_;
    double phase_sum_[6]{}; unsigned phase_n_[6]{};
    double capt_sum_[7]{}; unsigned capt_n_[7]{}, capt_jobs_{};
    WaitStats waits_{};
    LateTotals late_now_{}, late_prev_{};
    std::string job_lines_;
    std::uint64_t job_bytes_{};
};

Timeline& timeline() noexcept;
Ages& ages();

void* completion_event() noexcept;
void signal_completion() noexcept;
}
