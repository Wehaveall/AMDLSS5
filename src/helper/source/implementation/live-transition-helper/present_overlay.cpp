#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "../../../runtime/nr_publico.h"
#include "../../../runtime/proxy_layout.h"
#include "fg_tempo.h"
#include "fg_pacing.h"
#include "fg_config.h"
#include "tela_captura.h"
#include "../../../runtime/native_ini_path.h"
#include <windows.h>
#include <dxgi.h>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <atomic>
#include "../universal-scale/coordinator.h"
#include "../async-reprojection/reproject_controls.h"
#include "../compat-diag/compat.h"
#include "fsr2_gancho.h"
#include "../../../runtime/fsr_intermitente.h"
#include "../../../runtime/jogo_conhecido.h"
#include "../../../runtime/log_fila.h"
namespace {

void conferir_fsr_intermitente(void* base) noexcept {
    nrfsrint::presents.fetch_add(1,std::memory_order_relaxed);
    int modo=nrfsrint::modo.load(std::memory_order_relaxed);
    if(modo<0){
        static std::atomic<bool> lendo{false};if(lendo.exchange(true))return;
        int v=2;
        try{const auto ini=nrproxy::ini(base);
            if(!ini.empty())v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralFsrIntermitente",2,ini.c_str()));}catch(...){}
        wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);
        modo=nrfsrint::efetivo(v,nrjogo::conhecido(exe));nrfsrint::modo.store(modo,std::memory_order_relaxed);
        nrlogfila::linha(L"nr_reproject.log","pid=%lu FSR_INTERMITENTE vigia=%d NeuralFsrIntermitente=%d conhecido=%d\n",GetCurrentProcessId(),modo,v,nrjogo::conhecido(exe)?1:0);
    }
    if(modo==0)return;
    {
        static std::mutex trava_rep;static nrfsrint::Repetidos rep;std::lock_guard<std::mutex> lr(trava_rep);
        nrfsrint::repetidos.store(rep.passo(nrfsrint::upscales.load(std::memory_order_relaxed)),std::memory_order_relaxed);
    }
    static std::mutex trava;std::unique_lock<std::mutex> lk(trava,std::try_to_lock);if(!lk.owns_lock())return;
    static nrfsrint::Detector d;
    const std::uint64_t agora=GetTickCount64(),p=nrfsrint::presents.load(std::memory_order_relaxed),g=nrfsrint::gerados.load(std::memory_order_relaxed),
        u=nrfsrint::upscales.load(std::memory_order_relaxed),rp=nrfsrint::repetidos.load(std::memory_order_relaxed);
    const int r=d.passo(agora,p,g>rp?g:rp,u);
    if(!r)return;
    nrfsrint::taxa_mil.store(d.taxa_mil(),std::memory_order_relaxed);
    char t[320];
    std::snprintf(t,sizeof t,"%s: taxa %.3f despacho/quadro real na janela de %u ms (%llu despachos, %llu quadros reais), %u reentradas em %u ms; Presents %llu gerados %llu upscales %llu repetidos %llu",
        r>0?"so FSR (resolucao dinamica?)":"FSR seguido de novo, rede volta",double(d.taxa_mil())/1000.0,d.limiares().janela_ms,
        static_cast<unsigned long long>(d.despachos_janela()),static_cast<unsigned long long>(d.quadros_janela()),d.reentradas(agora),d.limiares().reentradas_ms,
        static_cast<unsigned long long>(p),static_cast<unsigned long long>(g),static_cast<unsigned long long>(u),static_cast<unsigned long long>(rp));
    nrfsrint::ativo.store(r>0,std::memory_order_relaxed);
    if(r>0)nrcompat::tolerado("tolerado_fsr_intermitente",t);else nrcompat::report("fsr_intermitente_fim",t);
    static std::atomic<unsigned> linhas{0};
    if(linhas.fetch_add(1,std::memory_order_relaxed)<200)nrlogfila::linha(L"nr_reproject.log","pid=%lu FSR_INTERMITENTE %s %s\n",GetCurrentProcessId(),r>0?"ENTRA":"SAI",t);
}

void conferir_sem_ffx() noexcept {
    static std::atomic<unsigned> presents{0};static std::atomic<bool> feito{false};
    if(feito.load(std::memory_order_relaxed)||presents.fetch_add(1,std::memory_order_relaxed)+1<3000u)return;
    if(feito.exchange(true))return;
    if(nrscale::despachos_vistos.load(std::memory_order_relaxed))return;
    char t[512];int n=std::snprintf(t,sizeof t,"3000 quadros sem nenhum despacho do FSR enganchado; carregados:");
    for(const wchar_t* m:{L"amd_fidelityfx_dx12.dll",L"amd_fidelityfx_loader_dx12.dll",L"amd_fidelityfx_upscaler_dx12.dll",L"ffx_fsr3_x64.dll",
                          L"ffx_fsr3upscaler_x64.dll",L"ffx_fsr2_api_x64.dll",L"ffx_fsr2_api_dx12_x64.dll",L"nvngx_dlss.dll",L"sl.interposer.dll",
                          L"libxess.dll",L"OptiScaler.dll",L"dxgi.dll"})
        if(GetModuleHandleW(m)&&n>0&&n<int(sizeof t)-40)n+=std::snprintf(t+n,sizeof t-std::size_t(n)," %ls",m);
    if(n>0&&n<int(sizeof t)-80)std::snprintf(t+n,sizeof t-std::size_t(n),"%s"," (sem amd_fidelityfx_*/ffx_fsr3*: FSR no exe ou desligado no menu do jogo)");
    nrcompat::report("sem_despacho_ffx",t);
}
}
#include <atomic>
#include "present_scope.h"
#include "menu_input.h"
#include "menu_layout.h"
#include "../../../layout/menu_compacto.h"
#include "../live-transition-binding/live_binding.h"
#include "../../../runtime/rosto_jogador_state.h"
#include "present_trace.h"
#include "../present-runtime/print_capture.h"
#include "../present-runtime/rec_aviso.h"
#include "../../../runtime/overlay_amd.h"
#include "../../../runtime/painel_rede.h"
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
    __try{nrprint::capture(swapchain,queue);nrprint::medir_tela(swapchain,queue);}
    __except(observe(GetExceptionCode(),GetExceptionInformation()->ExceptionRecord->ExceptionAddress,"print_capture")){}
}

