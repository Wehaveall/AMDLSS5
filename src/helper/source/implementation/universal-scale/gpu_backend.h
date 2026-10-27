#pragma once

#include "scale_policy.h"
#include "ffx_descriptor.h"
#include "input_adapter.h"
#include <cstdint>

namespace nrscale {
struct Resources {void* input{};void* output{};void* motion{};void* depth{};Geometry geometry{};};
enum class GpuStatus {Ready,Unsupported,FailedBeforeRecord,FailedAfterRecord};
class GpuBackend {
public:
  virtual ~GpuBackend()=default;
  virtual GpuStatus prepare(const nrffx::Descriptor&,const Target&,Resources&) noexcept=0;
  virtual GpuStatus activate() noexcept=0;
  virtual GpuStatus deactivate() noexcept=0;
  virtual GpuStatus downsample(void* command,std::uint32_t stage) noexcept=0;
  virtual GpuStatus reconstruct(void* command,std::uint32_t stage) noexcept=0;
};

struct AdaptedInputs {void* color{};std::uint32_t color_state{};void* motion{};std::uint32_t motion_state{};void* depth{};std::uint32_t depth_state{};
  void* output{};std::uint32_t output_state{};
  void* exposure{};std::uint32_t exposure_state{};};
class InputAdapter {
public:
  virtual ~InputAdapter()=default;
  virtual bool describe(void* resource,NrResourceDesc& out) noexcept=0;

  virtual GpuStatus convert(const nrffx::RawDescriptor& d,const AdapterPlan& plan,AdaptedInputs& out) noexcept=0;

  virtual GpuStatus finish(const nrffx::RawDescriptor& d,const AdapterPlan& plan) noexcept=0;
};
}
