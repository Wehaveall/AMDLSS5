#include "../../../runtime/nr_publico.h"
#include "live_binding.h"
#include "mode_state.h"
#include "native_ini_hook.h"
#include "../async-reprojection/backend.h"
#include "../async-reprojection/reproject_controls.h"
#include "../async-reprojection/gap_invalidacao.h"
#include "../async-reprojection/recusa_log.h"
#include "late_state.h"
#include "wait_trace.h"
#include "hip_memory_observer.h"
#include "captura_carimbo_win.h"
#include "espera_diag.h"
#include "custo_perfil_win.h"
#include "../../../runtime/native_ini_path.h"
#include "../../../runtime/pasta_helper.h"
#include "../../../runtime/proxy_layout.h"
#include "../../../runtime/ini_jogo.h"
#include "../../../runtime/log_fila.h"
#include "../../../runtime/reuso_state.h"
#include "../../../runtime/exposicao_fresca.h"
#include "../../../runtime/rede_cada.h"
#include "../../../runtime/pre_scale_state.h"
#include "../../../runtime/diag_mov.h"
#include "../../../runtime/filtros_gpu.h"
#include "../../../runtime/d3d12_comum.h"
#include "../live-recording-observer/compute_state.h"
#include "../live-recording-observer/preserve_capture.h"
#include "../live-recording-observer/recording_table.h"
#include "../live-transition-image/live_image.h"
#include "../active-scheduling/ffx_descriptor.h"
#include "../live-transition-fence-proof/d3d12_backend.h"
#include "../live-transition-thread-proof/thread_proof.h"
#include "../live-transition-hip-proof/hip_proof.h"
#include "../compat-diag/compat.h"
#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <wrl/client.h>
#include <intrin.h>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <cstdio>
#include <vector>
#include <filesystem>
#include <string>
#include <atomic>
#include <cwchar>
namespace nrlive {
namespace {
using Microsoft::WRL::ComPtr;
using nrproxy::field;using nrproxy::copy;

bool copy_imagem(const void* p,void* out,std::size_t n)noexcept{
    __try{std::memcpy(out,p,n);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return copy(p,out,n);}
}
bool same(IUnknown* a,IUnknown* b)noexcept{if(!a||!b)return false;ComPtr<IUnknown>x,y;return SUCCEEDED(a->QueryInterface(IID_PPV_ARGS(&x)))&&SUCCEEDED(b->QueryInterface(IID_PPV_ARGS(&y)))&&x.Get()==y.Get();}

void observer_log(const char* stage,const char* reason,void* command,std::uint64_t canonical,std::uint32_t owner)noexcept{
    try{
        void** table{};void* slots[2]{};
        const bool table_read=copy(command,&table,sizeof(table));
        const bool slots_read=table_read&&table&&copy(table+9,slots,sizeof(slots));
        char table_module[320],table_region[160],close_module[320],close_region[160],reset_module[320],reset_region[160],first_module[320],base_module[320];
        nrcompat::module_of(table,table_module,sizeof(table_module));nrcompat::region_of(table?table+9:nullptr,table_region,sizeof(table_region));
        nrcompat::module_of(slots[0],close_module,sizeof(close_module));nrcompat::region_of(slots[0],close_region,sizeof(close_region));
        nrcompat::module_of(slots[1],reset_module,sizeof(reset_module));nrcompat::region_of(slots[1],reset_region,sizeof(reset_region));
        void* const first=nrstate::observed_table();nrcompat::module_of(first,first_module,sizeof(first_module));
        auto* object=static_cast<IUnknown*>(command);
        HRESULT qi_list=E_POINTER,qi_list4=E_POINTER,qi_sl=E_POINTER,device_hr=E_POINTER;int type=-1;
        ComPtr<ID3D12GraphicsCommandList> list;ComPtr<ID3D12GraphicsCommandList4> list4;ComPtr<ID3D12Device> device;ComPtr<IUnknown> device_unknown,canonical_unknown,base;
        void** base_table{};
        if(object){
            qi_list=object->QueryInterface(IID_PPV_ARGS(&list));
            if(SUCCEEDED(qi_list)&&list){type=static_cast<int>(list->GetType());device_hr=list->GetDevice(IID_PPV_ARGS(&device));if(SUCCEEDED(device_hr)&&device)(void)device.As(&device_unknown);}
            qi_list4=object->QueryInterface(IID_PPV_ARGS(&list4));

            static const GUID streamline_base{0xADEC44E2,0x61F0,0x45C3,{0xAD,0x9F,0x1B,0x37,0x37,0x92,0x84,0xFF}};
            qi_sl=object->QueryInterface(streamline_base,reinterpret_cast<void**>(base.GetAddressOf()));
            if(SUCCEEDED(qi_sl)&&base)(void)copy(base.Get(),&base_table,sizeof(base_table));
        }
        if(canonical)(void)reinterpret_cast<IUnknown*>(canonical)->QueryInterface(IID_PPV_ARGS(&canonical_unknown));
        nrcompat::module_of(base_table,base_module,sizeof(base_module));
        char detail[3000];
        NRLOGF(detail,sizeof(detail),
            "owner=%lu stage=%s reason=%s command=%p vtable=%p vtable_module=%s vtable_slots_region=%s "
            "close=%p close_module=%s close_region=%s reset=%p reset_module=%s reset_region=%s first_vtable=%p first_module=%s same_as_first=%u private_tables=%zu "
            "qi_list=%08lX qi_list_same=%u type=%d qi_list4=%08lX qi_list4_same=%u get_device=%08lX list_device=%p list_device_unknown=%p "
            "canonical_device=%llx canonical_unknown=%p same_device=%u streamline_base=%08lX streamline_base_object=%p streamline_base_vtable=%p streamline_base_module=%s",
            static_cast<unsigned long>(owner),stage?stage:"ok_observer_installed",reason?reason:"none",
            command,static_cast<void*>(table),table_module,table_region,
            slots[0],close_module,close_region,slots[1],reset_module,reset_region,first,first_module,(first&&first==static_cast<void*>(table))?1u:0u,nrstate::private_table_count(),
            static_cast<unsigned long>(qi_list),list.Get()==command?1u:0u,type,static_cast<unsigned long>(qi_list4),static_cast<void*>(list4.Get())==command?1u:0u,
            static_cast<unsigned long>(device_hr),static_cast<void*>(device.Get()),static_cast<void*>(device_unknown.Get()),
            static_cast<unsigned long long>(canonical),static_cast<void*>(canonical_unknown.Get()),(device_unknown&&device_unknown.Get()==canonical_unknown.Get())?1u:0u,
            static_cast<unsigned long>(qi_sl),static_cast<void*>(base.Get()),static_cast<void*>(base_table),base_module);
        (void)slots_read;

        nrcompat::report(!stage?"ok_observer_installed":reason&&*reason?reason:stage,detail);
    }catch(...){}
}
class WindowsHost final:public Host {
    std::uint8_t* base_;

