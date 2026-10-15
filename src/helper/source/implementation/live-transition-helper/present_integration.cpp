#include "../../../runtime/nr_publico.h"
#include "present_integration.h"
#include "../present-runtime/present_state.h"
#include "../present-runtime/native_present.h"
#include "../present-runtime/present_transport.h"
#include "../adapter-menu/host_menu.h"
#include "multipass_math.h"
#include "tone_control.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <cstring>
#include <mutex>

namespace nrpresentintegration {
namespace {
std::filesystem::path folder(void* base) {
    wchar_t text[32768]{};
    const auto length=GetModuleFileNameW(static_cast<HMODULE>(base),text,32768);
    return length&&length<32768?std::filesystem::path(text).parent_path():std::filesystem::path{};
}
template<class T>T value(void* base,std::size_t offset) {
    T result{};auto* pointer=static_cast<char*>(base)+offset;
    if(!nradapter::accessible(pointer,sizeof(T),nradapter::Access::Read))throw std::runtime_error("proxy field unavailable");
    std::memcpy(&result,pointer,sizeof(result));return result;
}
struct Session {
    nrpresentnative::Engine engine;
    void* owner{};
    std::mutex mutex;
    std::uint64_t completed{};
    std::uint64_t last_logged{};
    std::string previous;
};
Session& session(){static auto* result=new Session;return *result;}
struct Call {Session& state;void* base;nrpresentnative::Controls controls;};
bool evaluate(void* opaque,const std::uint16_t* input,std::uint16_t* output,unsigned w,unsigned h,std::string& error){
    auto& call=*static_cast<Call*>(opaque);
    return call.state.engine.evaluate(call.base,input,output,w,h,call.controls,error);
}
void log(Session& state,void* base,const std::string& text,double elapsed=0){
    const auto key=text.substr(0,text.find(':'));
    if(key==state.previous && (state.completed<state.last_logged+120))return;
    const auto path=folder(base);if(path.empty())return;
    std::ofstream out(path/L"nr_present.log",std::ios::app);
    out<<"pid="<<GetCurrentProcessId()<<" mode=Present source=swapchain temporal=off completed="<<state.completed
       <<" elapsed_ms="<<elapsed<<" "<<text<<'\n';
    state.previous=key;state.last_logged=state.completed;
}
}
bool selected(void* base) noexcept {
#ifdef NR_PRESENT_RUNTIME
    if(!base)return false;
    try {
        if(!nrpresentmode::startup.load(std::memory_order_acquire)){
            const auto path=folder(base);if(path.empty())return false;
            nrpresentmode::initialize(path/L"dlssnr_on_amd.ini");
        }
        return nrpresentmode::active();
    }catch(...){return false;}
#else
    (void)base;return false;
#endif
}
void process(void* base,void* swapchain,void* queue) noexcept {
    if(!selected(base))return;
    auto& state=session();std::unique_lock<std::mutex> lock(state.mutex,std::try_to_lock);
    if(!lock.owns_lock())return;
    try{
        if(state.owner&&state.owner!=base)return;
        state.owner=base;
        if(nrpresentmode::status.load()==4)return;
        if(!value<unsigned char>(base,0x94a8c)){
            nrpresentmode::status.store(2);log(state,base,"disabled");return;
        }
        if(!queue||!swapchain){nrpresentmode::status.store(0);log(state,base,"waiting:exact_present_queue");return;}

        if(value<unsigned char>(base,0x94a8e)!=1){
            nrpresentmode::status.store(4);log(state,base,"failed:UseFsrInputs_must_be_1");return;
        }
        Microsoft::WRL::ComPtr<ID3D12Device> device;
        if(FAILED(static_cast<IDXGISwapChain*>(swapchain)->GetDevice(IID_PPV_ARGS(&device)))){
            nrpresentmode::status.store(3);log(state,base,"unsupported:non_D3D12_swapchain");return;
        }
        auto luid=device->GetAdapterLuid();std::uint64_t packed{};std::memcpy(&packed,&luid,sizeof(packed));
        state.engine.set_adapter_luid(packed);
        Call call{state,base,{}};
        call.controls.structure=value<float>(base,0x94aa4);
        call.controls.skin=value<float>(base,0x94aa8);
        call.controls.auto_mask=value<unsigned>(base,0x94ab0)!=0;
        call.controls.style=value<unsigned>(base,0x71d080);
        const auto config=nrmp::valid_config(value<unsigned>(base,0x71d090));
        call.controls.passes=config&255u;call.controls.strength=nrmp::multiplier(config);
        if(call.controls.strength==0){
            nrpresentmode::status.store(2);log(state,base,"disabled:zero_strength");return;
        }
        const auto tone=nrtone::state.requested.load();
        call.controls.tone=tone?float(tone-1)*.25f:value<float>(base,0x94aa0);
        call.controls.tone_channels=tone!=0||value<unsigned>(base,0x94ab4)!=0;
        std::string diagnostic;
        const auto start=std::chrono::steady_clock::now();
        const bool completed=nrpresentgpu::process(swapchain,queue,evaluate,&call,diagnostic);
        if(completed){
            ++state.completed;nrpresentmode::status.store(1);
            InterlockedExchange(reinterpret_cast<volatile LONG*>(static_cast<char*>(base)+0x71d084),static_cast<LONG>(call.controls.style));
            InterlockedExchange(reinterpret_cast<volatile LONG*>(static_cast<char*>(base)+0x71d094),static_cast<LONG>(config));
            nrtone::state.effective.store(tone);
        }else nrpresentmode::status.store(diagnostic.rfind("unsupported:",0)==0?3u:4u);
        log(state,base,diagnostic,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
    }catch(const std::exception& e){nrpresentmode::status.store(4);log(state,base,std::string("failed:")+e.what());}
    catch(...){nrpresentmode::status.store(4);log(state,base,"failed:unexpected_exception");}
}
}
