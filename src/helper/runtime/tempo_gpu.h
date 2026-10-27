#pragma once

#include "nr_publico.h"
#include <atomic>
#include <cstdint>
namespace nrtgpu {
enum Passe : unsigned {
    ComposeTotal = 0,
    Reduzir, Warp, Fill, Estab, Cadeia,
    MascaraHalo,
    RostoLimpa,
    ComposePost,
    Rosto,
    RostoNpc,
    Camada,
    RostoRecorte,
    Filtros,
    Captura,
    PostTardio,
    Passes
};
inline void configurar(bool, unsigned) noexcept {}
inline bool ligado() noexcept { return false; }
inline unsigned inicio(void*, unsigned) noexcept { return 0u; }
inline void fim(void*, unsigned) noexcept {}
inline void quadro() noexcept {}
inline void coletar_tudo() noexcept {}
inline std::uint64_t vezes_total(unsigned) noexcept { return 0u; }
struct Escopo { Escopo(void*, unsigned) noexcept {} void fechar() noexcept {} };
}