    bool ini_jogo_=preparar_ini_jogo(base_);
    static bool preparar_ini_jogo(const void* b)noexcept{nrinijogo::definir(nrinipath::resolve(b,reinterpret_cast<const void*>(&make_windows_host)));return true;}
    bool verified_{};
    bool debug_probe_=false;
    ULONGLONG trace_flush_tick_{};
    void flush_trace()noexcept{
}
    UINT64 debug_cursor_{};unsigned debug_written_{};bool debug_announced_{};
    void debug_messages(const nractive::Globals& g)noexcept{
(void)g;
}
    char verification_message_[256]{"verification_not_started"};
    std::unique_ptr<nrtransition::ThreadProof> threads_;
    std::unique_ptr<nrtransition::HipProof> hip_;
    nrrecord::RecordingTable recordings_;
    nrtransition::ThreadPairSnapshot stopped_{};
    int hip_device_=-1;
    nrreproject::Backend reprojection_;
    std::uint64_t reprojection_tick_{};
    std::uint32_t reprojection_structure_{},reprojection_skin_{},reprojection_style_{};
    std::uint64_t hip_generation_{};
    using Mutex=int(__cdecl*)(void*);
    bool lock()noexcept{return reinterpret_cast<Mutex>(base_+0x35EC4)(base_+0x94B00)==0;}
    bool unlock()noexcept{return reinterpret_cast<Mutex>(base_+0x35ECC)(base_+0x94B00)==0;}
    nrtransition::ThreadPairSnapshot thread_snapshot(const nractive::Globals& g,std::uint64_t gen)const noexcept{
        return {reinterpret_cast<HANDLE>(field<std::uint64_t>(g,0x949A8)),field<DWORD>(g,0x949B0),reinterpret_cast<HANDLE>(field<std::uint64_t>(g,0x948E0)),field<DWORD>(g,0x948E8),gen,field<std::uint8_t>(g,nrproxy::rva_inline_ativo)!=0};
    }
public:
    explicit WindowsHost(void* b):base_(static_cast<std::uint8_t*>(b)){}
    void* base()const noexcept override{return base_;}
    bool verify()noexcept override{
        nrliveimage::VerificationDiagnostic detail{};
        if(!nrliveimage::verify_live_image(base_,&detail)){
            NRLOGF(verification_message_,sizeof(verification_message_),"image_verify_%s_rva_%08X_win32_%u_protect_%08X_byte_%08X_expected_%02X_observed_%02X_n_%u_ew_%016llX_ow_%016llX",nrliveimage::verification_stage_name(detail.stage),static_cast<unsigned>(detail.span_rva),static_cast<unsigned>(detail.win32_error),static_cast<unsigned>(detail.actual_protection),static_cast<unsigned>(detail.mismatch_rva),static_cast<unsigned>(detail.expected_byte),static_cast<unsigned>(detail.observed_byte),static_cast<unsigned>(detail.window_size),static_cast<unsigned long long>(detail.expected_window),static_cast<unsigned long long>(detail.observed_window));return false;
        }
        HMODULE original{},provider{};
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(base_),&original)){
            NRLOGF(verification_message_,sizeof(verification_message_),"original_module_pin_failed_win32_%u",static_cast<unsigned>(GetLastError()));return false;
        }
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&make_windows_host),&provider)){
            NRLOGF(verification_message_,sizeof(verification_message_),"provider_module_pin_failed_win32_%u",static_cast<unsigned>(GetLastError()));return false;
        }
        if(!nrini::install(base_)){NRLOGF(verification_message_,sizeof(verification_message_),"native_ini_serialization_install_failed");return false;}

        const auto ini=nrinipath::resolve(base_,reinterpret_cast<const void*>(&make_windows_host));
        nrinijogo::definir(ini);

        if(NRCFG_INT(L"DlssNrOnAmd",L"NeuralSemDiagMov",0,ini.c_str())==1){
            const bool ok=nrdiagmov::desligar(base_);
            nrlogfila::linha(L"nr_reproject.log","pid=%lu DIAG_MOV NeuralSemDiagMov=1 -> %s\n",GetCurrentProcessId(),ok?"desligado (cmp job%100==50 -> 127)":"FALHOU (bytes do proxy diferentes): diagnostico segue");
        }
        const bool game_exposure=NRCFG_INT(L"DlssNrOnAmd",L"UseGameExposure",1,ini.c_str())!=0;
        nrffx::set_use_game_exposure(game_exposure);

        {const bool fresca=NRCFG_INT(L"DlssNrOnAmd",L"NeuralExposicaoFresca",0,ini.c_str())!=0;
         const bool historico=NRCFG_INT(L"DlssNrOnAmd",L"NeuralHistoricoExposicao",0,ini.c_str())!=0;

         const int semente_modo=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralSementeFixa",0,ini.c_str()));
         const bool semente=semente_modo!=0;
         const bool temporal=NRCFG_INT(L"DlssNrOnAmd",L"Temporal",1,ini.c_str())!=0;
         nrexpofresca::fresca.store(fresca&&game_exposure);nrexpofresca::historico.store(historico&&game_exposure);
         nrsementefixa::ativa.store(semente&&(!temporal||semente_modo==2));
         if(fresca||historico||semente){char l[256];NRLOGF(l,sizeof l,"pid=%lu CHAVES NeuralExposicaoFresca=%d NeuralHistoricoExposicao=%d NeuralSementeFixa=%d Temporal=%d UseGameExposure=%d -> fresca=%d historico=%d semente_fixa=%d",
             GetCurrentProcessId(),fresca?1:0,historico?1:0,semente?1:0,temporal?1:0,game_exposure?1:0,nrexpofresca::fresca.load()?1:0,nrexpofresca::historico.load()?1:0,nrsementefixa::ativa.load()?1:0);nrexpofresca::log(l);}}

        verified_=true;
        return verified_;
    }
    const char* verification_diagnostic()const noexcept override{return verification_message_;}
    nrrecord::Observer* recording(void* command,std::uint64_t canonical_device,std::uint32_t owner)noexcept override{

        const auto r=recordings_.find_or_install(command,canonical_device,owner);
        if(r.evicted){char d[96];NRLOGF(d,sizeof d,"despejadas=%u total=%llu tabela=%zu",r.evicted,static_cast<unsigned long long>(recordings_.evicted()),recordings_.size());
            nrcompat::report("host_recordings_despejo",d);}

        if(r.rearmada){char d[96];NRLOGF(d,sizeof d,"lista=%p rearmadas=%llu",command,static_cast<unsigned long long>(recordings_.rearmadas()));
            nrcompat::report("host_recordings_rearmada",d);}
        indice_sem_memoria();
        if(!r.observer){observer_log(r.stage,r.reason,command,canonical_device,owner);return nullptr;}
        if(r.installed)observer_log(nullptr,nullptr,command,canonical_device,owner);

        if(r.installed&&recordings_.size()%64==0){char d[96];NRLOGF(d,sizeof d,"listas observadas=%zu teto=%zu despejadas=%llu",recordings_.size(),
            nrrecord::RecordingTable::cap,static_cast<unsigned long long>(recordings_.evicted()));nrcompat::report("host_recordings_tamanho",d);}

        if(!reset_ligado_.exchange(true)&&listas_no_reset()){instancia_reset().store(this,std::memory_order_release);nrrecord::set_reset_nao_rastreada(&WindowsHost::no_reset);}
        return r.observer;
    }
    static bool listas_no_reset()noexcept{static const bool v=[]{double x=0.;return nrinijogo::ler_double(L"NeuralListasNoReset",x)&&x>.5;}();return v;}

    static void indice_sem_memoria()noexcept{
        static std::atomic<bool> feito{false};
        if(feito.load(std::memory_order_relaxed)||!nrrecord::indice_falhas()||feito.exchange(true))return;
        char d[96];NRLOGF(d,sizeof d,"falhas=%zu endpoints=%zu (lista fora do indice: segue sem observacao)",nrrecord::indice_falhas(),nrrecord::endpoints());
        nrcompat::report("host_recordings_indice_sem_memoria",d);
    }
    static std::atomic<WindowsHost*>& instancia_reset()noexcept{static std::atomic<WindowsHost*> h{nullptr};return h;}
    std::atomic<bool> reset_ligado_{false};

    static void no_reset(void* command)noexcept{
        auto* h=instancia_reset().load(std::memory_order_acquire);if(!h||!command||!h->verified_)return;
        const auto tipo=static_cast<ID3D12GraphicsCommandList*>(command)->GetType();
        if(tipo!=D3D12_COMMAND_LIST_TYPE_DIRECT&&!(tipo==D3D12_COMMAND_LIST_TYPE_COMPUTE&&fila_compute()))return;
        std::uint64_t canonico{};if(!copy_imagem(h->base_+nrproxy::rva_dispositivo,&canonico,sizeof(canonico))||!canonico)return;
        const auto r=h->recordings_.find_or_install(command,canonico,GetCurrentThreadId(),nrrecord::RecordingTable::Origem::Reset);

        if(r.stage&&!std::strcmp(r.stage,"reset_teto_instalados")){
            nrrecord::set_reset_nao_rastreada(nullptr);
            static std::atomic<bool> uma{false};
            if(!uma.exchange(true)){char d[112];NRLOGF(d,sizeof d,"instalados=%llu teto=%llu: listas so pelo FSR daqui em diante",
                static_cast<unsigned long long>(h->recordings_.instalados()),static_cast<unsigned long long>(nrrecord::RecordingTable::teto_instalados_reset_padrao));
                nrcompat::report("host_recordings_teto_reset",d);}
            return;
        }
        indice_sem_memoria();
        if(r.evicted){char d[112];NRLOGF(d,sizeof d,"pelo Reset: despejadas=%u total=%llu tabela=%zu",r.evicted,static_cast<unsigned long long>(h->recordings_.evicted()),h->recordings_.size());
            nrcompat::report("host_recordings_despejo",d);}
        if(r.installed){const auto n=h->recordings_.size();if(n%256==0){char d[112];NRLOGF(d,sizeof d,"listas observadas=%zu teto=%zu (acompanhadas desde o Reset)",n,nrrecord::RecordingTable::cap);
            nrcompat::report("host_recordings_tamanho",d);}}
    }
    void recordings_in_use(void* const* commands,unsigned count)noexcept override{recordings_.in_use(commands,count);}
    bool publish(const NRLifecycleCallbacks* callbacks)noexcept override{
        if(!verified_||!callbacks)return false;
        void* slot=base_+NR_LIFECYCLE_CALLBACK_RVA;
        if(!slot_callback_publicavel(base_,slot))return false;
        return _InterlockedCompareExchangePointer(reinterpret_cast<void*volatile*>(slot),const_cast<NRLifecycleCallbacks*>(callbacks),nullptr)==nullptr;
    }
    bool read(nractive::Globals& out)noexcept override{nrmem::flush();flush_trace();const bool ok=verified_&&copy_imagem(base_+nractive::globals_begin,out.data(),out.size());if(ok)debug_messages(out);return ok;}

    std::atomic<FARPROC> hip_set_ok_{},hip_sync_ok_{};
    bool hip_imports_valid()noexcept override{
        if(!verified_||!nrmem::validate(base_))return false;
        if(const auto set_ok=hip_set_ok_.load(std::memory_order_acquire)){
            FARPROC imported_set{},imported_sync{};
            return copy(base_+0x86C18,&imported_set,sizeof(imported_set))&&copy(base_+0x86B60,&imported_sync,sizeof(imported_sync))&&
                   imported_set==set_ok&&imported_sync==hip_sync_ok_.load(std::memory_order_relaxed);
        }
        HMODULE hip=GetModuleHandleW(L"amdhip64_7.dll");if(!hip)return false;
        const auto set=GetProcAddress(hip,"hipSetDevice");const auto sync=GetProcAddress(hip,"hipDeviceSynchronize");
        FARPROC imported_set{},imported_sync{};
        if(!set||!sync||!copy(base_+0x86C18,&imported_set,sizeof(imported_set))||!copy(base_+0x86B60,&imported_sync,sizeof(imported_sync))||imported_set!=set||imported_sync!=sync)return false;
        HMODULE pinned{},set_module{},sync_module{};
        const bool ok=GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN,L"amdhip64_7.dll",&pinned)&&pinned==hip&&
          GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(set),&set_module)&&
          GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(sync),&sync_module);
        if(ok){hip_sync_ok_.store(sync,std::memory_order_relaxed);hip_set_ok_.store(set,std::memory_order_release);}
        return ok;
    }
    unsigned thread()const noexcept override{return GetCurrentThreadId();}
    bool yield_gpu_progress()noexcept override{Sleep(1);return true;}

    static std::filesystem::path helper_folder()noexcept{
        try{return nrpasta::helper();}catch(...){}
        return {};
    }

    std::wstring ini_proxy()const{
        if(nrinijogo::pronto().load(std::memory_order_acquire))return nrinijogo::guardado();
        return nrinipath::resolve(base_,reinterpret_cast<const void*>(&make_windows_host));
    }
    int chave_ini(const wchar_t* chave,int padrao)const noexcept{
        try{const auto ini=ini_proxy();if(ini.empty())return padrao;return static_cast<int>(NRCFG_INT(L"DlssNrOnAmd",chave,padrao,ini.c_str()));}catch(...){return padrao;}
    }
    nrjob::Tuning async_tuning()noexcept override{
        nrjob::Tuning t{};
        t.short_wait_ms=(std::min)(8u,static_cast<unsigned>(chave_ini(L"NeuralAsyncShortWaitMs",0)));
        t.exclusive_ms=(std::min)(20u,static_cast<unsigned>(chave_ini(L"NeuralAsyncExclusiveMs",0)));

        t.age_log_frames=nrjob::age_log_frames(chave_ini(L"NeuralAgeLogFrames",static_cast<int>(nrjob::age_log_frames_default)));
        return t;
    }
    long long async_remaining_us(int job)noexcept override{
        auto& tl=nrjob::timeline();const auto inicio=tl.when(job,nrjob::EvalInicio);
        if(!inicio||tl.when(job,nrjob::EvalFim)||tl.frequency()<=0)return -1;
        const auto ema=tl.eval_ema_us();if(!ema)return 1000000000;
        const long long gasto=(nrjob::qpc()-inicio)*1000000/tl.frequency();
        return ema>gasto?ema-gasto:0;
    }

    int late_on_=-1,late_job_{};

    int gap_ms_=-1;
    std::uint64_t gap_ms()noexcept{
        if(gap_ms_<0){
            const int v=chave_ini(L"NeuralReprojectGapMs",100);gap_ms_=v<100?100:v>10000?10000:v;
            if(gap_ms_!=100){{nrlogfila::linha(L"nr_reproject.log","pid=%lu NeuralReprojectGapMs=%d\n",GetCurrentProcessId(),gap_ms_);}}
        }
        return static_cast<std::uint64_t>(gap_ms_);
    }

    int gap_suave_=-1;unsigned gap_suaves_{};
    bool gap_suave()noexcept{
        if(gap_suave_<0){
            gap_suave_=nrgap::gap_suave_de(chave_ini(L"NeuralReprojectGapSuave",0));
            if(gap_suave_){{nrlogfila::linha(L"nr_reproject.log","pid=%lu NeuralReprojectGapSuave=%d\n",GetCurrentProcessId(),gap_suave_);}}

            nrrecusa::segura.store(chave_ini(L"NeuralReprojectSeguraRecusa",0)==1);
            if(nrrecusa::segura.load()){{nrlogfila::linha(L"nr_reproject.log","pid=%lu NeuralReprojectSeguraRecusa=1\n",GetCurrentProcessId());}}
        }
        return gap_suave_==1;
    }
    bool late_enabled()noexcept{
        if(late_on_<0){
            late_on_=chave_ini(L"NeuralLateCompose",0)==1?1:0;
            {nrlogfila::linha(L"nr_reproject.log","pid=%lu NeuralLateCompose=%d\n",GetCurrentProcessId(),late_on_);}
        }
        if(nrlate::desligada.load(std::memory_order_relaxed))return false;
        if(nrreproject::controls.seeded.load(std::memory_order_acquire))return nrreproject::value(nrreproject::controls,nrreproject::LateCompose)>.5f;
        return late_on_==1;
    }
    void late_candidate(int job)noexcept override{late_job_=job;}
    nrjob::LateTotals late_totals()noexcept override{
        const auto c=reprojection_.late_counters();
        return {c.armed,c.recorded,c.resolved,c.executed_late,c.refused,c.b_failed,c.inconsistent,c.gain_frames,c.gain_ms,c.desligada};
    }
    HANDLE wait_timer_{};
    int wait_async_completion(int job,unsigned us)noexcept override{
        auto* evento=static_cast<HANDLE>(nrjob::completion_event());if(!evento)return -1;
        auto& tl=nrjob::timeline();

        ResetEvent(evento);
        if(tl.when(job,nrjob::Publicado))return 1;
        if(!us)return 0;
        if(!wait_timer_)wait_timer_=CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_ALL_ACCESS);
        if(!wait_timer_)return WaitForSingleObject(evento,(us+999)/1000)==WAIT_OBJECT_0||tl.when(job,nrjob::Publicado)?1:0;
        LARGE_INTEGER prazo{};prazo.QuadPart=-static_cast<LONGLONG>(us)*10;
        if(!SetWaitableTimer(wait_timer_,&prazo,0,nullptr,nullptr,FALSE))return WaitForSingleObject(evento,(us+999)/1000)==WAIT_OBJECT_0?1:0;
        HANDLE objetos[2]{evento,wait_timer_};
        const DWORD r=WaitForMultipleObjects(2,objetos,FALSE,us/1000+50);
        CancelWaitableTimer(wait_timer_);
        return r==WAIT_OBJECT_0||tl.when(job,nrjob::Publicado)?1:0;
    }

    int continuous_on_=-1;HANDLE batch_event_{},batch_timer_{};
    struct ContinuousStats{unsigned waits{},event_wakes{},timeouts{},fallbacks{},job_sleeps{};double ms{};}continuous_{};
    bool continuous_enabled()noexcept{
        if(continuous_on_<0){
            const int v=chave_ini(L"NeuralAsyncContinuous",0);continuous_on_=(v==1||v==2||v==3)?v:0;
            {nrlogfila::linha(L"nr_reproject.log","pid=%lu NeuralAsyncContinuous=%d\n",GetCurrentProcessId(),continuous_on_);}
        }
        return continuous_on_>0;
    }

    int espera_diag_on_=-1;nresperadiag::Diag espera_diag_;
    bool espera_diag()noexcept{
        if(espera_diag_on_<0){
            espera_diag_on_=chave_ini(L"NeuralEsperaDiag",0)==1?1:0;
            if(espera_diag_on_){espera_diag_.iniciar();nrlogfila::linha(L"nr_reproject.log","pid=%lu NeuralEsperaDiag=1\n",GetCurrentProcessId());}
        }
        return espera_diag_on_==1;
    }
    void frame_wait_begin(nrfence::OwnedFence* fence)noexcept override{if(espera_diag())espera_diag_.comeca(fence);}
    void frame_wait_end(unsigned voltas,unsigned worker)noexcept override{if(espera_diag())espera_diag_.termina(voltas,GetCurrentThreadId(),worker);}
    bool wait_batch(nrfence::OwnedFence* fence,bool job_in_flight)noexcept override{
        if(!continuous_enabled())return yield_gpu_progress();
        if(continuous_on_==2&&job_in_flight){++continuous_.job_sleeps;return yield_gpu_progress();}
        if(continuous_on_==3){
            if(!batch_timer_)batch_timer_=CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_ALL_ACCESS);
            LARGE_INTEGER prazo{};prazo.QuadPart=-2500;
            if(!batch_timer_||!SetWaitableTimer(batch_timer_,&prazo,0,nullptr,nullptr,FALSE)){++continuous_.fallbacks;Sleep(1);return true;}
            const auto t0=nrjob::qpc();
            const DWORD r=WaitForSingleObject(batch_timer_,4);
            ++continuous_.waits;if(r==WAIT_OBJECT_0)++continuous_.event_wakes;else ++continuous_.timeouts;
            continuous_.ms+=nrjob::timeline().ms(nrjob::qpc()-t0);
            return true;
        }
        if(!batch_event_)batch_event_=CreateEventW(nullptr,FALSE,FALSE,nullptr);

        if(batch_event_)ResetEvent(batch_event_);
        if(!fence||!batch_event_||!fence->arm(batch_event_)){++continuous_.fallbacks;Sleep(1);return true;}
        const auto t0=nrjob::qpc();
        const DWORD r=WaitForSingleObject(batch_event_,4);
        ++continuous_.waits;if(r==WAIT_OBJECT_0)++continuous_.event_wakes;else ++continuous_.timeouts;
        continuous_.ms+=nrjob::timeline().ms(nrjob::qpc()-t0);
        return true;
    }
    void age_log(const std::string& text)noexcept override{
(void)text;
}
    std::uint64_t now()const noexcept override{return GetTickCount64();}
    bool describe(void* object,NrResourceDesc& out)noexcept override{
        if(!object)return false;ComPtr<ID3D12Resource> resource;
        if(FAILED(static_cast<IUnknown*>(object)->QueryInterface(IID_PPV_ARGS(&resource))))return false;
        const auto d=resource->GetDesc();
        if(d.Format==DXGI_FORMAT_R9G9B9E5_SHAREDEXP){
            ComPtr<ID3D12Device> device;D3D12_FEATURE_DATA_FORMAT_SUPPORT support{d.Format};
            if(FAILED(resource->GetDevice(IID_PPV_ARGS(&device))) ||
               FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&support,sizeof(support))) ||
               !(support.Support1&D3D12_FORMAT_SUPPORT1_SHADER_LOAD) ||
               (support.Support2&(D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD|D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE)) !=
                (D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD|D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE))return false;
        }
        if(d.SampleDesc.Quality||d.Layout!=D3D12_TEXTURE_LAYOUT_UNKNOWN||(d.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))return false;
        out={};out.dimension=d.Dimension;out.alignment=d.Alignment;out.width=d.Width;out.height=d.Height;out.depth_or_array_size=d.DepthOrArraySize;out.mip_levels=d.MipLevels;out.format=d.Format;out.sample_count=d.SampleDesc.Count;out.sample_quality=d.SampleDesc.Quality;out.layout=d.Layout;out.flags=d.Flags;return true;
    }

    static bool fila_compute()noexcept{static const bool v=[]{double x=0.;return nrinijogo::ler_double(L"NeuralFilaCompute",x)&&x>.5;}();return v;}
    bool device_of(void* object,std::uint64_t& out)noexcept override{
        if(!object)return false;ComPtr<ID3D12DeviceChild> child;
        if(FAILED(static_cast<IUnknown*>(object)->QueryInterface(IID_PPV_ARGS(&child))))return false;
        ComPtr<ID3D12Resource> resource;if(FAILED(child.As(&resource))){ComPtr<ID3D12GraphicsCommandList> list;if(FAILED(child.As(&list))||(list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT&&
            !(list->GetType()==D3D12_COMMAND_LIST_TYPE_COMPUTE&&fila_compute())))return false;}

        std::uint64_t canonico{};ComPtr<ID3D12Device> actual;
        if(!verified_||!copy_imagem(base_+nrproxy::rva_dispositivo,&canonico,sizeof(canonico))||FAILED(child->GetDevice(IID_PPV_ARGS(&actual))))return false;
        out=canonico;return out&&same(actual.Get(),reinterpret_cast<IUnknown*>(out));
    }
    bool retain(void* p)noexcept override{if(!p)return false;static_cast<IUnknown*>(p)->AddRef();return true;}
    void release(void* p)noexcept override{if(p)static_cast<IUnknown*>(p)->Release();}

    bool sem_job_{};unsigned rede_cada_log_{1};
    unsigned rede_cada()noexcept override{
        if(!nrredecada::definido.load(std::memory_order_acquire)){
            unsigned n=1;
            try{const auto ini=nrinipath::resolve(base_,reinterpret_cast<const void*>(&make_windows_host));
                n=nrredecada::aceitar(static_cast<int>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRedeCadaQuadros",0,ini.c_str())));}catch(...){n=1;}
            unsigned esperado=1;bool falso=false;
            if(nrredecada::definido.compare_exchange_strong(falso,true))nrredecada::pedido.compare_exchange_strong(esperado,n);
        }
        unsigned n=nrredecada::aceitar(nrredecada::pedido.load(std::memory_order_relaxed));
        if(n!=rede_cada_log_){rede_cada_log_=n;
            nrlogfila::linha(L"nr_reproject.log","pid=%lu NeuralRedeCadaQuadros=%u (job novo so a cada %u quadros; vetos=%llu jobs=%llu)\n",GetCurrentProcessId(),n,n,
                static_cast<unsigned long long>(nrredecada::vetos.load()),static_cast<unsigned long long>(nrredecada::jobs.load()));}
        nrredecada::efetiva.store(n,std::memory_order_relaxed);
        return n;
    }
    void capture_sem_job(bool v)noexcept override{sem_job_=v;}
    void capture(const NrCaptureRecord& r)override{
        const bool sem_job=sem_job_;sem_job_=false;
        if(!hip_imports_valid()||!nrmem::install_if_requested(base_)||!nractive::publish_capture_bookkeeping(base_,nrliveimage::image_size))throw std::runtime_error("capture bookkeeping or HIP import provenance rejected");

        using Call=void(__cdecl*)(const NrCaptureRecord*);
        if((nrstate::readiness(r.command_list)&32u)==0){static bool avisado=false;ComPtr<ID3D12GraphicsCommandList> lc;
            if(!avisado&&r.command_list&&SUCCEEDED(static_cast<IUnknown*>(r.command_list)->QueryInterface(IID_PPV_ARGS(&lc)))&&lc->GetType()==D3D12_COMMAND_LIST_TYPE_COMPUTE){
                avisado=true;nrcompat::tolerado("tolerado_lista_compute_sem_pso","FSR e o primeiro comando da lista COMPUTE: captura sem PSO a repor");}}
        const auto result=nrstate::preserve_capture(r.command_list,[&]{
            nrd3d::marcador(r.command_list,L"nr_captura");
            nractive::Globals before{};if(!read(before))throw std::runtime_error("temporal_capture_read_failed");
            const int previous=field<int>(before,nrproxy::rva_job_aceito);
            const auto tick=GetTickCount64();
            const auto structure=field<unsigned>(before,nrproxy::rva_estrutura_local),skin=field<unsigned>(before,nrproxy::rva_estrutura_pele);
            unsigned style{};if(!copy(base_+0x71D084,&style,sizeof(style)))throw std::runtime_error("temporal_style_read_failed");

            const bool mudou=structure!=reprojection_structure_||skin!=reprojection_skin_||style!=reprojection_style_;
            const bool suave=gap_suave();
            switch(nrgap::decidir(tick,reprojection_tick_,gap_ms(),mudou,suave)){
            case nrgap::Invalidar::Dura:reprojection_.invalidate();break;
            case nrgap::Invalidar::Suave:reprojection_.invalidate_suave();
                if(++gap_suaves_<=20u||gap_suaves_%64u==0u){{
                    nrlogfila::linha(L"nr_reproject.log","pid=%lu GAP_SUAVE gap_ms=%llu limite=%llu n=%u (so o registro de jobs; sem CORTE)\n",GetCurrentProcessId(),
                        static_cast<unsigned long long>(tick-reprojection_tick_),static_cast<unsigned long long>(gap_ms()),gap_suaves_);}}
                break;
            case nrgap::Invalidar::Nada:break;}
            reprojection_tick_=tick;reprojection_structure_=structure;reprojection_skin_=skin;reprojection_style_=style;

            const int previsto=carimbo()?(field<unsigned char>(before,nrproxy::rva_reset_pendente)?1:previous+1):0;
            if(previsto>0)nrcapt::marcar(r.command_list,previsto,nrcapt::AntesBegin);
            custo_instalar(before);nrcusto::marca(r.command_list,nrcusto::CapInicio);if(sem_job)nrcusto::categoria(nrcusto::Vetado);
            reprojection_.rj_sem_job(sem_job);
            const bool captured=reprojection_.begin(r);
            nrcusto::marca(r.command_list,nrcusto::BeginFim);
            if(previsto>0){nrcapt::marcar(r.command_list,previsto,nrcapt::AntesNativa);carimbo_job_=previsto;}
            struct FimCarimbo{int& j;~FimCarimbo(){j=0;}}fim_carimbo{carimbo_job_};

            if(*reinterpret_cast<volatile std::uint8_t*>(base_+nrproxy::rva_aquecido)==0)
                if(const auto aquecer=nrprescale::aquecer_job1.load(std::memory_order_acquire)){

                    ComPtr<ID3D12Resource> cor;
                    if(r.color&&SUCCEEDED(static_cast<IUnknown*>(r.color)->QueryInterface(IID_PPV_ARGS(&cor)))){const auto d=cor->GetDesc();
                        if(d.Width&&d.Height&&d.Width<=16384u)(void)aquecer(base_,int(d.Width),int(d.Height));}}
            {
                nrredecada::Trava trava;
                if(sem_job&&trava.armar(reinterpret_cast<volatile long*>(base_+nrproxy::rva_job_pendente),field<int>(before,nrproxy::rva_worker_job),
                                        field<std::uint64_t>(before,nrproxy::rva_lista_pendente),field<unsigned char>(before,nrproxy::rva_reset_pendente))){
                    const auto v=nrredecada::vetos.fetch_add(1)+1;
                    if((v&1023u)==0)nrlogfila::linha(L"nr_reproject.log","pid=%lu REDE_CADA n=%u vetos=%llu jobs=%llu\n",GetCurrentProcessId(),
                        nrredecada::pedido.load(),static_cast<unsigned long long>(v),static_cast<unsigned long long>(nrredecada::jobs.load()));
                }
                reinterpret_cast<Call>(base_+0x11AC0)(&r);
            }
            nrcusto::marca(r.command_list,nrcusto::NativaFim);
            if(previsto>0)nrcapt::marcar(r.command_list,previsto,nrcapt::DepoisNativa);
            nractive::Globals after{};if(!read(after))throw std::runtime_error("temporal_publication_read_failed");
            const int accepted=field<int>(after,nrproxy::rva_job_aceito);
            const int expected=field<unsigned char>(before,nrproxy::rva_reset_pendente)?1:previous+1;
            if(captured&&accepted==expected&&field<unsigned char>(after,nrproxy::rva_inline_ativo)==0&&
               field<std::uint64_t>(after,nrproxy::rva_lista_pendente)==reinterpret_cast<std::uint64_t>(r.command_list)&&
               field<int>(after,nrproxy::rva_job_pendente)==accepted){nrcusto::categoria(nrcusto::Job);nrredecada::jobs.fetch_add(1);reprojection_.accepted(accepted);nrexpofresca::carimbar(accepted);nrreusostate::carimbar(accepted);if(late_enabled())reprojection_.late_arm(accepted);}
        });
        if(result==nrstate::PreserveResult::Skipped)reprojection_.invalidate();

        nr::wait_trace::Record(result==nrstate::PreserveResult::Skipped?nr::wait_trace::Event::CaptureStateSkipped:nr::wait_trace::Event::CaptureStateRestored,0,0,0,reinterpret_cast<std::uint64_t>(r.command_list),result==nrstate::PreserveResult::RestoreFailed?1u:0u);
        if(result==nrstate::PreserveResult::RestoreFailed)throw std::runtime_error("capture_state_restore_failed");
    }
    void invalidate_reprojection()noexcept override{reprojection_.invalidate();}
    void retire_reprojection()noexcept override{reprojection_.reset();reprojection_tick_=0;}
    unsigned reproject(void* command,void* destination,unsigned state,int job,float exposure,unsigned controls)noexcept override{
        nrreusostate::contar_quadro();

        if(carimbo_job_>0)nrcapt::marcar(command,carimbo_job_,nrcapt::FimCopia);
        nrcusto::marca(command,nrcusto::ComposeInicio);
        const int nativo=segurado_nativo_;segurado_nativo_=0;
        try{

            nractive::Globals g{};if(!read(g)){nrrecusa::registrar("host_globais_ilegiveis",job,0,0,0);return 1;}
            if(job<=0){nrrecusa::registrar("host_sem_job",job,0,0,0);return 1;}
            const int pronto=field<int>(g,nrproxy::rva_job_pronto),compondo=field<int>(g,nrproxy::rva_job_compondo);
            if(!nrrecusa::campos_ok(job,pronto,compondo,nativo,nrrecusa::segura.load(std::memory_order_relaxed))){
                nrrecusa::registrar(pronto!=job?"host_pronto_difere":"host_compondo_difere",job,pronto,compondo,nativo);return 1;}
            const bool segurado=nativo>0||pronto!=job||compondo!=job;
            if(segurado&&nativo<=0){nrrecusa::segurados.fetch_add(1);nrrecusa::registrar("host_pronto_avancou_segurado",job,pronto,compondo,0,job);}
            void* residual=reinterpret_cast<void*>(field<std::uint64_t>(g,nrproxy::rva_residuo+8*(unsigned(job)&3u)));
            const unsigned tone=(field<unsigned>(g,nrproxy::rva_tom_ef)!=0?1u:0u)|controls;
            const int late=segurado?0:late_job_;late_job_=0;
            nrreproject::Result result;

            if(late>0&&late==job+1&&late_enabled()&&field<std::uint8_t>(g,nrproxy::rva_interop_ativo)==1){
                nrreproject::LateRequest q{};
                q.residual_prev=residual;q.job_prev=job;q.exposure_prev=exposure;q.capture_qpc_prev=nrjob::timeline().when(job,nrjob::Capturado);
                q.late_source=reinterpret_cast<void*>(field<std::uint64_t>(g,0x948F8+8*(unsigned(late)&3u)));q.job_late=late;

                float e{};if(!copy(base_+nrproxy::rva_expo_job+4*(unsigned(late)&3u),&e,sizeof e))e=0.f;q.exposure_late=nrexpofresca::para_compor(late,e);
                q.exposure_prev=nrexpofresca::para_compor(job,exposure);
                q.capture_qpc_late=nrjob::timeline().when(late,nrjob::Capturado);
                nrd3d::marcador(command,L"nr_compor_tardia");result=reprojection_.compose_late(command,destination,state,q,tone);
            }else{nrd3d::marcador(command,L"nr_compor");result=reprojection_.compose(command,destination,state,residual,job,nrexpofresca::para_compor(job,exposure),tone);}
            if(result==nrreproject::Result::Composed)nrfiltros::expo.store(nrexpofresca::para_compor(job,exposure),std::memory_order_relaxed);
            return result==nrreproject::Result::Composed?0u:result==nrreproject::Result::Failed?2u:1u;
        }catch(...){return 2;}
    }
    bool dump_final(void* command,void* input,unsigned input_state,void* output,unsigned output_state,unsigned mode,std::uint64_t frame)noexcept override{
        return nrreproject::despejar_final(command,input,input_state,output,output_state,mode,frame);}
    int segurado_nativo_{};
    void reproject_segurado(int nativo)noexcept override{segurado_nativo_=nativo;}
    void dump_final_prepare(void* command,void* input,void* output,unsigned mode)noexcept override{nrreproject::preparar_final(command,input,output,mode);}
    void dump_final_tick()noexcept override{
}
    unsigned reproject_post(void* command,void* output,unsigned ffx_state)noexcept override{
        try{

            if(!reprojection_.post_pending())return 1;
            nrreproject::Result result=nrreproject::Result::Skipped;

            const auto preservado=nrstate::preserve_capture(command,[&]{nrd3d::marcador(command,L"nr_compor_post");result=reprojection_.compose_post(command,output,ffx_state);});
            if(preservado==nrstate::PreserveResult::RestoreFailed)return 2;
            if(preservado==nrstate::PreserveResult::Skipped){

                reprojection_.refuse_post("post_unsupported_list_state_fallback_pre");
                return 1;
            }
            return result==nrreproject::Result::Composed?0u:result==nrreproject::Result::Failed?2u:1u;
        }catch(...){return 2;}
    }
    unsigned filtros_post(void* command,void* output,unsigned ffx_state)noexcept override{
        try{

            if(!nrfiltros::carregado()){const auto ini=ini_proxy();nrfiltros::carregar(ini.c_str());}
            if(!nrfiltros::parametros().quais)return 1;
            unsigned r=1;

            const auto preservado=nrstate::preserve_capture(command,[&]{nrd3d::marcador(command,L"nr_filtros");r=nrfiltrosgpu::aplicar(command,output,ffx_state);});
            if(preservado==nrstate::PreserveResult::RestoreFailed)return 2;
            return preservado==nrstate::PreserveResult::Skipped?1u:r;
        }catch(...){return 2;}
    }
    void clear_async_completion()noexcept override {nrmode::async_completed_job.store(0);nrmode::async_completion_status.store(0);}

    int custo_on_=-1;
    void custo_instalar(const nractive::Globals& g)noexcept{
        if(custo_on_>=0)return;
        custo_on_=chave_ini(L"NeuralCustoPerfil",0)==1?1:0;
        if(custo_on_==1)nrcustowin::instalar(reinterpret_cast<ID3D12CommandQueue*>(field<std::uint64_t>(g,nrproxy::rva_fila)));
    }

    int carimbo_on_=-1,carimbo_job_{};
    bool carimbo()noexcept{
        if(carimbo_on_<0)carimbo_on_=nrcapt::chave(static_cast<long long>(static_cast<unsigned>(chave_ini(L"NeuralCapturaCarimbo",0))))?1:0;
        return carimbo_on_==1;
    }
    void captura_submetida(int job,void* queue)noexcept override{
        if(!carimbo())return;
        nractive::Globals g{};if(!read(g))return;
        nrcapt::submetida(job,queue,reinterpret_cast<void*>(field<std::uint64_t>(g,nrproxy::rva_cerca)),field<std::uint64_t>(g,nrproxy::rva_cerca_alvo));
    }

    bool fg_render_queue_learning()noexcept override{
        static std::atomic<int> lido{-1};
        if(const int v=lido.load(std::memory_order_acquire);v>=0)return v!=0;
        const bool valor=chave_ini(L"NeuralFgFilaRender",1)!=0;
        lido.store(valor?1:0,std::memory_order_release);return valor;
    }
    int async_completion(int job)noexcept override {
        return nrmode::async_completed_job.load(std::memory_order_acquire)==job ? nrmode::async_completion_status.load() : (std::numeric_limits<int>::min)();
    }
    bool set_inline_mode(unsigned value)noexcept override {
        std::lock_guard<std::recursive_mutex> configuration(nrini::mutex());
        if(!verified_||value>1||!lock())return false;
        nractive::Globals g{};
        const bool ok=read(g)&&!field<std::uint64_t>(g,0x949A8)&&!field<std::uint64_t>(g,0x948E0)&&
          !field<unsigned>(g,nrproxy::rva_job_aceito)&&field<int>(g,nrproxy::rva_worker_job)==-1&&!field<std::uint64_t>(g,nrproxy::rva_lista_pendente);
        bool persisted=false;
        if(ok){

            try {
                struct NativePath {char text[16];std::uint64_t size,capacity;} path{};
                std::memcpy(&path,g.data()+nrproxy::rva_caminho_ini-nractive::globals_begin,sizeof(path));
                if(path.size&&path.size<=32767&&path.capacity>=path.size&&path.capacity<=32767){
                    const char* data=path.text;if(path.capacity>=16)std::memcpy(&data,path.text,sizeof(data));
                    std::string narrow(static_cast<std::size_t>(path.size)+1,'\0');
                    if(copy(data,narrow.data(),narrow.size())&&narrow.back()=='\0'&&narrow.find('\0')==path.size){
                        const int count=MultiByteToWideChar(CP_ACP,MB_ERR_INVALID_CHARS,narrow.data(),int(path.size),nullptr,0);
                        if(count>0){
                            std::wstring wide(std::size_t(count),L'\0');
                            if(MultiByteToWideChar(CP_ACP,MB_ERR_INVALID_CHARS,narrow.data(),int(path.size),wide.data(),count)==count&&std::filesystem::path(wide).is_absolute())
                                persisted=NRCFG_ESCREVER(L"DlssNrOnAmd",L"Inline",value?L"1":L"0",wide.c_str())!=FALSE;
                        }
                    }
                }
            }catch(...){persisted=false;}
            if(persisted){
                nrmode::async_completed_job.store(0);nrmode::async_completion_status.store(0);
                _InterlockedExchange8(reinterpret_cast<volatile char*>(base_+nrproxy::rva_inline_pedido),static_cast<char>(value));
            }
        }
        return unlock()&&ok&&persisted;
    }
    bool set_pending_reset()noexcept override{if(!verified_)return false;_InterlockedExchange8(reinterpret_cast<volatile char*>(base_+nrproxy::rva_reset_pendente),1);return true;}

    bool clamp_structure(float local,float skin,unsigned auto_mask,unsigned which)noexcept override{
        if(!verified_)return false;long l{},s{};std::memcpy(&l,&local,4);std::memcpy(&s,&skin,4);
        if(which&1u)_InterlockedExchange(reinterpret_cast<volatile long*>(base_+nrproxy::rva_estrutura_local),l);
        if(which&2u)_InterlockedExchange(reinterpret_cast<volatile long*>(base_+nrproxy::rva_estrutura_pele),s);
        if(which&4u)_InterlockedExchange(reinterpret_cast<volatile long*>(base_+nrproxy::rva_mascara_auto),static_cast<long>(auto_mask));return true;
    }
    bool reset_staging()noexcept override{
        if(!verified_||!lock())return false;
        nractive::Globals before{},after{};bool ok=read(before);
        if(ok&&field<int>(before,0x94B4C)==INT_MAX)ok=false;
        if(ok){

            reinterpret_cast<void(__cdecl*)()>(base_+0x14FA0)();
            nrmem::flush(true);
            ok=read(after)&&post_reset_matches(before,after);
            if(ok){

                _InterlockedExchange8(reinterpret_cast<volatile char*>(base_+nrproxy::rva_reset_pendente),0);
                _InterlockedExchange64(reinterpret_cast<volatile long long*>(base_+0x94B80),0);
                _InterlockedExchange8(reinterpret_cast<volatile char*>(base_+0x94B88),0);
                ok=read(after)&&!field<std::uint8_t>(after,nrproxy::rva_reset_pendente)&&
                    !field<std::uint64_t>(after,0x94B80)&&!field<std::uint8_t>(after,0x94B88);
            }
        }
        const bool unlocked=unlock();if(ok&&unlocked)retire_reprojection();return unlocked&&ok;
    }
    std::unique_ptr<nrfence::OwnedFence> fence(std::uint64_t gen)override{
        nractive::Globals g{};if(!read(g))return {};
        return std::make_unique<nrfence::OwnedFence>(nrfence::make_d3d12_backend(reinterpret_cast<ID3D12Device*>(field<std::uint64_t>(g,nrproxy::rva_dispositivo)),reinterpret_cast<ID3D12CommandQueue*>(field<std::uint64_t>(g,nrproxy::rva_fila))),gen);
    }
    std::unique_ptr<nrfence::OwnedFence> fence_on_queue(std::uint64_t gen,std::uint64_t selected)override{
        nractive::Globals g{};if(!read(g)||!selected)return {};
        ComPtr<ID3D12CommandQueue> q;ComPtr<ID3D12Device> d;
        if(FAILED(reinterpret_cast<IUnknown*>(selected)->QueryInterface(IID_PPV_ARGS(&q)))||
           (q->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT&&!(q->GetDesc().Type==D3D12_COMMAND_LIST_TYPE_COMPUTE&&fila_compute()))||
           FAILED(q->GetDevice(IID_PPV_ARGS(&d)))||reinterpret_cast<std::uint64_t>(d.Get())!=field<std::uint64_t>(g,nrproxy::rva_dispositivo))return {};
        return std::make_unique<nrfence::OwnedFence>(nrfence::make_d3d12_backend(d.Get(),q.Get()),gen);
    }
    bool stop_producers(std::uint64_t gen)noexcept override{
        nractive::Globals g{};bool sampled=false;DWORD proof_error=ERROR_INVALID_DATA;
        const auto log=[&](bool ok,const char* phase)noexcept{
(void)ok;(void)phase;
};
        if(!verified_||!gen||!lock()){log(false,"admission_or_lock");return false;}
        bool ok=false;
        try{sampled=read(g);if(sampled&&field<std::uint8_t>(g,0x949B8)==0){
            const auto s=thread_snapshot(g,gen);auto proof=std::make_unique<nrtransition::ThreadProof>(s);
            proof_error=proof->last_error();
            if(proof->valid()){stopped_=s;hip_device_=field<int>(g,nrproxy::rva_hip_dispositivo);threads_=std::move(proof);hip_.reset();hip_generation_=0;
                _InterlockedExchange8(reinterpret_cast<volatile char*>(base_+0x949B8),1);ok=true;}
        }}catch(...){ok=false;}
        const bool unlocked=unlock();const bool result=unlocked&&ok;log(result,unlocked?"identity_and_stop":"unlock_failed");return result;
    }
    Proof producers(std::uint64_t gen)noexcept override{
        if(!threads_||gen!=stopped_.generation||!lock())return Proof::Failed;
        nractive::Globals g{};const bool matches=read(g)&&field<std::uint8_t>(g,0x949B8)==1&&field<int>(g,nrproxy::rva_hip_dispositivo)==hip_device_&&threads_->matches(thread_snapshot(g,gen));
        const bool unlocked=unlock();if(!matches||!unlocked)return Proof::Failed;
        const auto result=threads_->poll();return result==nrtransition::ThreadPoll::Terminated?Proof::Complete:result==nrtransition::ThreadPoll::Pending?Proof::Pending:Proof::Failed;
    }
    Proof hip_barrier(std::uint64_t gen)noexcept override{
        if(producers(gen)!=Proof::Complete||!hip_imports_valid())return Proof::Failed;
        try{
            if(!hip_){hip_generation_=gen;hip_=std::make_unique<nrtransition::HipProof>(nrtransition::HipApi{reinterpret_cast<nrtransition::HipSetDevice>(base_+0x61DF0),reinterpret_cast<nrtransition::HipSynchronize>(base_+0x61C80),hip_device_,gen});}
            if(hip_generation_!=gen)return Proof::Failed;
            const auto r=hip_->poll();if(r.generation!=gen||r.device!=hip_device_)return Proof::Failed;
            return r.state==nrtransition::HipState::Succeeded?Proof::Complete:r.state==nrtransition::HipState::Pending?Proof::Pending:Proof::Failed;
        }catch(...){return Proof::Failed;}
    }
};
}
std::unique_ptr<Host> make_windows_host(void* base){return std::make_unique<WindowsHost>(base);}

bool slot_callback_publicavel(const void* image_base,const void* slot)noexcept{
    MEMORY_BASIC_INFORMATION page{};
    if(!image_base||!slot||!VirtualQuery(slot,&page,sizeof(page))||page.State!=MEM_COMMIT||page.Type!=MEM_IMAGE||page.AllocationBase!=image_base)return false;
    if(page.Protect!=PAGE_READWRITE&&page.Protect!=PAGE_WRITECOPY)return false;
    return reinterpret_cast<std::uintptr_t>(slot)-reinterpret_cast<std::uintptr_t>(page.BaseAddress)<=page.RegionSize-sizeof(void*);
}
}
