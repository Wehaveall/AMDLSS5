#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "../../../runtime/nr_publico.h"
#include "fg_tempo.h"
#include "fg_pacing.h"
#include "fg_config.h"
#include "tela_captura.h"
#include "../../../runtime/native_ini_path.h"
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <atomic>
#include "present_scope.h"
#include "present_integration.h"
#include "menu_input.h"
#include "menu_layout.h"
#include "../../../layout/menu_compacto.h"
#include "../live-transition-binding/live_binding.h"
#include "../../../runtime/rosto_jogador_state.h"
#include "present_trace.h"
#include "../present-runtime/print_capture.h"
#include "../present-runtime/rec_aviso.h"
#include "../../../runtime/overlay_amd.h"
namespace {

int observe(unsigned code,void* address,const char* where) noexcept {
    static std::atomic<unsigned> count{0};
    if(count.fetch_add(1)<16)nrpresenttrace::line("exception where=%s code=0x%08x address=%p",where,code,address);
    return EXCEPTION_CONTINUE_SEARCH;
}
__declspec(noinline) void native_present(void* base,void* swapchain){
    using Native=void(__cdecl*)(void*);
    __try{reinterpret_cast<Native>(static_cast<std::uint8_t*>(base)+0x9990)(swapchain);}
    __except(observe(GetExceptionCode(),GetExceptionInformation()->ExceptionRecord->ExceptionAddress,"present_0x9990")){}
}

__declspec(noinline) void print_capture(void* swapchain,void* queue){
    __try{nrprint::capture(swapchain,queue);}
    __except(observe(GetExceptionCode(),GetExceptionInformation()->ExceptionRecord->ExceptionAddress,"print_capture")){}
}
__declspec(noinline) void native_overlay(void* base,void* swapchain,void* actual,const char* title){
    using Native=void(__cdecl*)(void*,void*,const char*);
    __try{reinterpret_cast<Native>(static_cast<std::uint8_t*>(base)+0x1f880)(swapchain,actual,title);}
    __except(observe(GetExceptionCode(),GetExceptionInformation()->ExceptionRecord->ExceptionAddress,"overlay_0x1f880")){}
}
}

extern "C" __declspec(dllexport) void NRPresent_v1(void* base,void* swapchain){
    (void)nrpresentintegration::selected(base);
    nrpresent::Scope scope(base,swapchain);
    auto& trace=nrpresenttrace::state();
    const auto n=trace.presents.fetch_add(1,std::memory_order_relaxed)+1;
    const bool first=nrpresenttrace::first(trace.present_seen,swapchain);
    if(first)nrpresenttrace::line("present_enter_first swapchain=%p n=%llu",swapchain,static_cast<unsigned long long>(n));
    nrfgtempo::evento('P',0);
    {static bool feito=false;if(!feito){feito=true;const auto ini_fg=nrinipath::resolve(base,reinterpret_cast<const void*>(&NRPresent_v1));nrfgpacing::instalar(ini_fg);nrfgconfig::instalar(ini_fg);}}
    LARGE_INTEGER t0{},t1{},fq{};QueryPerformanceCounter(&t0);
    native_present(base,swapchain);
    nrrj::presents_fim.fetch_add(1,std::memory_order_release);
    QueryPerformanceCounter(&t1);QueryPerformanceFrequency(&fq);
    const double ms=fq.QuadPart?double(t1.QuadPart-t0.QuadPart)*1000.0/double(fq.QuadPart):0.0;
    if(first)nrpresenttrace::line("present_exit_first swapchain=%p ms=%.2f",swapchain,ms);
    else if(ms>250.0)nrpresenttrace::line("present_slow swapchain=%p ms=%.1f n=%llu",swapchain,ms,static_cast<unsigned long long>(n));
    nrpresenttrace::pulse(n,swapchain);
}
extern "C" __declspec(dllexport) void NRPresentQueue_v1(void* base,void* swapchain,void* queue) noexcept {
    nrpresent::Scope::record(base,swapchain,queue);
    {auto& trace=nrpresenttrace::state();trace.queues.fetch_add(1,std::memory_order_relaxed);
     if(nrpresenttrace::first(trace.queue_seen,swapchain))nrpresenttrace::line("present_queue_first swapchain=%p queue=%p",swapchain,queue);}

    nrlive::note_present_queue(reinterpret_cast<std::uint64_t>(queue));
}
extern "C" __declspec(dllexport) void NROverlay_v1(void* base,void* swapchain,void* supplied,const char* title){
    auto* selected=nrpresent::Scope::selected(base,swapchain);
    {auto& trace=nrpresenttrace::state();trace.overlays.fetch_add(1,std::memory_order_relaxed);
     if(nrpresenttrace::first(trace.overlay_seen,swapchain))nrpresenttrace::line("overlay_first swapchain=%p supplied=%p selected=%p",swapchain,supplied,selected);}
    if(nrpresent::Scope::claim(base,swapchain))nrpresentintegration::process(base,swapchain,selected);

    auto* actual=selected?selected:supplied;
    static std::mutex mutex;
    {std::lock_guard<std::mutex> lock(mutex);
     static unsigned count{};static void* last_supplied{};static void* last_actual{};
     if(count<100&&(count==0||last_supplied!=supplied||last_actual!=actual)){

++count;last_supplied=supplied;last_actual=actual;
     }}
    print_capture(swapchain,actual);
    nraviso::desenhar(swapchain,actual);
    nrmenulayout::Scope layout(base,swapchain);
    {
        const int m=nrmenucompacto::modo.load(std::memory_order_relaxed);
        const bool fechado=*reinterpret_cast<volatile std::uint8_t*>(static_cast<std::uint8_t*>(base)+0x923B0)==0;
        if(!fechado&&!nrmenucompacto::desenhando.load(std::memory_order_relaxed))nrmenucompacto::ja_aberto.store(true,std::memory_order_relaxed);

        nrmenucompacto::pedido=fechado&&(nrmenucompacto::ja_aberto.load(std::memory_order_relaxed)||m==2)&&
            (m==2||(m==1&&nrmenucompacto::recolhido.load(std::memory_order_relaxed)));
    }
    native_overlay(base,swapchain,actual,title);
    if(nrmenucompacto::desenhando.load(std::memory_order_relaxed)){
        auto* b=static_cast<std::uint8_t*>(base);
        *reinterpret_cast<volatile std::uint8_t*>(b+0x923B0)=0;
        nrmenucompacto::desenhando.store(false,std::memory_order_relaxed);
        if(!nrmenucompacto::ja_aberto.load(std::memory_order_relaxed)){
            if(void* obj=*reinterpret_cast<void* volatile*>(b+0x92440)){int novo=0;using Aviso=void(__fastcall*)(void*,int*);(*reinterpret_cast<Aviso**>(obj))[2](obj,&novo);}
            nrmenucompacto::ja_aberto.store(true,std::memory_order_relaxed);
        }
    }
    nrmenucompacto::pedido=false;
    {static const std::wstring ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&NRPresent_v1));nrtela::capturar(swapchain,actual,ini);}
    nrmenuinput::tick(base);
    nroverlayamd::tick(base,reinterpret_cast<const void*>(&NRPresent_v1));
}
