#include "active_scheduling.h"
#include "ffx_descriptor.h"
#include "admission_reason.h"
#include "../../../runtime/reuso_state.h"

#include <cstdio>
#include <cstring>
#include <intrin.h>

namespace {
constexpr std::uint32_t kTexture2d = 3, kRgba16Float = 10, kRg16Float = 34,
                        kR32Typeless = 39, kAllowUav = 4, kDenySrv = 8;
using nrffx::Descriptor;
void init_result(NrActiveDispatchResultV1 *r) {
  if (!r)
    return;
  std::memset(r, 0, sizeof(*r));
  r->size = sizeof(*r);
  r->version = 1;
  r->outcome = NR_ACTIVE_FSR_ONLY;
}
bool valid_header(const NrActiveDispatchV1 *i) {
  return i && i->size == sizeof(*i) && i->version == 1 &&
         i->selection.size == sizeof(i->selection) &&
         i->selection.version == 1 && i->hooks.size == sizeof(i->hooks) &&
         i->hooks.version == 1;
}
bool equal_desc(const NrResourceDesc &a, const NrResourceDesc &b) {
  return std::memcmp(&a, &b, sizeof(a)) == 0;
}
bool describe_all(const NrActiveDispatchV1 &i, const Descriptor &d,
                  NrResourceDesc (&x)[5], std::uint64_t selected) {
  if (!i.hooks.describe_resource || !i.hooks.get_device || !selected)
    return false;
  void *o[5]{d.input, d.output, d.motion, d.depth, d.exposure};
  for (unsigned n = 0; n < (d.exposure ? 5u : 4u); ++n) {
    if (!i.hooks.describe_resource(i.hooks.user, o[n], &x[n]))
      return false;
    std::uint64_t device = 0;
    if (!i.hooks.get_device(i.hooks.user, o[n], &device) || device != selected)
      return false;
  }
  std::uint64_t device = 0;
  if (!i.hooks.get_device(i.hooks.user, d.command, &device) ||
      device != selected)
    return false;
  return nractive::resource_refusal(d, x, nullptr, 0) == nullptr;
}
bool read_env(const NrActiveDispatchV1 &i, NrActiveEnvironmentV1 &e) {
  if (!i.hooks.read_environment)
    return false;
  std::memset(&e, 0, sizeof(e));
  e.size = sizeof(e);
  e.version = 1;
  return i.hooks.read_environment(i.hooks.user, &e) && e.size == sizeof(e) &&
         e.version == 1;
}
bool environment_ok(const NrActiveEnvironmentV1 &e) {
  return nractive::environment_refusal(e) == nullptr;
}

bool environment_ok_ignoring_master(NrActiveEnvironmentV1 e) {
  e.enabled = 1;
  return environment_ok(e);
}
bool supported_scale(std::uint32_t value){
  switch(value){case 100:case 85:case 70:case 65:case 50:case 40:case 30:return true;default:return false;}
}
bool selection_ok(const NrActiveSelectionV1 &s) {
  return s.master_enabled == 1 && supported_scale(s.scale_percent) && (s.stage == NR_ACTIVE_STAGE_PRE || s.stage == NR_ACTIVE_STAGE_POST);
}
bool frozen_matches(const NrActiveSchedulerV1 &s, const NrActiveDispatchV1 &i,
                    const Descriptor &d, const NrResourceDesc (&x)[5],
                    const NrActiveEnvironmentV1 &e) {
  if (s.selection.master_enabled != i.selection.master_enabled ||
      s.selection.stage != i.selection.stage ||
      s.selection.scale_percent != i.selection.scale_percent ||
      s.selection.generation != i.selection.generation ||
      s.original != i.original || s.context != i.context ||
      s.use_depth != e.use_depth ||
      s.tone != e.tone || s.reserved4c != e.requested_inline || s.selected_device != e.selected_device ||
      s.selected_queue != e.selected_queue)
    return false;
  if (s.capture_started &&
      (!e.caches_match || e.active_inline != s.reserved4c || e.active_interop != 1))
    return false;
  void *o[5]{d.input, d.output, d.motion, d.depth, d.exposure};
  for (unsigned n = 0; n < 5; ++n)
    if (s.resources[n] != o[n] || !equal_desc(s.descriptions[n], x[n]))
      return false;
  return true;
}
bool retain_binding(NrActiveSchedulerV1 &s, const NrActiveDispatchV1 &i,
                    const Descriptor &d, const NrResourceDesc (&x)[5],
                    const NrActiveEnvironmentV1 &e) {
  if (!i.hooks.retain_object || !i.hooks.release_object)
    return false;
  void *o[7]{d.input,
             d.output,
             d.motion,
             d.depth,
             reinterpret_cast<void *>(e.selected_device),
             reinterpret_cast<void *>(e.selected_queue),
             nullptr};
  unsigned count = 6;
  if (d.exposure)
    o[count++] = d.exposure;
  unsigned acquired = 0;
  for (; acquired < count; ++acquired)
    if (!i.hooks.retain_object(i.hooks.user, o[acquired]))
      break;
  if (acquired != count) {
    while (acquired)
      i.hooks.release_object(i.hooks.user, o[--acquired]);
    return false;
  }
  s.selection = i.selection;
  s.original = i.original;
  s.context = i.context;
  s.owner_thread = e.current_thread;
  s.use_depth = e.use_depth;
  s.tone = e.tone;
  s.reserved4c = e.requested_inline;
  s.selected_device = e.selected_device;
  s.selected_queue = e.selected_queue;
  void *r[5]{d.input, d.output, d.motion, d.depth, d.exposure};
  for (unsigned n = 0; n < 5; ++n) {
    s.resources[n] = r[n];
    s.descriptions[n] = x[n];
  }
  for (unsigned n = 0; n < count; ++n)
    s.retained[n] = o[n];
  s.retained_count = count;
  s.retain_user = i.hooks.user;
  s.release_object = i.hooks.release_object;
  s.frozen = 1;
  return true;
}
const char *volatile ultimo_veneno_ = "";
void poison(NrActiveSchedulerV1 &s, NrActiveDispatchResultV1 *r, const char *motivo) {
  ultimo_veneno_ = motivo ? motivo : "?";
  s.poisoned = 1;
  if (r) {
    r->poisoned = 1;
    r->selection_frozen = s.frozen;
    r->outcome = NR_ACTIVE_REJECTED_STICKY;
  }
}
NrCaptureRecord make_record(const Descriptor &d, std::uint32_t stage) {
  NrCaptureRecord r{};
  r.command_list = d.command;
  r.motion = d.motion;
  r.motion_state = d.motion_state;
  r.depth = d.depth;
  r.depth_state = d.depth_state;
  r.exposure = d.exposure;
  r.exposure_state = d.exposure_state;
  r.motion_scale_x = d.motion_x;
  r.motion_scale_y = d.motion_y;
  if (stage == NR_ACTIVE_STAGE_PRE) {
    r.color = d.input;
    r.color_state = d.input_state;
  } else {
    r.color = d.output;
    r.color_state = d.output_state;
  }
  return r;
}
}

