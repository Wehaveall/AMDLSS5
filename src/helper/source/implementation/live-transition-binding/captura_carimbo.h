#pragma once

#include <cstdint>

namespace nrcapt {
enum Ponto : unsigned { AntesBegin = 0, AntesNativa = 1, FimCopia = 2, DepoisNativa = 3, Pontos = 4 };

constexpr bool chave(long long ini) noexcept { return ini == 1; }

constexpr std::int64_t gpu_para_qpc(std::uint64_t gpu, std::uint64_t gpu0, std::int64_t qpc0, std::uint64_t gpu_freq,
                                    std::int64_t qpc_freq) noexcept {
    if (!gpu_freq || qpc_freq <= 0) return 0;
    const long double d = static_cast<long double>(static_cast<std::int64_t>(gpu - gpu0)) * static_cast<long double>(qpc_freq) /
                          static_cast<long double>(gpu_freq);
    return qpc0 + static_cast<std::int64_t>(d < 0 ? d - 0.5L : d + 0.5L);
}

constexpr unsigned indice(int job, Ponto p) noexcept { return (static_cast<unsigned>(job) & 15u) * Pontos + p; }
}
