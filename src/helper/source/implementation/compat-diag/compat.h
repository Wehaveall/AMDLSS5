#pragma once
#include <cstddef>

namespace nrcompat {

enum class Origem : unsigned { Binding=0, Descritor, Admissao, Captura, Composicao, Count };

void report(const char* id,const char* detalhe) noexcept;

void block(Origem origem,const char* id,const char* detalhe,const char* campo=nullptr) noexcept;

void unblock(Origem origem) noexcept;

bool blocked(Origem origem) noexcept;

struct Motivo { char id[64]; char campo[48]; };

Motivo last_reason() noexcept;

void gate_id(const char* diagnostico,char* out,std::size_t n) noexcept;

void module_of(const void* address,char* out,std::size_t n) noexcept;
void region_of(const void* address,char* out,std::size_t n) noexcept;

class Limiter {
public:
    static constexpr unsigned per_id=4,total=64;
    bool admit(const char* id) noexcept;
    unsigned lines() const noexcept { return lines_; }
private:
    struct Entry { char id[64]; unsigned count; };
    Entry entries_[total]{};
    unsigned used_{},lines_{};
};

namespace teste {

using Sink=void(*)(const char* linha);
void set_sink(Sink sink) noexcept;

void reset() noexcept;
}
}
