#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
namespace nrrecord {
enum class Phase { Unobserved, Open, Closed, Failed };
struct Snapshot {
    Phase phase{Phase::Unobserved};
    std::uint64_t epoch{},generation{};
    bool neural{},submitted{};
    bool busy{},primed{};
    std::uint64_t resets{},closes{};
    const char* diagnostic{"unobserved"};
};
class Observer {
public:
    virtual ~Observer()=default;
    virtual Snapshot snapshot() noexcept=0;
    virtual bool mark_neural(std::uint64_t generation) noexcept=0;
    virtual bool mark_submitted(std::uint64_t epoch) noexcept=0;
    virtual bool can_retire(std::uint64_t generation) noexcept=0;

    virtual bool retirable_if_abandoned(std::uint64_t generation) noexcept{return can_retire(generation);}

    virtual std::uint64_t uso_reset() noexcept{return 0;}
};

enum class Method { Reset, Close };
class Ledger final:public Observer {
public:
    explicit Ledger(std::uint32_t owner) noexcept;
    Snapshot snapshot() noexcept override;
    bool mark_neural(std::uint64_t generation) noexcept override;
    bool mark_submitted(std::uint64_t epoch) noexcept override;
    bool can_retire(std::uint64_t generation) noexcept override;
    bool retirable_if_abandoned(std::uint64_t generation) noexcept override;
    bool begin(Method method,std::uint32_t thread) noexcept;
    void end(Method method,std::uint32_t thread,std::int32_t result) noexcept;
    void fail(const char* diagnostic) noexcept;

    void pronta_no_reset() noexcept;
private:
    std::mutex mutex_;
    Snapshot state_{};
    std::uint32_t owner_{};
    std::uint32_t method_thread_{};
    bool busy_{};
    Method method_{};
    void fail_locked(const char* diagnostic) noexcept;
};

std::unique_ptr<Observer> make_windows_observer(void* command,std::uint64_t device,std::uint32_t owner) noexcept;

const char* last_refusal() noexcept;

bool forget(void* command) noexcept;

std::size_t endpoints() noexcept;

std::uint64_t avancar_uso() noexcept;
std::uint64_t relogio_uso() noexcept;

std::size_t indice_falhas() noexcept;

using GanchoResetNaoRastreada=void(*)(void* command) noexcept;
void set_reset_nao_rastreada(GanchoResetNaoRastreada gancho) noexcept;
}
