#pragma once
#include "../active-binding/memory_contract.h"
#include "../live-transition-controller/controller.h"
#include "../live-transition-fence-proof/fence_proof.h"
#include "../live-transition-relays/lifecycle_callbacks.h"
#include "../live-recording-observer/recording_observer.h"
#include "job_timeline.h"
#include <memory>
#include <string>
#include <limits>
#include "../host-resolution-api.h"

namespace nrlive {
enum class Proof { Pending, Complete, Failed };
struct MenuSnapshot {
    nrtransition::State state{nrtransition::State::FailedFsrOnly};
    nrtransition::Stage requested{nrtransition::Stage::Pre};
    std::uint32_t effective_stage{};
    std::uint64_t generation{};
    bool enabled{};
    int async_pending_job{},async_completed_job{};
    float local_structure{},skin_structure{};
    std::uint32_t render_width{},render_height{},output_width{},output_height{};
    std::uint64_t observed_epoch{},observed_resets{},observed_closes{};
    bool recording_primed{};
    unsigned working_percent=100;
    std::uint64_t enabled_dispatches{},captured_jobs{},pending_unsubmitted{},pending_submitted{},compose_accepted{},compose_skipped{};
    const char* diagnostic{"not_admitted"};
};

class Host {
public:
    virtual ~Host()=default;
    virtual void* base() const noexcept=0;
    virtual bool verify() noexcept=0;
    virtual const char* verification_diagnostic()const noexcept{return "host_verification_failed";}
    virtual bool publish(const NRLifecycleCallbacks*) noexcept=0;
    virtual bool hip_imports_valid() noexcept=0;
    virtual nrrecord::Observer* recording(void* command,std::uint64_t canonical_device,std::uint32_t owner) noexcept=0;

    virtual void recordings_in_use(void* const* commands,unsigned count) noexcept{(void)commands;(void)count;}
    virtual bool read(nractive::Globals&) noexcept=0;
    virtual std::uint32_t thread() const noexcept=0;
    virtual std::uint64_t now() const noexcept=0;
    virtual bool yield_gpu_progress()noexcept{return false;}

    virtual bool wait_batch(nrfence::OwnedFence* fence,bool job_in_flight)noexcept{(void)fence;(void)job_in_flight;return yield_gpu_progress();}

    virtual void frame_wait_begin(nrfence::OwnedFence* fence)noexcept{(void)fence;}
    virtual void frame_wait_end(unsigned voltas,unsigned worker)noexcept{(void)voltas;(void)worker;}
    virtual std::unique_ptr<nrfence::OwnedFence> fence_on_queue(std::uint64_t,std::uint64_t){return {};}
    virtual bool describe(void*,NrResourceDesc&) noexcept=0;
    virtual bool device_of(void*,std::uint64_t&) noexcept=0;
    virtual bool retain(void*) noexcept=0;
    virtual void release(void*) noexcept=0;
    virtual void capture(const NrCaptureRecord&)=0;

    virtual unsigned rede_cada() noexcept {return 1;}
    virtual void capture_sem_job(bool) noexcept {}
    virtual void invalidate_reprojection() noexcept {}
    virtual void retire_reprojection() noexcept {}

    virtual unsigned reproject(void*,void*,std::uint32_t,int,float,std::uint32_t) noexcept {return 1;}

    virtual unsigned reproject_post(void* command,void* output,unsigned ffx_state)noexcept{(void)command;(void)output;(void)ffx_state;return 1;}

    virtual unsigned filtros_post(void* command,void* output,unsigned ffx_state)noexcept{(void)command;(void)output;(void)ffx_state;return 1;}

    virtual bool dump_final(void*,void*,unsigned,void*,unsigned,unsigned,std::uint64_t)noexcept{return false;}
    virtual void dump_final_tick()noexcept{}

    virtual void reproject_segurado(int)noexcept{}

