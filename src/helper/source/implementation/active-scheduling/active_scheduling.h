#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_WIN32)
#define NR_ACTIVE_CALL __cdecl
#else
#define NR_ACTIVE_CALL
#endif

extern "C" {

enum : std::uint32_t {
  NR_ACTIVE_ABI_VERSION = 1,
  NR_ACTIVE_STAGE_PRE = 1,
  NR_ACTIVE_STAGE_POST = 2,
};

enum NrActiveOutcome : std::uint32_t {
  NR_ACTIVE_FSR_ONLY = 0,
  NR_ACTIVE_CAPTURE_ATTEMPTED = 1,
  NR_ACTIVE_REJECTED_STICKY = 2,
  NR_ACTIVE_BUSY = 3,
  NR_ACTIVE_PARAMETER_ERROR = 4,
};

struct NrCaptureRecord {
  void *command_list;
  void *color;
  std::uint32_t color_state;
  std::uint32_t reserved14;
  void *motion;
  std::uint32_t motion_state;
  std::uint32_t reserved24;
  void *depth;
  std::uint32_t depth_state;
  std::uint32_t reserved34;
  void *exposure;
  std::uint32_t exposure_state;
  float motion_scale_x;
  float motion_scale_y;
};

struct NrResourceDesc {
  std::uint32_t dimension;
  std::uint32_t reserved04;
  std::uint64_t alignment;
  std::uint64_t width;
  std::uint32_t height;
  std::uint16_t depth_or_array_size;
  std::uint16_t mip_levels;
  std::uint32_t format;
  std::uint32_t sample_count;
  std::uint32_t sample_quality;
  std::uint32_t layout;
  std::uint32_t flags;
  std::uint32_t reserved34;
};

struct NrActiveSelectionV1 {
  std::uint32_t size;
  std::uint32_t version;
  std::uint32_t master_enabled;
  std::uint32_t stage;
  std::uint32_t scale_percent;
  std::uint64_t generation;
};

struct NrActiveEnvironmentV1 {
  std::uint32_t size;
  std::uint32_t version;
  std::uint32_t first_use_empty;
  std::uint32_t caches_match;
  std::uint32_t engine_ready;
  std::uint32_t error_clear;
  std::uint32_t enabled;
  std::uint32_t use_fsr_inputs;
  std::uint32_t requested_inline;
  std::uint32_t requested_interop;
  std::uint32_t use_depth;
  std::uint32_t tone;
  std::uint32_t pending_reset;
  std::uint32_t active_inline;
  std::uint32_t active_interop;
  std::uint32_t current_thread;
  std::uint32_t owner_thread;
  std::uint32_t reserved44;
  std::uint64_t selected_device;
  std::uint64_t selected_queue;
};

using NrFfxDispatch = std::int32_t(NR_ACTIVE_CALL *)(void *, void *);
using NrCapture = void(NR_ACTIVE_CALL *)(void *, const NrCaptureRecord *);
using NrDescribeResource = std::uint32_t(NR_ACTIVE_CALL *)(void *, void *,
                                                           NrResourceDesc *);
using NrReadEnvironment =
    std::uint32_t(NR_ACTIVE_CALL *)(void *, NrActiveEnvironmentV1 *);
using NrGetDevice = std::uint32_t(NR_ACTIVE_CALL *)(void *, void *,
                                                    std::uint64_t *);
using NrRetainObject = std::uint32_t(NR_ACTIVE_CALL *)(void *, void *);
using NrReleaseObject = void(NR_ACTIVE_CALL *)(void *, void *);

struct NrActiveHooksV1 {
  std::uint32_t size;
  std::uint32_t version;
  void *user;
  NrCapture capture;
  NrDescribeResource describe_resource;
  NrReadEnvironment read_environment;
  NrGetDevice get_device;
  NrRetainObject retain_object;
  NrReleaseObject release_object;
};

struct NrActiveDispatchV1 {
  std::uint32_t size;
  std::uint32_t version;
  void *image_base;
  NrFfxDispatch original;
  void *context;
  void *descriptor;
  std::int32_t nesting;
  std::uint32_t reserved34;
  NrActiveSelectionV1 selection;
  NrActiveHooksV1 hooks;
};

struct NrActiveDispatchResultV1 {
  std::uint32_t size;
  std::uint32_t version;
  std::uint32_t outcome;
  std::uint32_t capture_attempted;
  std::uint32_t capture_stage;
  std::uint32_t selection_frozen;
  std::uint32_t poisoned;
  std::int32_t original_result;
};

struct NrActiveSchedulerV1 {
  std::uint32_t size;
  std::uint32_t version;
  volatile long entrant;
  std::uint32_t frozen;
  std::uint32_t poisoned;
  std::uint32_t capture_started;
  std::uint32_t reserved_after_capture;
  NrActiveSelectionV1 selection;
  NrFfxDispatch original;
  void *context;
  std::uint32_t owner_thread;
  std::uint32_t use_depth;
  std::uint32_t tone;
  std::uint32_t reserved4c;
  std::uint64_t selected_device;
  std::uint64_t selected_queue;
  void *resources[5];
  NrResourceDesc descriptions[5];
  void *retained[7];
  std::uint32_t retained_count;
  std::uint32_t reserved_after_retained;
  void *retain_user;
  NrReleaseObject release_object;
};

void NR_ACTIVE_CALL nr_active_scheduler_init_v1(NrActiveSchedulerV1 *scheduler);
void NR_ACTIVE_CALL
nr_active_scheduler_destroy_v1(NrActiveSchedulerV1 *scheduler);

std::int32_t NR_ACTIVE_CALL nr_active_schedule_dispatch_v1(
    NrActiveSchedulerV1 *scheduler, const NrActiveDispatchV1 *input,
    std::uint32_t *exclusive_handled, NrActiveDispatchResultV1 *result);

std::uint32_t NR_ACTIVE_CALL nr_active_compose_passthrough_v1(
    void *image_base, void *command_list, void *destination,
    std::uint32_t destination_state, std::int32_t job, float exposure,
    std::int32_t alternate, std::uint32_t inline_mode);
}

static_assert(sizeof(void *) == 8, "Win64 only");
static_assert(sizeof(long) == 4, "Win64 long ABI required");
static_assert(offsetof(NrCaptureRecord, command_list) == 0x00);
static_assert(offsetof(NrCaptureRecord, color) == 0x08);
static_assert(offsetof(NrCaptureRecord, color_state) == 0x10);
static_assert(offsetof(NrCaptureRecord, motion) == 0x18);
static_assert(offsetof(NrCaptureRecord, motion_state) == 0x20);
static_assert(offsetof(NrCaptureRecord, depth) == 0x28);
static_assert(offsetof(NrCaptureRecord, depth_state) == 0x30);
static_assert(offsetof(NrCaptureRecord, exposure) == 0x38);
static_assert(offsetof(NrCaptureRecord, exposure_state) == 0x40);
static_assert(offsetof(NrCaptureRecord, motion_scale_x) == 0x44);
static_assert(offsetof(NrCaptureRecord, motion_scale_y) == 0x48);
static_assert(sizeof(NrCaptureRecord) == 0x50);
static_assert(sizeof(NrResourceDesc) == 0x38);
static_assert(offsetof(NrActiveSelectionV1, generation) == 0x18);
static_assert(sizeof(NrActiveSelectionV1) == 0x20);
static_assert(sizeof(NrActiveHooksV1) == 0x40);
static_assert(sizeof(NrActiveDispatchV1) == 0x90);
