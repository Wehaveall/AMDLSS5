#pragma once

#include <cstddef>
#include <cstdint>

namespace nrffx {
constexpr std::size_t descriptor_size=0x1c0;
struct Descriptor {
  void *command{},*input{},*output{},*motion{},*depth{},*exposure{};
  std::uint32_t input_state{},output_state{},motion_state{},depth_state{},exposure_state{};
  float motion_x{},motion_y{};
  std::uint32_t render_w{},render_h{},output_w{},output_h{};

  float jitter_x{},jitter_y{};

  float camera_near{},camera_far{},camera_fov{};
};
struct ScaledFields {
  void *input{},*output{},*motion{},*depth{};
  float motion_x{},motion_y{};
  std::uint32_t render_w{},render_h{},output_w{},output_h{};
};
bool decode(const void* head,Descriptor& out) noexcept;

struct RawDescriptor:Descriptor {
  float pre_exposure{};
  bool reset{};
};
bool decode_raw(const void* head,RawDescriptor& out) noexcept;

struct AdaptedFields {
  void *input{},*motion{},*depth{};
  std::uint32_t input_state{},motion_state{},depth_state{};
};
bool encode_adapted_copy(const void* source,std::size_t source_size,
  const AdaptedFields& fields,std::byte* destination,std::size_t destination_size) noexcept;

const char* refusal(const void* head,char* detalhe,std::size_t n,char* campo,std::size_t m) noexcept;

void set_use_game_exposure(bool enabled) noexcept;

bool host_reset(const void* head) noexcept;
bool encode_scaled_copy(const void* source,std::size_t source_size,
  const ScaledFields& fields,std::byte* destination,std::size_t destination_size) noexcept;
}
