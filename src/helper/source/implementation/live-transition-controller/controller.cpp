#include "controller.h"
#include <limits>
namespace nrtransition {
namespace { constexpr Id Max = std::numeric_limits<Id>::max(); }
Controller::Controller(Config c, Id now):config_(c),requested_(c.initial),initializing_(c.initial),generation_(c.first_generation),last_time_(now),started_(now),enabled_(c.enabled) {
    if (!c.device || !c.queue || !c.first_generation || !c.deadline_ms || now==Max ||
        (c.initial!=Stage::Pre && c.initial!=Stage::Post))
        fail("invalid initial identity or clock");
}
void Controller::fail(const char* reason) noexcept {
    if(state_!=State::FailedFsrOnly) failure_=reason;
    state_=State::FailedFsrOnly; token_.reset();
}
Controller::Entry* Controller::find(std::array<Entry,Capacity>& entries,std::size_t count,Id id) {
    for(std::size_t i=0;i<count;++i) if(entries[i].id==id) return &entries[i];
    return nullptr;
}
Controller::Entry* Controller::add(std::array<Entry,Capacity>& entries,std::size_t& count,Id id) {
    if(!id || id==Max || count==Capacity || find(entries,count,id)) {
        fail("invalid, repeated or overflowing ledger ID"); return nullptr;
    }
    entries[count]={id,0,false}; return &entries[count++];
}
void Controller::tick(Id now) {
    if(state_==State::FailedFsrOnly) return;
    if(now<last_time_ || now==Max) { fail("clock regressed or wrapped"); return; }
    last_time_=now;
    if(state_==State::Initializing && !initialization_started_) return;
    if(state_!=State::Active && initialization_deadline_armed_&&now-started_>=config_.deadline_ms) fail("transition deadline");
}
void Controller::close(Id now) { state_=State::Closing; started_=now; }
void Controller::request(Stage stage,Id now) {
    tick(now); if(state_==State::FailedFsrOnly) return;
    if(stage!=Stage::Pre && stage!=Stage::Post) { fail("invalid stage request"); return; }
    requested_=stage;
    if(state_==State::Active && effective_!=stage) close(now);
}
void Controller::set_enabled(bool value) noexcept {
    enabled_=value;
    if(state_==State::Initializing && !initialization_started_ && !value)
        initialization_deadline_armed_=false;
}
void Controller::request_rebuild(Stage stage,Id request_id,Id now) {
    request(stage,now);if(state_==State::FailedFsrOnly)return;
    if(!request_id||request_id<rebuild_request_){fail("stale rebuild request");return;}
    if(request_id==rebuild_request_)return;
    rebuild_request_=request_id;
    if(state_==State::Active)close(now);
}
void Controller::request_mode_change(Id now) {
    tick(now);

    if(state_==State::Active) close(now);
}
void Controller::advance() {
    if(state_!=State::Quiescing || !recording_closed_ || !generation_hip_done_) return;
    if(generation_==Max || serial_==Max) { fail("generation or authorization wrap"); return; }
    if(!token_) token_=Token{generation_,++serial_};
}
void Controller::accept(const Event& e,Id now) {
    tick(now); if(state_==State::FailedFsrOnly) return;
    if(e.generation!=generation_ || e.device!=config_.device || e.queue!=config_.queue) {
        fail("stale generation or identity drift"); return;
    }
    if(!e.success || e.kind==Kind::EvidenceLost) { fail("checked failure or lost evidence"); return; }
    auto all_done=[](const auto& a,std::size_t n) {
        for(std::size_t i=0;i<n;++i) if(!a[i].done) return false;
        return true;
    };
    const bool recording=state_==State::Active || state_==State::Closing || state_==State::WaitingForSubmission ||
        (state_==State::Initializing && !submissions_closed_);
    switch(e.kind) {
    case Kind::RecordedList:
        if(!recording || (state_==State::Initializing && !enabled_) || recording_closed_ || submissions_closed_ || e.id<=last_list_) { fail("record admission closed or reused epoch"); break; }
        if(add(lists_,list_count_,e.id)) {
            last_list_=e.id;
            if(state_==State::Initializing) {initialization_started_=true;if(!initialization_deadline_armed_){started_=now;initialization_deadline_armed_=true;}}
        }
        break;
    case Kind::SubmissionBegin:
        if(!recording || submissions_closed_ || e.id<=last_submission_) { fail("submission closed or reused epoch"); break; }
        if(add(submissions_,submission_count_,e.id)) last_submission_=e.id;
        if(state_==State::Closing) state_=State::WaitingForSubmission;
        break;
    case Kind::SubmitList: {
        auto* list=find(lists_,list_count_,e.id);
        auto* submission=find(submissions_,submission_count_,e.related);
        if(!recording || !list || list->done || !submission || submission->done) { fail("unmatched or duplicate list submission"); break; }
        list->related=e.related; list->done=true; break;
    }
    case Kind::JobPublished: {
        auto* submission=find(submissions_,submission_count_,e.related);
        if(!recording || !submission || submission->done || e.id<=last_job_) { fail("publication outside wrapper or reused job"); break; }
        auto* job=add(jobs_,job_count_,e.id);
        if(job) { job->related=e.related; last_job_=e.id; }
        break;
    }
    case Kind::SubmissionEnd: {
        auto* submission=find(submissions_,submission_count_,e.id);
        if(!recording || !submission || submission->done) { fail("unmatched submission end"); break; }
        submission->done=true; break;
    }
    case Kind::SubmissionsClosed:
        if((state_!=State::Closing && state_!=State::WaitingForSubmission && state_!=State::Initializing) || submissions_closed_ ||
           !all_done(lists_,list_count_) || !all_done(submissions_,submission_count_)) {
            fail("submission closure lacks complete ledger"); break;
        }
        submissions_closed_=true;
        if(state_!=State::Initializing) state_=State::WaitingForCompletion;
        break;
    case Kind::HipCompleted: {
        auto* job=find(jobs_,job_count_,e.id);
        if(!job || job->done || state_==State::Resetting) { fail("unmatched HIP completion"); break; }
        job->done=true; break;
    }
    case Kind::FenceSignaled:
        if((state_!=State::WaitingForCompletion && state_!=State::Initializing) || !submissions_closed_ ||
           fence_ || !e.id || e.id==Max || e.id<=last_retire_fence_) {
            fail("invalid fence signal"); break;
        }
        fence_=e.id; break;
    case Kind::FenceCompleted:
        if((state_!=State::WaitingForCompletion && state_!=State::Initializing) || !fence_ || e.id==Max || e.id!=fence_ || fence_done_) {
            fail("invalid fence completion"); break;
        }
        fence_done_=true; break;
    case Kind::BeginQuiesce:
        if(state_!=State::WaitingForCompletion || !fence_done_ || !all_done(jobs_,job_count_)) {
            fail("producer stop attempted before submission completion"); break;
        }
        state_=State::Quiescing; break;
    case Kind::EnqueuersClosed:
        if(state_!=State::Quiescing || recording_closed_) { fail("invalid producer join proof"); break; }
        recording_closed_=true; break;
    case Kind::GenerationHipCompleted:
        if(state_!=State::Quiescing || !recording_closed_ || generation_hip_done_) { fail("final HIP before producer closure"); break; }
        generation_hip_done_=true; advance(); break;
    case Kind::BeginReset:
        if(state_!=State::Quiescing || !token_ || e.related!=token_->serial) { fail("invalid or reused reset authorization"); break; }
        token_.reset(); state_=State::Resetting; break;
    case Kind::ResetFinished:
        if(state_!=State::Resetting || generation_==Max) { fail("unexpected reset finish"); break; }
        ++generation_; initializing_=requested_; state_=State::Initializing;
        initialization_started_=false; initialization_deadline_armed_=false;
        list_count_=submission_count_=job_count_=0;
        last_list_=last_submission_=last_job_=last_retire_fence_=fence_=0;
        recording_closed_=submissions_closed_=fence_done_=generation_hip_done_=false;
        break;
    case Kind::InitChecked:
        if(state_!=State::Initializing || !list_count_ || !job_count_ || !submissions_closed_ ||
           !fence_done_ || !all_done(jobs_,job_count_)) { fail("initialization lacks checked work"); break; }
        last_retire_fence_=fence_; fence_=0; fence_done_=submissions_closed_=false;
        list_count_=submission_count_=job_count_=0;
        effective_=initializing_; state_=State::Active;
        if(requested_!=initializing_) close(now);
        break;
    case Kind::RetireGraphicsCompleted: {

        if(state_!=State::Active || !list_count_ || !submission_count_ ||
           !e.id || e.id==Max || e.id<=last_retire_fence_ ||
           !all_done(lists_,list_count_) || !all_done(submissions_,submission_count_)) {
            fail("graphics retirement lacks checked closed batch completion"); break;
        }
        last_retire_fence_=e.id;
        list_count_=submission_count_=0;
        std::size_t pending=0;
        for(std::size_t i=0;i<job_count_;++i)
            if(!jobs_[i].done) jobs_[pending++]=jobs_[i];
        job_count_=pending;

        break;
    }
    case Kind::RetireCompleted:
        if(state_==State::Initializing) {
            if(!list_count_ || job_count_ || !submissions_closed_ || !fence_done_ ||
               !fence_ || e.id!=fence_ || !all_done(lists_,list_count_) ||
               !all_done(submissions_,submission_count_)) {
                fail("initial jobless retirement lacks checked sealed work"); break;
            }
            last_retire_fence_=fence_; fence_=0;
            fence_done_=submissions_closed_=false;
            list_count_=submission_count_=0;

            break;
        }
        if(state_!=State::Active || !e.id || e.id==Max || e.id<=last_retire_fence_ ||
           !all_done(lists_,list_count_) || !all_done(submissions_,submission_count_) || !all_done(jobs_,job_count_)) {
            fail("retirement lacks checked closed batch completion"); break;
        }
        last_retire_fence_=e.id;
        list_count_=submission_count_=job_count_=0;
        break;
    case Kind::EvidenceLost: break;
    default: fail("unknown event kind"); break;
    }
}
State Controller::state() const noexcept { return state_; }
Id Controller::generation() const noexcept { return generation_; }
Stage Controller::requested() const noexcept { return requested_; }
Stage Controller::initializing_stage() const noexcept { return initializing_; }
std::optional<Stage> Controller::effective() const noexcept { return effective_; }
std::optional<Token> Controller::reset_authorization() const noexcept { return token_; }
bool Controller::neural_allowed() const noexcept { return enabled_ && state_==State::Active; }
bool Controller::initialization_capture_allowed() const noexcept { return enabled_ && state_==State::Initializing && !submissions_closed_; }
bool Controller::retain_resources() const noexcept { return state_!=State::Resetting; }
const char* Controller::failure() const noexcept { return failure_; }
}