    virtual void dump_final_prepare(void*,void*,void*,unsigned)noexcept{}
    virtual void clear_async_completion() noexcept {}

    virtual void captura_submetida(int,void*) noexcept {}

    virtual bool fg_render_queue_learning() noexcept {return true;}

    virtual bool set_inline_mode(unsigned) noexcept {return false;}
    virtual int async_completion(int) noexcept {return (std::numeric_limits<int>::min)();}

    virtual nrjob::Tuning async_tuning() noexcept {return {};}

    virtual long long async_remaining_us(int) noexcept {return -1;}

    virtual int wait_async_completion(int,unsigned) noexcept {return -1;}

    virtual void age_log(const std::string&) noexcept {}

    virtual void late_candidate(int) noexcept {}
    virtual nrjob::LateTotals late_totals() noexcept {return {};}
    virtual bool set_pending_reset() noexcept=0;
    virtual bool reset_staging() noexcept { return false; }

    virtual bool clamp_structure(float,float,unsigned,unsigned) noexcept { return false; }
    virtual std::unique_ptr<nrfence::OwnedFence> fence(std::uint64_t)=0;
    virtual bool stop_producers(std::uint64_t) noexcept=0;
    virtual Proof producers(std::uint64_t) noexcept=0;
    virtual Proof hip_barrier(std::uint64_t) noexcept=0;
};
class PinnedBinding final {
public:
    explicit PinnedBinding(void* image_base);
    explicit PinnedBinding(std::unique_ptr<Host> authored_host);
    ~PinnedBinding();
    PinnedBinding(const PinnedBinding&)=delete;
    PinnedBinding& operator=(const PinnedBinding&)=delete;
    std::int32_t dispatch(NrFfxDispatch original,void* context,void* descriptor,
        std::int32_t nesting,const NrActiveSelectionV1& selection,
        std::uint32_t* handled,NrActiveDispatchResultV1* result);
    std::uint32_t compose(void* command_list,void* destination,std::uint32_t state,
        std::int32_t job,float exposure,std::int32_t alternate,std::uint32_t inline_mode) noexcept;
    void tick() noexcept;

    void tick(void(*also)(void*),void* context) noexcept;
    unsigned reproject(void* command,void* destination,std::uint32_t state,int job,float exposure,std::uint32_t controls) noexcept;
    MenuSnapshot snapshot() const noexcept;
    const char* status() const noexcept;
    void report_error(const char*) noexcept;
    std::optional<std::uint64_t> acknowledged_generation() const noexcept;
    std::uint64_t queue_begin(void*,std::uint32_t,void**) noexcept;
    void queue_end(std::uint64_t,void*,std::uint32_t,void**) noexcept;
    void hip_complete(std::int32_t,std::int32_t) noexcept;
    std::uint32_t reset_gate(std::uint32_t) noexcept;
    bool publish_callbacks() noexcept;
    nrhost::Status prepare_resolution(nrhost::Ticket&) noexcept;
    nrhost::Status install_resolution(const nrhost::Ticket&,const nrhost::Resources&) noexcept;
    nrhost::Status poll_resolution(std::uint64_t) noexcept;
    nrhost::Status cancel_resolution(std::uint64_t) noexcept;
private:
    struct Impl;
    explicit PinnedBinding(std::shared_ptr<Impl> retained);
    std::shared_ptr<Impl> impl_;
};
std::unique_ptr<Host> make_windows_host(void* base);

void note_present_queue(std::uint64_t queue) noexcept;
bool present_queue_known(std::uint64_t queue) noexcept;
void forget_present_queues() noexcept;

void note_frame_generation() noexcept;
bool frame_generation_seen() noexcept;

inline constexpr unsigned listas_por_submissao_max=65536;

bool slot_callback_publicavel(const void* image_base,const void* slot) noexcept;
bool post_reset_matches(const nractive::Globals& before,const nractive::Globals& after) noexcept;
}
