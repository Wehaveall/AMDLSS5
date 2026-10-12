#include "hip_proof.h"
#include <atomic>
#include <process.h>

namespace nrtransition {
namespace {
bool pin_executable(const void* address) noexcept {
    MEMORY_BASIC_INFORMATION info{};
    if(!address || !VirtualQuery(address,&info,sizeof(info)) ||
       info.Type!=MEM_IMAGE || info.State!=MEM_COMMIT ||
       (info.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    const auto protection=info.Protect&0xff;
    if(protection!=PAGE_EXECUTE && protection!=PAGE_EXECUTE_READ &&
       protection!=PAGE_EXECUTE_READWRITE && protection!=PAGE_EXECUTE_WRITECOPY)return false;
    HMODULE module{};
    return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        static_cast<LPCWSTR>(address),&module)!=FALSE;
}
}
struct HipProof::Shared {
    HipApi api;
    HipResult result;
    std::atomic<bool> finished{false};
    explicit Shared(HipApi a):api(a) {result.device=a.device;result.generation=a.generation;}
};
unsigned __stdcall HipProof::run(void* payload) noexcept {
    const std::unique_ptr<std::shared_ptr<Shared>> owner(static_cast<std::shared_ptr<Shared>*>(payload));
    auto& s=**owner;
    s.result.host_error=ERROR_SUCCESS;
    try {
        s.result.set_device_status=s.api.set_device(s.api.device);
        if(s.result.set_device_status==0){
            s.result.synchronize_status=s.api.synchronize();
            if(s.result.synchronize_status==0)s.result.state=HipState::Succeeded;
        }
    }catch(...){s.result.host_error=ERROR_UNHANDLED_EXCEPTION;}
    s.finished.store(true,std::memory_order_release);
    return 0;
}
HipProof::HipProof(HipApi api) noexcept {
    try {
        state_=std::make_shared<Shared>(api);
        if(api.device<0 || !api.generation || api.generation==UINT64_MAX ||
           !pin_executable(reinterpret_cast<const void*>(api.set_device)) ||
           !pin_executable(reinterpret_cast<const void*>(api.synchronize)) ||
           !pin_executable(reinterpret_cast<const void*>(&HipProof::run))){
            state_->finished.store(true,std::memory_order_release);return;
        }
        auto owner=std::make_unique<std::shared_ptr<Shared>>(state_);
        const auto thread=_beginthreadex(nullptr,0,&HipProof::run,owner.get(),0,nullptr);
        if(!thread){
            state_->result.host_error=ERROR_NOT_ENOUGH_MEMORY;
            state_->finished.store(true,std::memory_order_release);return;
        }
        (void)owner.release();
        CloseHandle(reinterpret_cast<HANDLE>(thread));
    }catch(...){
        if(state_){state_->result.host_error=ERROR_NOT_ENOUGH_MEMORY;state_->finished.store(true,std::memory_order_release);}
    }
}
HipResult HipProof::poll() const noexcept {
    if(!state_)return {};
    if(state_->finished.load(std::memory_order_acquire))return state_->result;
    HipResult pending{};pending.state=HipState::Pending;
    pending.device=state_->api.device;pending.generation=state_->api.generation;
    pending.host_error=ERROR_SUCCESS;
    return pending;
}
}
