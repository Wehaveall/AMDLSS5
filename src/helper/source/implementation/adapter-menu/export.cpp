#include "adapter_menu.h"
#include "../pipeline-api.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
extern "C" __declspec(dllexport) void NRMenuAppend_v1(void* base) noexcept {
    try {
        HMODULE self=nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&NRMenuAppend_v1),&self)) return;
        (void)nradapter::append_from_base(base);
    } catch (...) { OutputDebugStringA("NR menu: export rejected C++ exception\n"); }
}
namespace {
std::int32_t passive_dispatch(void* base,nrabi::FfxDispatch original,void* context,
    void* descriptor,std::int32_t nesting,std::uint32_t* handled) {
    if(handled)*handled=0;

    if(!original)return 6;
    (void)nesting;
    (void)nradapter::runtime_snapshot(base);

    return original(context,descriptor);
}
std::uint32_t passive_compose(void* base,void*,void*,std::uint32_t,std::int32_t,float,std::int32_t,std::uint32_t) {
    (void)nradapter::runtime_snapshot(base);
    return 0;
}
void passive_tick(void* base) { (void)nradapter::runtime_snapshot(base); }
void passive_reload(void* base) { (void)nradapter::runtime_reload(base); }
constexpr nrabi::PipelineApiV1 passive_api{40,1,passive_dispatch,passive_compose,passive_tick,passive_reload};
}
extern "C" __declspec(dllexport) const nrabi::PipelineApiV1* NRGetPipelineApi_v1() noexcept {

    return &passive_api;
}