namespace nractive {

const char *resource_refusal(const nrffx::Descriptor &d, const NrResourceDesc (&x)[5], char *campo, std::size_t n) noexcept {
  auto put = [&](const char *id, const char *format, auto... values) {
    if (campo && n) std::snprintf(campo, n, format, values...);
    return id;
  };
  if (campo && n) campo[0] = 0;
  auto layout = [](const NrResourceDesc &r) {
    return r.dimension == kTexture2d && r.depth_or_array_size == 1 && r.mip_levels == 1 && r.sample_count == 1;
  };
  const char *nome[2]{"cor", "saida"};
  const std::uint32_t w[2]{d.render_w, d.output_w}, h[2]{d.render_h, d.output_h};
  for (unsigned c = 0; c < 2; ++c) {
    const auto &r = x[c];
    if (!layout(r) || !w[c] || !h[c] || w[c] > 16384 || h[c] > 16384)
      return put("describe_resource_layout", "%s dim %u mips %u amostras %u", nome[c], r.dimension, r.mip_levels, r.sample_count);
    if (r.width != w[c] || r.height != h[c])
      return put("describe_size_equals_render", "%s %llux%u != %ux%u", nome[c], static_cast<unsigned long long>(r.width), r.height, w[c], h[c]);
    if (r.format != kRgba16Float && r.format != 67)
      return put("describe_color_format", "%s fmt %u", nome[c], r.format);
    if (!(r.flags & kAllowUav) || (r.flags & kDenySrv))
      return put("describe_color_uav", "%s flags 0x%x", nome[c], r.flags);
  }
  const char *papel[2]{"mv", "prof"};
  for (unsigned k = 0; k < 2; ++k) {
    const auto &r = x[2 + k];
    if (!layout(r))
      return put("describe_resource_layout", "%s dim %u mips %u amostras %u", papel[k], r.dimension, r.mip_levels, r.sample_count);
    if (r.width != d.render_w || r.height != d.render_h)
      return put("describe_size_equals_render", "%s %llux%u != %ux%u", papel[k], static_cast<unsigned long long>(r.width), r.height, d.render_w, d.render_h);
    const bool formato = k == 0 ? (r.format == kRg16Float || r.format == kRgba16Float)
                                : (r.format == kR32Typeless || r.format == 19 || r.format == 41);
    if (!formato)
      return put(k == 0 ? "describe_motion_format" : "describe_depth_format", "%s fmt %u", papel[k], r.format);
    if (r.flags & kDenySrv)
      return put("describe_resource_layout", "%s DENY_SRV", papel[k]);
  }
  if (d.exposure) {
    const auto &r = x[4];
    if (!layout(r) || r.width != 1 || r.height != 1 || r.format != 41 || (r.flags & kDenySrv))
      return put("describe_exposure", "exp fmt %u %llux%u", r.format, static_cast<unsigned long long>(r.width), r.height);
  }
  if (d.motion_x != static_cast<float>(d.render_w) || d.motion_y != static_cast<float>(d.render_h))
    return put("describe_motion_scale_equals_render", "mvScale %.4g,%.4g", static_cast<double>(d.motion_x), static_cast<double>(d.motion_y));
  return nullptr;
}
const char *ultimo_veneno() noexcept { return ultimo_veneno_; }
const char *environment_refusal(const NrActiveEnvironmentV1 &e) noexcept {
  if (e.engine_ready != 1) return "engine_ready";
  if (e.error_clear != 1) return "error_clear";
  if (e.enabled != 1) return "enabled";
  if (e.use_fsr_inputs != 1) return "use_fsr_inputs";
  if (!(e.requested_inline == 1 || (e.requested_inline == 0 && e.reserved44 == 0xA51Cu))) return "requested_inline";
  if (e.requested_interop != 1) return "requested_interop";
  if (e.pending_reset != 0) return "pending_reset";
  if (!e.current_thread || !e.owner_thread) return "thread";
  if (!e.selected_device || !e.selected_queue) return "device_queue";
  return nullptr;
}
}

