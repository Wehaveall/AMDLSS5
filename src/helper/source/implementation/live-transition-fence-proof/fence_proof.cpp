#include "fence_proof.h"
#include <limits>
namespace nrfence {
namespace { constexpr Value Max=std::numeric_limits<Value>::max(); }
OwnedFence::OwnedFence(std::unique_ptr<Backend> b,Value generation) noexcept:backend_(std::move(b)) {
    result_.generation=generation;
    if(!backend_ || !generation || generation==Max) { fail("invalid backend or generation"); return; }
    if(!backend_->initialize()) { fail("fresh fence creation or selected DIRECT identity failed"); return; }
    result_.identity=backend_->identity();
    if(!validate()) return;
    if(backend_->completed_value()!=0) { fail("owned fence is not fresh at zero"); return; }
    if(!validate()) return;
    result_.status=Status::Ready;
}
OwnedFence::~OwnedFence() {

    if(result_.status==Status::Pending || result_.status==Status::Failed)
        (void)backend_.release();
}
Result OwnedFence::fail(const char* reason) noexcept {
    if(!*failure_) failure_=reason;
    result_.status=Status::Failed; return result_;
}
bool OwnedFence::validate() noexcept {
    const auto actual=backend_->identity();
    if(!actual.device || !actual.queue || actual.device!=result_.identity.device || actual.queue!=result_.identity.queue) {
        fail("canonical selected identity drift"); return false;
    }
    if(!backend_->healthy()) { fail("device removed or health check failed"); return false; }
    return true;
}
Result OwnedFence::signal(Value target) noexcept {
    if(result_.status==Status::Failed) return result_;
    if(result_.status==Status::Pending) return fail("outstanding target cannot be overwritten");
    if(!target || target==Max || target<=result_.target) return fail("nonmonotonic or wrapped target");
    if(!validate()) return result_;
    result_.target=target;

    result_.status=Status::Pending;
    if(!backend_->signal(target)) return fail("queue Signal HRESULT failed");
    if(!validate()) return result_;
    return result_;
}
Result OwnedFence::poll() noexcept {
    if(result_.status==Status::Failed) return result_;
    if(!validate()) return result_;
    if(result_.status==Status::Ready) return result_;
    const Value value=backend_->completed_value();
    last_read_=value;
    if(!validate()) return result_;
    if(value==Max || value>result_.target || value<last_observed_) return fail("removed, foreign or regressed fence value");
    last_observed_=value;
    result_.status=value==result_.target?Status::Completed:Status::Pending;
    return result_;
}
bool OwnedFence::arm(void* event) noexcept {
    if(!event || result_.status!=Status::Pending || !backend_) return false;
    return backend_->arm_event(result_.target,event);
}
Result OwnedFence::snapshot() const noexcept { return result_; }
const char* OwnedFence::failure() const noexcept { return failure_; }
long OwnedFence::removed_reason() noexcept { return backend_?backend_->removed_reason():0; }
}
