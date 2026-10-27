#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dxgi.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include "fps_counter.h"

namespace nrmenufps {
namespace {
struct State {
    void* base{};void* chain{};unsigned width{},height{};
    LONGLONG last_poll{},input_time{},display_time{},last_fg{};
    UINT input_count{},fg_count{},display_count{};
    bool input_seeded{},display_seeded{},fg_seen{};
    double input_fps{},display_fps{};
    bool input_valid{},display_valid{};
    Display text{};
};
std::array<State,4> states{};
std::mutex mutex;
LONGLONG frequency() noexcept {
    static const LONGLONG value=[] { LARGE_INTEGER f{};return QueryPerformanceFrequency(&f)?f.QuadPart:0; }();
    return value;
}

bool plausible(UINT delta,double seconds) noexcept {
    return seconds>0&&seconds<=5&&double(delta)/seconds<=10000;
}
}
Display observe(void* base,void* swapchain,unsigned width,unsigned height) noexcept {
    try {
        const auto hz=frequency();LARGE_INTEGER now{};
        if(!base||!swapchain||!hz||!QueryPerformanceCounter(&now))return {};
        std::lock_guard<std::mutex> guard(mutex);
        State* selected{};
        for(auto& s:states)if(s.base==base&&s.chain==swapchain){selected=&s;break;}
        if(!selected){
            for(auto& s:states)if(!s.chain){selected=&s;break;}
            if(!selected){selected=&states[0];for(auto& s:states)if(s.last_poll<selected->last_poll)selected=&s;}
            *selected=State{};selected->base=base;selected->chain=swapchain;
        }
        auto& s=*selected;
        if(s.width!=width||s.height!=height||now.QuadPart<s.last_poll||
           (s.last_poll&&now.QuadPart-s.last_poll>5*hz)){
            s=State{};s.base=base;s.chain=swapchain;s.width=width;s.height=height;
        }

        if(s.last_poll&&now.QuadPart-s.last_poll<hz/4)return s.text;
        s.last_poll=now.QuadPart;
        auto* image=static_cast<unsigned char*>(base);

        const UINT frames=*reinterpret_cast<volatile const UINT*>(image+0x949e0);
        const UINT fg=*reinterpret_cast<volatile const UINT*>(image+0x949ec);
        if(!s.input_seeded){
            s.input_seeded=true;s.input_count=frames;s.fg_count=fg;s.input_time=now.QuadPart;
        }else{
            const UINT fg_delta=fg-s.fg_count;s.fg_count=fg;
            if(fg_delta&&fg_delta<10000){s.last_fg=now.QuadPart;s.fg_seen=true;}
            else if(fg_delta>=10000){s.last_fg=0;s.fg_seen=false;}
            const double seconds=double(now.QuadPart-s.input_time)/double(hz);
            if(seconds>=1){
                const UINT delta=frames-s.input_count;
                s.input_valid=plausible(delta,seconds);
                if(s.input_valid)s.input_fps=double(delta)/seconds;
                s.input_count=frames;s.input_time=now.QuadPart;
            }
        }
        DXGI_FRAME_STATISTICS stats{};
        const HRESULT result=static_cast<IDXGISwapChain*>(swapchain)->GetFrameStatistics(&stats);
        if(FAILED(result)||stats.SyncQPCTime.QuadPart<=0){

            s.display_seeded=false;s.display_valid=false;
        }else if(!s.display_seeded||stats.SyncQPCTime.QuadPart<s.display_time){
            s.display_seeded=true;s.display_valid=false;
            s.display_count=stats.PresentCount;s.display_time=stats.SyncQPCTime.QuadPart;
        }else{
            const double seconds=double(stats.SyncQPCTime.QuadPart-s.display_time)/double(hz);
            if(seconds>=1){
                const UINT delta=stats.PresentCount-s.display_count;
                s.display_valid=plausible(delta,seconds);
                if(s.display_valid)s.display_fps=double(delta)/seconds;
                s.display_count=stats.PresentCount;s.display_time=stats.SyncQPCTime.QuadPart;
            }

            if(now.QuadPart-stats.SyncQPCTime.QuadPart>2*hz){s.display_valid=false;s.display_seeded=false;}
        }
        const bool active=s.last_fg&&now.QuadPart-s.last_fg<=2*hz;
        s.text.display_fps=s.display_fps;s.text.input_fps=s.input_fps;s.text.display_valid=s.display_valid;
        s.text.input_valid=s.input_valid;s.text.read_ok=true;s.text.fg=active?2:s.fg_seen?1:0;
        return s.text;
    }catch(...){return {};}
}
}
