#pragma once
#include <cstdint>

namespace nrfgpub {
constexpr std::uint32_t kMagia = 0x4746524Eu;
constexpr std::uint32_t kVersao = 1u;
#pragma pack(push, 4)
struct Fg {
  std::uint32_t magia, versao;
  std::uint32_t disponivel;
  std::uint32_t ligado;
  volatile long pedido;
  std::uint32_t reservado;
  std::uint64_t geracoes;
  std::uint64_t tick_ms;
  char provedor[24];
};
#pragma pack(pop)
static_assert(sizeof(Fg) == 64, "nrfgpub::Fg: 64 bytes (o helper confere o mesmo tamanho)");
}
