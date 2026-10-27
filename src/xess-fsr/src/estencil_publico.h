#pragma once
#include <cstdint>

namespace nrest {
constexpr std::uint32_t kMagia = 0x5345524Eu;
constexpr std::uint32_t kVersao = 1u;
#pragma pack(push, 4)
struct Publicado {
  std::uint32_t magia, versao;
  volatile long seq;
  std::uint32_t ok;
  std::uint64_t handle;
  std::uint32_t geracao;
  std::uint32_t largura, altura, pitch;
  std::uint64_t execucao;
  std::uint64_t tick_ms;
  std::uint32_t luid_baixo;
  std::int32_t luid_alto;
};
#pragma pack(pop)
static_assert(sizeof(Publicado) == 64, "nrest::Publicado: 64 bytes (o helper confere o mesmo tamanho)");
}