__declspec(noinline) void native_overlay(void* base,void* swapchain,void* actual,const char* title){
    using Native=void(__cdecl*)(void*,void*,const char*);
    __try{reinterpret_cast<Native>(static_cast<std::uint8_t*>(base)+0x1f880)(swapchain,actual,title);}
    __except(observe(GetExceptionCode(),GetExceptionInformation()->ExceptionRecord->ExceptionAddress,"overlay_0x1f880")){}
}
}

namespace {
std::atomic<unsigned long long> present_excecoes{0};
void present_excecao(const char* onde) noexcept {
    if(present_excecoes.fetch_add(1,std::memory_order_relaxed)<16)nrpresenttrace::line("exception where=%s code=cpp (contida)",onde);
}

bool admitido(void* base) noexcept {
    if(nrproxy::admitido(base))return true;
    static std::atomic<bool> avisou{false};
    if(!avisou.exchange(true)){char t[96];std::snprintf(t,sizeof t,"base=%p: digest do proxy fora do pin, so o Present/overlay nativos",base);
        nrcompat::report("proxy_nao_admitido",t);nrpresenttrace::line("proxy_nao_admitido base=%p",base);}
    return false;
}
}

extern "C" __declspec(dllexport) void NRPresent_v1(void* base,void* swapchain) noexcept {
    if(!admitido(base)){try{native_present(base,swapchain);}catch(...){}return;}
    try{
    conferir_sem_ffx();
    conferir_fsr_intermitente(base);
    nrfsr2::tentar(base);
    nrreproject::prof_invertida_jogo.store(*reinterpret_cast<const volatile int*>(static_cast<const std::uint8_t*>(base)+nrproxy::rva_flags_ffx)!=0?1:0,std::memory_order_relaxed);
    {
        static const int forca=[base]{int v=-1;
            try{const auto ini=nrproxy::ini(base);
                if(!ini.empty())v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralTomAutoForca",-1,ini.c_str()));}catch(...){}
            if(v==0||v==1){nrreproject::tom_pedido.store(v,std::memory_order_relaxed);char t[96];std::snprintf(t,sizeof t,"NeuralTomAutoForca=%d: tonemap da entrada da rede = %d",v,v);
                nrcompat::tolerado("tolerado_tom_forca",t);nrlogfila::linha(L"nr_reproject.log","pid=%lu TOM_AUTO %s\n",GetCurrentProcessId(),t);return v;}
            return -1;}();
        (void)forca;
        auto* b=static_cast<std::uint8_t*>(base);auto* ef=reinterpret_cast<volatile int*>(b+nrreproject::rva_tom_ef);
        nrreproject::tom_ini.store(*reinterpret_cast<const volatile int*>(b+nrreproject::rva_tom_ini),std::memory_order_relaxed);
        const int p=nrreproject::tom_pedido.load(std::memory_order_relaxed);
        if(p>=0&&*ef!=p){static std::atomic<int> trocas{0};if(trocas.fetch_add(1,std::memory_order_relaxed)<8)nrlogfila::linha(L"nr_reproject.log","pid=%lu TOM_AUTO efetivo %d -> %d (ini %d)\n",GetCurrentProcessId(),int(*ef),p,nrreproject::tom_ini.load(std::memory_order_relaxed));*ef=p;}
        {static std::atomic<int> vistos{-99};const int par=nrreproject::tom_ini.load(std::memory_order_relaxed)*16+int(*ef);static std::atomic<int> n{0};
         if(p<0&&vistos.exchange(par,std::memory_order_relaxed)!=par&&n.fetch_add(1,std::memory_order_relaxed)<8){nrlogfila::linha(L"nr_reproject.log","pid=%lu TOM_AUTO proxy ini=%d efetivo=%d\n",GetCurrentProcessId(),nrreproject::tom_ini.load(std::memory_order_relaxed),int(*ef));}}
        nrreproject::tom_ef.store(*ef,std::memory_order_relaxed);}
    if(nrpainel::sonda.load(std::memory_order_relaxed)){LARGE_INTEGER q{};QueryPerformanceCounter(&q);nrpainel::no_present(q.QuadPart);}
    }catch(...){present_excecao("present_antes");}
    nrpresent::Scope scope(base,swapchain);
    std::uint64_t n=0;bool first=false;

    static thread_local int profundidade=0;
    if(profundidade>0)nrpresenttrace::state().aninhados.fetch_add(1,std::memory_order_relaxed);
    struct Profundidade{int& p;explicit Profundidade(int& x):p(x){++p;}~Profundidade(){--p;}} prof(profundidade);
    try{
    auto& trace=nrpresenttrace::state();
    n=trace.presents.fetch_add(1,std::memory_order_relaxed)+1;
    first=nrpresenttrace::first(trace.present_seen,swapchain);
    if(first)nrpresenttrace::line("present_enter_first swapchain=%p n=%llu",swapchain,static_cast<unsigned long long>(n));
    nrfgtempo::evento('P',0);
    {static std::atomic<bool> feito{false};if(!feito.exchange(true)){const auto ini_fg=nrproxy::ini(base);nrfgpacing::instalar(ini_fg);nrfgconfig::instalar(ini_fg);}}
    }catch(...){present_excecao("present_preparo");}
    try{native_present(base,swapchain);}catch(...){present_excecao("present_0x9990");}
    try{
    nrrj::presents_fim.fetch_add(1,std::memory_order_release);
    (void)first;(void)n;
    }catch(...){present_excecao("present_depois");}
}
extern "C" __declspec(dllexport) void NRPresentQueue_v1(void* base,void* swapchain,void* queue) noexcept {
    try{
    nrpresent::Scope::record(base,swapchain,queue);
    {auto& trace=nrpresenttrace::state();trace.queues.fetch_add(1,std::memory_order_relaxed);
     if(nrpresenttrace::first(trace.queue_seen,swapchain))nrpresenttrace::line("present_queue_first swapchain=%p queue=%p",swapchain,queue);}

    nrlive::note_present_queue(reinterpret_cast<std::uint64_t>(queue));
    }catch(...){present_excecao("present_fila");}
}
extern "C" __declspec(dllexport) void NROverlay_v1(void* base,void* swapchain,void* supplied,const char* title) noexcept {
    if(!admitido(base)){try{native_overlay(base,swapchain,supplied,title);}catch(...){}return;}
    void* actual=supplied;
    try{
    auto* selected=nrpresent::Scope::selected(base,swapchain);
    {auto& trace=nrpresenttrace::state();trace.overlays.fetch_add(1,std::memory_order_relaxed);
     if(nrpresenttrace::first(trace.overlay_seen,swapchain))nrpresenttrace::line("overlay_first swapchain=%p supplied=%p selected=%p",swapchain,supplied,selected);}

    actual=selected?selected:supplied;
    static std::mutex mutex;
    {std::lock_guard<std::mutex> lock(mutex);
     static unsigned count{};static void* last_supplied{};static void* last_actual{};
     if(count<100&&(count==0||last_supplied!=supplied||last_actual!=actual)){
        ++count;last_supplied=supplied;last_actual=actual;
     }}
    }catch(...){present_excecao("overlay_antes");}
    try{
    print_capture(swapchain,actual);
    nraviso::desenhar(swapchain,actual);
    }catch(...){present_excecao("overlay_print");}
    nrmenulayout::Scope layout(base,swapchain);
    try{
        const int m=nrmenucompacto::modo.load(std::memory_order_relaxed);
        const bool fechado=nrproxy::menu_visivel(base)==0;
        if(!fechado&&!nrmenucompacto::desenhando.load(std::memory_order_relaxed))nrmenucompacto::ja_aberto.store(true,std::memory_order_relaxed);

        nrmenucompacto::pedido=fechado&&(nrmenucompacto::ja_aberto.load(std::memory_order_relaxed)||m==2)&&
            (m==2||(m==1&&nrmenucompacto::recolhido.load(std::memory_order_relaxed)));
    }catch(...){present_excecao("overlay_compacto");}
    try{native_overlay(base,swapchain,actual,title);}catch(...){present_excecao("overlay_0x1f880");}
    try{
    if(nrmenucompacto::desenhando.load(std::memory_order_relaxed)){
        auto* b=static_cast<std::uint8_t*>(base);
        nrproxy::menu_visivel(b)=0;
        nrmenucompacto::desenhando.store(false,std::memory_order_relaxed);
        if(!nrmenucompacto::ja_aberto.load(std::memory_order_relaxed)){
            nrproxy::avisar_menu_fechado(b);
            nrmenucompacto::ja_aberto.store(true,std::memory_order_relaxed);
        }
    }
    nrmenucompacto::pedido=false;
    {static const std::wstring ini=nrproxy::ini(base);nrtela::capturar(swapchain,actual,ini);}
    nrmenuinput::tick(base);
    nroverlayamd::tick(base,reinterpret_cast<const void*>(&NRPresent_v1));
    }catch(...){nrmenucompacto::pedido=false;present_excecao("overlay_depois");}
}
