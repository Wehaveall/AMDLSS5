#pragma once
#include <cstdint>
#include "../pipeline-api.h"

namespace nrfsr2 {

void tentar(void* base) noexcept;

std::uint64_t despachos() noexcept;
namespace teste {

unsigned enganchar(void* base) noexcept;
}
}

std::int32_t nr_despacho_generico(void* base,nrabi::FfxDispatch original,void* context,void* descriptor,std::int32_t nesting,std::uint32_t* handled);