extern "C" void NR_ACTIVE_CALL
nr_active_scheduler_init_v1(NrActiveSchedulerV1 *s) {
  if (!s)
    return;
  std::memset(s, 0, sizeof(*s));
  s->size = sizeof(*s);
  s->version = 1;
}
extern "C" void NR_ACTIVE_CALL
nr_active_scheduler_destroy_v1(NrActiveSchedulerV1 *s) {
  if (!s || s->size != sizeof(*s) || s->version != 1)
    return;
  if (s->release_object)
    for (std::uint32_t i = s->retained_count; i > 0; --i)
      s->release_object(s->retain_user, s->retained[i - 1]);
  nr_active_scheduler_init_v1(s);
}
extern "C" std::int32_t NR_ACTIVE_CALL nr_active_schedule_dispatch_v1(
    NrActiveSchedulerV1 *s, const NrActiveDispatchV1 *i, std::uint32_t *handled,
    NrActiveDispatchResultV1 *r) {
  if (handled)
    *handled = 1;
  init_result(r);
  if (!valid_header(i) || !i->original) {
    if (r)
      r->outcome = NR_ACTIVE_PARAMETER_ERROR;
    return 6;
  }
  auto forward = [&]() {
    const auto value = i->original(i->context, i->descriptor);
    if (r)
      r->original_result = value;
    return value;
  };
  if (!s || s->size != sizeof(*s) || s->version != 1)
    return forward();
  if (_InterlockedCompareExchange(&s->entrant, 1, 0) != 0) {
    if (r)
      r->outcome = NR_ACTIVE_BUSY;
    return forward();
  }
  struct Exit {
    volatile long *p;
    ~Exit() { _InterlockedExchange(p, 0); }
  } exit{&s->entrant};
  if (s->poisoned) {
    if (r) {
      r->poisoned = 1;
      r->selection_frozen = s->frozen;
      r->outcome = NR_ACTIVE_REJECTED_STICKY;
    }
    return forward();
  }
  if (i->nesting != 0)
    return forward();
  Descriptor d{};
  NrResourceDesc x[5]{};
  NrActiveEnvironmentV1 e{};
  if (!read_env(*i, e)) {
    if (s->frozen)
      poison(*s, r, "antes_ambiente_ilegivel");
    return forward();
  }

  if (e.enabled == 0)
    return forward();

  if (nrffx::host_reset(i->descriptor)) {
    nrreusostate::pedir_cheio();
    return forward();
  }
  if (!selection_ok(i->selection) || !i->hooks.capture || !environment_ok(e)) {
    if (s->frozen)
      poison(*s, r, !selection_ok(i->selection) ? "antes_selecao" : !i->hooks.capture ? "antes_captura_nula"
                                                                    : nractive::environment_refusal(e));
    return forward();
  }

  if (!nrffx::decode(i->descriptor, d) || !describe_all(*i, d, x, e.selected_device)) {
    if (s->frozen && r)
      r->selection_frozen = 1;
    return forward();
  }
  if (s->frozen) {
    if (!frozen_matches(*s, *i, d, x, e)) {
      poison(*s, r, "antes_congelado_diferente");
      return forward();
    }
  } else {
    if (!retain_binding(*s, *i, d, x, e))
      return forward();
    if (!e.first_use_empty) {
      NrActiveEnvironmentV1 adopted{};
      if (!read_env(*i, adopted) || !environment_ok(adopted) ||
          !adopted.caches_match || adopted.active_inline != s->reserved4c ||
          adopted.active_interop != 1) {
        nr_active_scheduler_destroy_v1(s);
        return forward();
      }
    }
  }
  auto capture = [&]() {
    const auto record = make_record(d, s->selection.stage);
    s->capture_started = 1;
    if (r) {
      r->capture_attempted = 1;
      r->capture_stage = s->selection.stage;
      r->selection_frozen = 1;
      r->outcome = NR_ACTIVE_CAPTURE_ATTEMPTED;
    }
    try {
      i->hooks.capture(i->hooks.user, &record);
    } catch (...) {

      poison(*s, r, "captura_excecao");
      return;
    }
    NrActiveEnvironmentV1 after{};
    if (!read_env(*i, after) || !environment_ok_ignoring_master(after) ||
        after.active_inline != s->reserved4c || after.active_interop != 1)
      poison(*s, r, "depois_da_captura");
  };
  if (s->selection.stage == NR_ACTIVE_STAGE_PRE) {
    capture();
    return forward();
  }
  const auto status = forward();
  if (status == 0) {
    NrActiveEnvironmentV1 after{};
    NrResourceDesc ax[5]{};
    Descriptor ad{};

    if (read_env(*i, after) && after.enabled == 0 && environment_ok_ignoring_master(after))
      return status;
    if (!nrffx::decode(i->descriptor, ad) || !read_env(*i, after) ||
        !environment_ok(after) ||
        !describe_all(*i, ad, ax, after.selected_device) ||
        !frozen_matches(*s, *i, ad, ax, after)) {
      poison(*s, r, "depois_do_fsr_post");
      return status;
    }
    d = ad;
    capture();
  }
  return status;
}
extern "C" std::uint32_t NR_ACTIVE_CALL nr_active_compose_passthrough_v1(
    void *, void *, void *, std::uint32_t, std::int32_t, float, std::int32_t,
    std::uint32_t) {
  return 0;
}
