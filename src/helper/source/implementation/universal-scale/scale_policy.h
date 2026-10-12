#pragma once

#include <cstdint>
#include <optional>

namespace nrscale {
struct Geometry {
  std::uint32_t render_w{},render_h{},output_w{},output_h{};
};
inline bool operator==(const Geometry& a,const Geometry& b) noexcept{
  return a.render_w==b.render_w&&a.render_h==b.render_h&&a.output_w==b.output_w&&a.output_h==b.output_h;
}
struct Target {Geometry original{},scaled{};std::uint32_t percent{};};
bool supported_percent(std::uint32_t percent) noexcept;
std::optional<Target> target(Geometry original,std::uint32_t percent) noexcept;
}
