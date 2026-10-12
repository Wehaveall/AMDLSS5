#include "observer_platform.h"
#include "compute_state.h"
#include <atomic>
#include <limits>
#include <cstdio>
#include <cstring>
namespace nrrecord {
Ledger::Ledger(std::uint32_t owner) noexcept:owner_(owner){}
void Ledger::fail_locked(const char* why) noexcept {if(state_.phase!=Phase::Failed){state_.phase=Phase::Failed;state_.diagnostic=why;}}
void Ledger::fail(const char* why) noexcept {std::lock_guard<std::mutex> lock(mutex_);state_.phase=Phase::Failed;state_.diagnostic=why;}
Snapshot Ledger::snapshot() noexcept {std::lock_guard<std::mutex> lock(mutex_);auto s=state_;s.busy=busy_;return s;}
bool Ledger::begin(Method method,std::uint32_t thread) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if(state_.phase==Phase::Failed)return false;
    if(!owner_||!thread||busy_){fail_locked("invalid_or_overlapping_method");return false;}
    busy_=true;method_=method;method_thread_=thread;return true;
}
void Ledger::end(Method method,std::uint32_t thread,std::int32_t result) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if(!busy_||method!=method_||!thread||thread!=method_thread_){fail_locked("method_completion_mismatch");return;}
    busy_=false;method_thread_=0;if(state_.phase==Phase::Failed)return;
    if(result<0){fail_locked("recording_API_failed");return;}
    if(method==Method::Reset){
        if((state_.neural&&!state_.submitted)||state_.phase==Phase::Open){fail_locked("reset_discarded_unclosed_or_unsubmitted_recording");return;}
        if(state_.epoch==UINT64_MAX||state_.resets==UINT64_MAX){fail_locked("recording_epoch_wrap");return;}
        ++state_.epoch;++state_.resets;state_.phase=Phase::Open;state_.neural=state_.submitted=false;state_.generation=0;state_.diagnostic="observed_open";
    }else{
        if(state_.closes==UINT64_MAX){fail_locked("close_count_wrap");return;}++state_.closes;
        if(state_.phase==Phase::Unobserved)return;
        if(state_.phase!=Phase::Open){fail_locked("close_without_open");return;}
        state_.phase=Phase::Closed;state_.primed=true;state_.diagnostic="observed_closed";
    }
}
bool Ledger::mark_neural(std::uint64_t generation) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if(state_.phase==Phase::Failed)return false;
    if(busy_||!state_.primed||state_.phase!=Phase::Open||!generation||(state_.neural&&state_.generation!=generation)){fail_locked("neural_recording_identity_failed");return false;}
    state_.neural=true;state_.generation=generation;return true;
}
bool Ledger::mark_submitted(std::uint64_t epoch) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if(state_.phase==Phase::Failed)return false;
    if(busy_||state_.phase!=Phase::Closed||epoch!=state_.epoch||!state_.neural||state_.submitted){fail_locked("neural_submission_identity_or_replay");return false;}
    state_.submitted=true;return true;
}
bool Ledger::can_retire(std::uint64_t generation) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return generation&&!busy_&&state_.phase!=Phase::Failed&&state_.phase!=Phase::Unobserved&&!(state_.neural&&state_.generation==generation);
}
bool Ledger::retirable_if_abandoned(std::uint64_t generation) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if(!generation||busy_||state_.phase==Phase::Failed||state_.phase==Phase::Unobserved)return false;
    return !(state_.neural&&state_.generation==generation)||(state_.submitted&&state_.phase==Phase::Closed);
}
namespace detail {
namespace {thread_local const char* refusal{};}
bool refuse(const char* why) noexcept {refusal=why;return false;}
namespace {
struct Endpoint {
    const std::unique_ptr<Platform> platform;
    void* const command;
    const Targets originals;
    const std::uint32_t owner;
    Ledger ledger;
    std::atomic<bool> active{false};
    std::atomic<Endpoint*> next{nullptr};
    char installation_diagnostic[768]{};
    Endpoint(std::unique_ptr<Platform> p,void* c,Targets t,std::uint32_t o):platform(std::move(p)),command(c),originals(t),owner(o),ledger(o){}
};
std::atomic<Endpoint*> published{};
std::mutex install_mutex;
Endpoint& endpoint_for(void* command) noexcept {
    auto* root=published.load();
    for(auto* e=root;e;e=e->next.load())if(e->command==command)return *e;
    return *root;
}
std::int32_t close_wrapper(void* self);
std::int32_t reset_wrapper(void* self,void* allocator,void* initial);
void* close_address() noexcept{return reinterpret_cast<void*>(&close_wrapper);}
void* reset_address() noexcept{return reinterpret_cast<void*>(&reset_wrapper);}
bool valid(Endpoint& e) noexcept {
    if(!e.active.load()||!e.platform->intact(e.command,e.originals.table,close_address(),reset_address())){

        if(e.ledger.snapshot().phase!=Phase::Failed){
            const char* why=e.active.load()?e.platform->drift_reason(e.command,e.originals.table):nullptr;
            e.ledger.fail(why?why:"observer_inactive_or_slot_drift");
        }
        return false;
    }return true;
}
std::int32_t close_wrapper(void* self){
    auto& e=endpoint_for(self);const bool tracked=self==e.command;
    const auto thread=tracked?e.platform->thread():0;
    const bool began=tracked&&valid(e)&&e.ledger.begin(Method::Close,thread);
    try{const auto result=e.originals.close(self);if(began)e.ledger.end(Method::Close,thread,result);return result;}
    catch(...){if(tracked)e.ledger.fail("original_Close_exception");throw;}
}
std::int32_t reset_wrapper(void* self,void* allocator,void* initial){
    auto& e=endpoint_for(self);const bool tracked=self==e.command;
    const auto thread=tracked?e.platform->thread():0;
    const bool began=tracked&&valid(e)&&e.ledger.begin(Method::Reset,thread);
    try{const auto result=e.originals.reset(self,allocator,initial);if(began){e.ledger.end(Method::Reset,thread,result);if(result>=0)nrstate::reset(self,initial);}return result;}
    catch(...){if(tracked)e.ledger.fail("original_Reset_exception");throw;}
}
class View final:public Observer {
    Endpoint& e_;
public:
    explicit View(Endpoint& e):e_(e){}
    Snapshot snapshot() noexcept override{
        const bool checked=valid(e_);auto s=e_.ledger.snapshot();
        if(!checked&&s.phase!=Phase::Failed){s.phase=Phase::Failed;s.diagnostic="observer_validation_failed";}
        return s;
    }
    bool mark_neural(std::uint64_t g) noexcept override{return valid(e_)&&e_.ledger.mark_neural(g);}
    bool mark_submitted(std::uint64_t epoch) noexcept override{return valid(e_)&&e_.ledger.mark_submitted(epoch);}
    bool can_retire(std::uint64_t g) noexcept override{
        if(!valid(e_)||!e_.ledger.can_retire(g))return false;
        for(auto* e=published.load();e;e=e->next.load()){
            const auto s=e->ledger.snapshot();
            if(!valid(*e)||s.busy||(s.neural&&s.generation==g))return false;
        }
        return true;
    }
    bool retirable_if_abandoned(std::uint64_t g) noexcept override{
        if(!valid(e_)||!e_.ledger.retirable_if_abandoned(g))return false;
        for(auto* e=published.load();e;e=e->next.load()){
            const auto s=e->ledger.snapshot();
            if(!valid(*e)||s.busy||s.phase==Phase::Failed)return false;
            if(s.neural&&s.generation==g&&!(s.submitted&&s.phase==Phase::Closed))return false;
        }
        return true;
    }
};
}
std::unique_ptr<Observer> install(std::unique_ptr<Platform> platform,void* command,std::uint64_t device,std::uint32_t owner) noexcept {
    refusal=nullptr;
    try{
        std::lock_guard<std::mutex> lock(install_mutex);
        Targets targets{};
        if(!platform){refuse("install_null_platform");return {};}
        if(!platform->prepare(command,device,owner,targets)){if(!refusal)refuse("prepare_refused");return {};}
        if(!targets.table||!targets.close||!targets.reset){refuse("install_targets_null");return {};}
        if(auto* root=published.load()){
            if(!valid(*root)){refuse("install_root_invalid");return {};}
            if(targets.table!=root->originals.table){

                if(!platform->private_table()){refuse("install_vtable_differs_from_root");return {};}
                const auto root_offset=reinterpret_cast<std::uintptr_t>(root->originals.table)-reinterpret_cast<std::uintptr_t>(root->command);
                if(!root->platform->private_table()||
                   !same_private_offset(reinterpret_cast<std::uintptr_t>(command),reinterpret_cast<std::uintptr_t>(targets.table),root_offset)){
                    refuse("install_private_offset_differs_from_root");return {};
                }
                auto* last=root;unsigned count=0;
                for(auto* e=root;e;e=e->next.load()){
                    if(e->command==command){refuse("install_known_list_vtable_changed");return {};}
                    last=e;++count;
                }
                if(count>=64){refuse("install_endpoint_cap_64");return {};}

                if(targets.close!=root->originals.close||targets.reset!=root->originals.reset){refuse("install_private_originals_differ_from_root");return {};}
                const Targets own{targets.table,root->originals.close,root->originals.reset};
                auto endpoint=std::make_unique<Endpoint>(std::move(platform),command,own,owner);
                auto& e=*endpoint;std::uint32_t previous=0;
                if(!e.platform->writable(own.table,previous)){refuse("install_private_writable_failed");return {};}
                const bool first=e.platform->exchange(own.table+9,reinterpret_cast<void*>(own.close),close_address());
                const bool second=first&&e.platform->exchange(own.table+10,reinterpret_cast<void*>(own.reset),reset_address());
                const bool restored=e.platform->restore(own.table,previous);
                if(first&&second&&restored&&e.platform->intact(command,own.table,close_address(),reset_address())){
                    auto view=std::make_unique<View>(e);
                    e.active.store(true);last->next.store(endpoint.release());
                    return view;
                }

                std::uint32_t rollback_previous=0;
                if(e.platform->writable(own.table,rollback_previous)){
                    e.platform->exchange(own.table+9,close_address(),reinterpret_cast<void*>(own.close));
                    e.platform->exchange(own.table+10,reset_address(),reinterpret_cast<void*>(own.reset));
                    e.platform->restore(own.table,rollback_previous);
                }
                refuse("install_private_patch_failed");return {};
            }
            if(reinterpret_cast<void*>(targets.close)!=close_address()||reinterpret_cast<void*>(targets.reset)!=reset_address()){refuse("install_root_slots_not_wrappers");return {};}
            auto* last=root;unsigned count=0;
            for(auto* e=root;e;e=e->next.load()){
                if(e->command==command)return std::make_unique<View>(*e);
                last=e;++count;
            }
            if(count>=64){refuse("install_endpoint_cap_64");return {};}
            auto endpoint=std::make_unique<Endpoint>(std::move(platform),command,root->originals,owner);
            if(!endpoint->platform->intact(command,targets.table,close_address(),reset_address())){refuse("install_new_endpoint_not_intact");return {};}
            auto view=std::make_unique<View>(*endpoint);
            endpoint->active.store(true);last->next.store(endpoint.release());
            return view;
        }
        auto endpoint=std::make_unique<Endpoint>(std::move(platform),command,targets,owner);
        auto view=std::make_unique<View>(*endpoint);Endpoint* none=nullptr;
        if(!published.compare_exchange_strong(none,endpoint.get())){refuse("install_publish_race");return {};}
        auto& e=*endpoint.release();
        std::uint32_t previous=0;
        if(!e.platform->writable(targets.table,previous)){e.ledger.fail("table_writable_failed");return view;}
        const bool first=e.platform->exchange(targets.table+9,reinterpret_cast<void*>(targets.close),close_address());
        const bool second=first&&e.platform->exchange(targets.table+10,reinterpret_cast<void*>(targets.reset),reset_address());
        const bool restored=e.platform->restore(targets.table,previous);
        const bool checked=first&&second&&restored;
        const bool intact=checked&&e.platform->intact(command,targets.table,close_address(),reset_address());
        if(intact){e.active.store(true);return view;}
        std::snprintf(e.installation_diagnostic,sizeof(e.installation_diagnostic),
            "slot_install_first_%u_second_%u_restored_%u_intact_checked_%u_intact_%u_%s",
            first?1u:0u,second?1u:0u,restored?1u:0u,checked?1u:0u,intact?1u:0u,e.platform->diagnostic());
        e.ledger.fail(e.installation_diagnostic);
        auto rollback_failure=[&](const char* reason){
            const auto length=std::strlen(e.installation_diagnostic);
            std::snprintf(e.installation_diagnostic+length,sizeof(e.installation_diagnostic)-length,"_%s",reason);
            e.ledger.fail(e.installation_diagnostic);
        };
        std::uint32_t rollback_previous=0;
        if(e.platform->writable(targets.table,rollback_previous)){
            e.platform->exchange(targets.table+9,close_address(),reinterpret_cast<void*>(targets.close));
            e.platform->exchange(targets.table+10,reset_address(),reinterpret_cast<void*>(targets.reset));
            if(!e.platform->restore(targets.table,previous))rollback_failure("rollback_protection_restore_failed");
        }else rollback_failure("rollback_writable_failed");
        return view;
    }catch(...){refuse("install_exception");return {};}
}
}
const char* last_refusal() noexcept {return detail::refusal;}
bool forget(void* command) noexcept {
    try{
        std::lock_guard<std::mutex> lock(detail::install_mutex);
        auto* root=detail::published.load();if(!root||!command||root->command==command)return false;
        for(auto* prev=root;auto* e=prev->next.load();prev=e){
            if(e->command!=command)continue;
            if(e->ledger.snapshot().busy)return false;

            e->active.store(false);prev->next.store(e->next.load());e->platform->release();
            return true;
        }
        return false;
    }catch(...){return false;}
}
std::size_t endpoints() noexcept {std::size_t n=0;for(auto* e=detail::published.load();e;e=e->next.load())++n;return n;}
}
