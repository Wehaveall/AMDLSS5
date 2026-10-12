#pragma once
#include <cstdint>
#include <cstddef>

namespace nrabi {
using FfxDispatch = std::int32_t (*)(void* context, void* descriptor);
using Dispatch = std::int32_t (*)(void* base, FfxDispatch original,
    void* context, void* descriptor, std::int32_t nesting, std::uint32_t* handled);
using Compose = std::uint32_t (*)(void* base, void* command_list, void* destination,
    std::uint32_t state, std::int32_t job, float exposure,
    std::int32_t alternate, std::uint32_t inline_mode);
using Notify = void (*)(void* base);
struct PipelineApiV1 {
    std::uint32_t size;
    std::uint32_t version;
    Dispatch dispatch;
    Compose compose;
    Notify tick;
    Notify reload;
};
using GetApi = const PipelineApiV1* (*)();
using LegacyDispatch = std::int32_t (*)(void* base, FfxDispatch original,
    void* context, void* descriptor, std::int32_t nesting, std::uint32_t* handled);
struct LegacyPipelineApiV1 {
    std::uint32_t size;
    std::uint32_t version;
    LegacyDispatch dispatch_fsr3;
};
using GetLegacyApi = const LegacyPipelineApiV1* (*)();
static_assert(sizeof(void*) == 8);
static_assert(sizeof(PipelineApiV1) == 40);
static_assert(offsetof(PipelineApiV1, dispatch) == 8);
static_assert(offsetof(PipelineApiV1, compose) == 16);
static_assert(offsetof(PipelineApiV1, tick) == 24);
static_assert(offsetof(PipelineApiV1, reload) == 32);
static_assert(sizeof(LegacyPipelineApiV1) == 16);
static_assert(offsetof(LegacyPipelineApiV1, dispatch_fsr3) == 8);
}
