#pragma once

#include "captura_carimbo.h"
#include <cstdint>

namespace nrcapt {

void marcar(void* command_list, int job, Ponto p) noexcept;

void submetida(int job, void* queue, void* fence, std::uint64_t alvo) noexcept;
}
