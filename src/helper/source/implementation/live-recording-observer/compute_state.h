#pragma once
#include <memory>
#include <cstddef>
#include <cstdint>
namespace nrstate {

inline constexpr std::size_t teto_listas=4096;

bool observe(void* command) noexcept;

const char* observe_refusal() noexcept;
void* observed_table() noexcept;

std::size_t private_table_count() noexcept;
void reset(void* command,void* initial_pipeline) noexcept;

bool tracked(void* command) noexcept;
bool forget(void* command) noexcept;
std::size_t count() noexcept;

std::size_t bytes_por_lista() noexcept;
class Saved {
public:
    virtual ~Saved()=default;
    virtual bool suspend_predication() noexcept=0;
    virtual bool restore() noexcept=0;
};
std::unique_ptr<Saved> save(void* command) noexcept;
std::uint32_t readiness(void* command) noexcept;

void com_trava_para_teste(void(*f)(void*),void* arg) noexcept;
}
