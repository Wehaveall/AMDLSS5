#pragma once
#include <cstddef>
#include <cstdint>
#include "../active-scheduling/fsr2_descriptor.h"

namespace nrfsr2ffx {

bool pedido() noexcept;

void ao_criar(void* ctx,const void* descricao_fsr2) noexcept;

bool despachar(void* ctx,const void* desc,const nrffx::fsr2::Leiaute& L,std::size_t legivel,std::int32_t& retorno) noexcept;

void ao_destruir(void* ctx) noexcept;

std::uint64_t despachos() noexcept;
namespace teste {
struct Estado{void* ffx;std::uint32_t flags_ffx,max_w,max_h,tela_w,tela_h;unsigned recriacoes;bool visto_pelo_gancho,recusado;std::uint64_t despachos;};
bool estado(void* ctx,Estado& e) noexcept;
unsigned vivos() noexcept;
std::uint64_t destruidos() noexcept;
}
}
