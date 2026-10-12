#pragma once

#include "gpu_backend.h"
#include "active_scheduling.h"
#include <atomic>
#include <mutex>

namespace nrscale {
enum class PrepareStatus {Ready,Pending,Failed};
class NeuralPipeline {
public:
  virtual ~NeuralPipeline()=default;
  virtual PrepareStatus prepare(const Target&,const Resources&,void* context,std::uint32_t stage,std::uint64_t generation) noexcept=0;
  virtual std::int32_t dispatch(NrFfxDispatch original,void* context,void* descriptor,std::int32_t nesting,
    const NrActiveSelectionV1& selection,std::uint32_t* handled,NrActiveDispatchResultV1* result)=0;
};
class Coordinator {
public:
  struct Snapshot {std::uint32_t requested_percent{100},effective_percent{100},stage{};Geometry original{},scaled{};void* context{};};
  Coordinator(GpuBackend& gpu,NeuralPipeline& neural,InputAdapter* adapter=nullptr) noexcept:gpu_(gpu),neural_(neural),adapter_(adapter){}
  std::int32_t dispatch(NrFfxDispatch original,void* context,void* descriptor,std::int32_t nesting,
    const NrActiveSelectionV1& selection,std::uint32_t* handled,NrActiveDispatchResultV1* result);
  bool sticky()const noexcept{return sticky_.load(std::memory_order_acquire);}
  const char* diagnostic()const noexcept{return diagnostic_.load(std::memory_order_acquire);}

  const char* adapter_reason()const noexcept{return adapter_reason_.load(std::memory_order_acquire);}
  Snapshot snapshot()const noexcept{std::lock_guard<std::mutex> lock(snapshot_mutex_);return snapshot_;}
private:
  GpuBackend& gpu_;NeuralPipeline& neural_;InputAdapter* adapter_{};std::atomic<bool> sticky_{};
  std::atomic<const char*> adapter_reason_{""};std::uint64_t adapter_reported_{};

  unsigned adapter_sticky_{};std::uint64_t adapter_generation_{};std::uint64_t adapter_geometry_{};
  void report_adapter(const AdapterPlan&,const nrffx::RawDescriptor&,const NrResourceDesc& input,const NrResourceDesc& motion) noexcept;

  int adapted_dispatch(NrFfxDispatch original,void* context,void* descriptor,std::int32_t nesting,
    const NrActiveSelectionV1& selection,std::uint32_t* handled,NrActiveDispatchResultV1* result,std::int32_t& code);
  std::atomic<const char*> diagnostic_{"scale_idle"};std::uint32_t active_percent_{100};

  const char* decode_reported_{};
  void refuse_decode(const void* descriptor) noexcept;
  void make_sticky(const char* diagnostic) noexcept;
  mutable std::mutex snapshot_mutex_;Snapshot snapshot_{};
};
}
