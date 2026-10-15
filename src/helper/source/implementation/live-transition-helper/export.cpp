#include "../../../runtime/nr_publico.h"
#include "multipass_runtime.h"
#if NR_SPIN_DRAW
#include "integration.h"
#endif
#include "pre_scale_state.h"
#include "reuso_state.h"
#include "../compose-controls/control_state.h"
#include "../compose-controls/realce.h"
#include "../async-reprojection/reproject_controls.h"
#include "../async-reprojection/recusa_log.h"
#include "present_integration.h"
#include "present_trace.h"
#include "../adapter-menu/adapter_menu.h"
#include "../live-transition-binding/live_binding.h"
#include "../live-recording-observer/compute_state.h"
#include "../pipeline-api.h"
#include "../universal-scale/coordinator.h"
#include "../universal-scale/d3d12_backend.h"
#include "../active-scheduling/legacy_fsr3_descriptor.h"
#include "../frame-limiter/frame_limiter.h"
#include "fg_nitidez.h"
#include "fg_tempo.h"
#include "../../../layout/menu_compacto.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <intrin.h>
#include "../../../runtime/native_ini_path.h"
#include "../../../runtime/log_fila.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <atomic>
#include <array>
#include <algorithm>
#include <optional>
#include <mutex>
#if NR_SPIN_DRAW
namespace { void spin_fault(const char* message) noexcept; }
#endif

extern "C" __declspec(dllexport) void NRMenuAppend_v1(void* base) noexcept {
    try{
        HMODULE self{};
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&NRMenuAppend_v1),&self))return;
        const auto appended=nradapter::append_from_base(base);
        if(appended==nradapter::AppendResult::Appended || appended==nradapter::AppendResult::AlreadyPresent) {
#if NR_SPIN_DRAW
            nrspin::install(base,spin_fault);
#endif
            nradapter::NativeHost host;
            if(nradapter::resolve_host(base,host) && host.multipass_requested && host.multipass_effective)
                (void)NRMultipassInstall_v1(base);
        }
    }catch(...){NRDIAG(OutputDebugStringA("NR live menu append refused\n");)}
}
namespace {

std::atomic<nrlive::PinnedBinding*> binding{};
std::atomic<void*> binding_base{};
#if NR_SPIN_DRAW
void spin_fault(const char* message) noexcept {
    if(auto* current=binding.load(std::memory_order_acquire))current->report_error(message);
}
#endif
std::atomic<bool> attempted{};
std::atomic<nrscale::Coordinator*> scale_coordinator{};
std::atomic<bool> host_resolution_owned{};

class BindingPipeline final:public nrscale::NeuralPipeline {
    nrlive::PinnedBinding& binding_;std::mutex mutex_;std::uint64_t sequence_{};
    struct Request {nrscale::Target target{};nrscale::Resources resources{};void* context{};std::uint64_t id{},menu_generation{};bool installed{},complete{};};
    std::optional<Request> current_;
    static bool same(const Request& a,const nrscale::Target& target,const nrscale::Resources& resources,void* context){
        return a.target.percent==target.percent&&a.target.original==target.original&&a.target.scaled==target.scaled&&a.context==context&&
          a.resources.input==resources.input&&a.resources.output==resources.output&&a.resources.motion==resources.motion&&a.resources.depth==resources.depth;
    }
public:
    explicit BindingPipeline(nrlive::PinnedBinding& value):binding_(value){}
    nrscale::PrepareStatus prepare(const nrscale::Target& target,const nrscale::Resources& resources,void* context,unsigned stage,std::uint64_t generation)noexcept override{
        std::lock_guard<std::mutex> lock(mutex_);
        if(current_&&current_->installed&&!current_->complete){const auto polled=binding_.poll_resolution(current_->id);if(polled==nrhost::Status::Ready||polled==nrhost::Status::DisabledReady)current_->complete=true;else if(polled==nrhost::Status::Failed)return nrscale::PrepareStatus::Failed;}
        const bool identical=current_&&same(*current_,target,resources,context);
        if(identical&&current_->installed)return nrscale::PrepareStatus::Ready;
        if(current_&&!identical&&!current_->complete)return nrscale::PrepareStatus::Pending;
        Request request=identical?*current_:Request{};
        if(!identical){request.target=target;request.resources=resources;request.context=context;request.menu_generation=generation;request.id=std::max(sequence_+1,generation);sequence_=request.id;}
        nrhost::Ticket ticket{};ticket.percent=target.percent;ticket.stage=stage;ticket.request_id=request.id;ticket.scene_width=target.scaled.render_w;ticket.scene_height=target.scaled.render_h;ticket.working_width=target.scaled.output_w;ticket.working_height=target.scaled.output_h;ticket.final_width=target.original.output_w;ticket.final_height=target.original.output_h;
        const auto ready=binding_.prepare_resolution(ticket);if(ready==nrhost::Status::Pending){current_=request;return nrscale::PrepareStatus::Pending;}if(ready==nrhost::Status::Failed)return nrscale::PrepareStatus::Failed;
        nrhost::Resources native{};native.context=context;native.input=resources.input;native.output=resources.output;native.motion=resources.motion;native.depth=resources.depth;
        const auto installed=binding_.install_resolution(ticket,native);if(installed==nrhost::Status::Failed)return nrscale::PrepareStatus::Failed;if(installed==nrhost::Status::Pending){current_=request;return nrscale::PrepareStatus::Pending;}
        request.id=ticket.request_id;request.installed=true;current_=request;return nrscale::PrepareStatus::Ready;
    }
    int dispatch(NrFfxDispatch original,void* context,void* descriptor,int nesting,const NrActiveSelectionV1& selection,unsigned* handled,NrActiveDispatchResultV1* result)override{
        return binding_.dispatch(original,context,descriptor,nesting,selection,handled,result);
    }
};

std::string menu_line(void* base,const nradapter::GatewayResult& request) noexcept {
    auto* current=binding.load(std::memory_order_acquire);if(!current||!request.state)return {};
    try{
        const auto snapshot=current->snapshot();
        auto* coordinator=scale_coordinator.load(std::memory_order_acquire);
        const bool universal=!host_resolution_owned.load(std::memory_order_acquire)&&coordinator;
        const auto scale=universal?coordinator->snapshot():nrscale::Coordinator::Snapshot{};
        const char* scale_diagnostic=universal?coordinator->diagnostic():"scale_backend_host";
        if(universal&&coordinator->sticky()){
            (void)nradapter::runtime_fail(base,request.state->generation,scale_diagnostic);
        }else if(snapshot.state==nrtransition::State::FailedFsrOnly){
            (void)nradapter::runtime_fail(base,request.state->generation,current->status());
        }else if(const auto generation=current->acknowledged_generation();generation&&*generation==request.state->generation&&snapshot.working_percent==request.state->requested.percent&&snapshot.effective_stage==(request.state->requested.pre_upscale?1u:2u)){
            (void)nradapter::runtime_acknowledge(base,*generation);
        }

        const char* diagnostico=current->status();if(!diagnostico)diagnostico="";
        {
            auto bits=[](float f){std::uint32_t u{};std::memcpy(&u,&f,sizeof u);return std::uint64_t(u);};
            const std::array<std::uint64_t,24> chave{std::uint64_t(snapshot.state),request.state->generation,snapshot.generation,
                std::uint64_t(request.state->requested.pre_upscale),std::uint64_t(request.state->requested.percent),
                std::uint64_t(snapshot.effective_stage),std::uint64_t(snapshot.working_percent),
                std::uint64_t(snapshot.render_width),std::uint64_t(snapshot.render_height),
                std::uint64_t(snapshot.output_width),std::uint64_t(snapshot.output_height),std::uint64_t(universal),
                scale.original.render_w,scale.original.render_h,scale.original.output_w,scale.original.output_h,
                scale.scaled.render_w,scale.scaled.render_h,scale.scaled.output_w,scale.scaled.output_h,
                std::uint64_t(snapshot.enabled),bits(snapshot.local_structure),bits(snapshot.skin_structure),
                snapshot.enabled_dispatches/64};
            static std::mutex chave_mutex;std::lock_guard<std::mutex> chave_lock(chave_mutex);
            static std::array<std::uint64_t,24> chave_anterior{};static bool tem_chave=false;
            static std::string escala_anterior,diagnostico_anterior;
            if(tem_chave&&chave==chave_anterior&&escala_anterior==scale_diagnostic&&diagnostico_anterior==diagnostico)return {};
            chave_anterior=chave;tem_chave=true;escala_anterior=scale_diagnostic;diagnostico_anterior=diagnostico;
        }
        FILETIME created{},exited{},kernel{},user{};
        if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user))return {};
        const auto creation=(std::uint64_t(created.dwHighDateTime)<<32)|created.dwLowDateTime;
        const std::string message=std::string("NR_LIVE state=")+std::to_string(static_cast<int>(snapshot.state))+
            " menu_generation="+std::to_string(request.state->generation)+
            " resource_generation="+std::to_string(snapshot.generation)+
            " requested_stage="+(request.state->requested.pre_upscale?"PRE":"POST")+
            " requested_percent="+std::to_string(request.state->requested.percent)+
            " effective_stage="+std::to_string(snapshot.effective_stage)+
            " effective_percent="+std::to_string(snapshot.working_percent)+
            " render="+std::to_string(snapshot.render_width)+"x"+std::to_string(snapshot.render_height)+
            " output="+std::to_string(snapshot.output_width)+"x"+std::to_string(snapshot.output_height)+
            " scale_backend="+(universal?"universal_d3d12":"host")+
            " scale_diagnostic="+scale_diagnostic+
            " original_render="+std::to_string(scale.original.render_w)+"x"+std::to_string(scale.original.render_h)+
            " original_output="+std::to_string(scale.original.output_w)+"x"+std::to_string(scale.original.output_h)+
            " internal_render="+std::to_string(scale.scaled.render_w)+"x"+std::to_string(scale.scaled.render_h)+
            " internal_output="+std::to_string(scale.scaled.output_w)+"x"+std::to_string(scale.scaled.output_h)+
            " backend_version=unverified"+
            " enabled="+std::to_string(snapshot.enabled)+
            " local_structure="+std::to_string(snapshot.local_structure)+
            " skin_structure="+std::to_string(snapshot.skin_structure)+" diagnostic="+diagnostico;
        const std::string identity=" pid="+std::to_string(GetCurrentProcessId())+" created="+std::to_string(creation);
        const auto key=message+identity+std::to_string(snapshot.enabled_dispatches/64);
        static std::mutex log_mutex;std::lock_guard<std::mutex> log_lock(log_mutex);
        static std::string previous;
        if(key==previous)return {};
        previous=key;

        std::ostringstream line;
        line<<message<<" observed_epoch="<<snapshot.observed_epoch
           <<" observed_resets="<<snapshot.observed_resets
           <<" observed_closes="<<snapshot.observed_closes
           <<" recording_primed="<<snapshot.recording_primed
           <<" flow_enabled="<<snapshot.enabled_dispatches<<" flow_jobs="<<snapshot.captured_jobs
           <<" flow_unsubmitted="<<snapshot.pending_unsubmitted<<" flow_submitted="<<snapshot.pending_submitted
           <<" async_pending_job="<<snapshot.async_pending_job<<" async_completed_job="<<snapshot.async_completed_job
           <<" compose_ok="<<snapshot.compose_accepted<<" compose_skip="<<snapshot.compose_skipped<<identity<<'\n';
        return line.str();
    }catch(...){ }
    return {};
}
void write_menu_line(const std::string& text) noexcept {
    if(text.empty())return;
    try{

        static std::mutex write_mutex;std::lock_guard<std::mutex> lock(write_mutex);
        static std::filesystem::path destino;
        if(destino.empty()){
            HMODULE self{};wchar_t path[32768]{};
            if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&NRMenuAppend_v1),&self))return;
            const auto length=GetModuleFileNameW(self,path,32768);
            if(!length||length>=32768)return;
            destino=std::filesystem::path(path).parent_path()/L"nr_live_transition.log";
        }

        nrlogfila::escrever(destino.c_str(),text);
    }catch(...){}
}
void update_menu(void* base,const nradapter::GatewayResult& request) noexcept {write_menu_line(menu_line(base,request));}

nradapter::GatewayResult snapshot_com_espera(void* base) noexcept {
    auto request=nradapter::runtime_snapshot(base);
    if(request.status==nradapter::GatewayStatus::Busy){
        LARGE_INTEGER freq{},start{},now{};QueryPerformanceFrequency(&freq);QueryPerformanceCounter(&start);
        const auto limit=start.QuadPart+freq.QuadPart*150/1000000;
        do{YieldProcessor();request=nradapter::runtime_snapshot(base);QueryPerformanceCounter(&now);}
        while(request.status==nradapter::GatewayStatus::Busy&&now.QuadPart<limit);
    }
    return request;
}
std::int32_t dispatch(void* base,nrabi::FfxDispatch original,void* context,
    void* descriptor,std::int32_t nesting,std::uint32_t* handled){
    if(handled)*handled=1;
    if(!original)return 6;

    struct Ritmo{bool upscale=false;~Ritmo(){if(upscale){nrfgtempo::evento('E',nrreproject::idade_composta.load(std::memory_order_relaxed));nrlimit::depois();}}} ritmo;

    if(descriptor&&nradapter::accessible(descriptor,sizeof(std::uint64_t),nradapter::Access::Read)){
        std::uint64_t type{};std::memcpy(&type,descriptor,sizeof(type));
        const auto effect=type&0xFFFF0000ull;
        if(effect==0x20000ull||effect==0x30000ull)nrpresenttrace::ffx_type(type);
        if(effect==0x20000ull){

            if(handled)*handled=0;
            if(type==0x20003ull){
                nrlive::note_frame_generation();nrfgtempo::evento('G',0);
                const auto r=original(context,descriptor);

                if(r==0){static const std::wstring ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&NRMenuAppend_v1));nrfgnitidez::aplicar(descriptor,ini);}
                return r;
            }
            return original(context,descriptor);
        }
        if(effect==0x30000ull)return original(context,descriptor);

        if(type==0x10001ull&&nesting==0){
            {static std::atomic<bool> lido{false};if(!lido.exchange(true,std::memory_order_relaxed)){nrfgtempo::ler(nrinipath::resolve(base,reinterpret_cast<const void*>(&NRMenuAppend_v1)));}}
            nrfgtempo::evento('U',nrreproject::idade_composta.load(std::memory_order_relaxed));
            nrlimit::ritmar(base);ritmo.upscale=true;}
    }
    if(nrpresentintegration::selected(base))return original(context,descriptor);
    auto request=snapshot_com_espera(base);
    if(request.status!=nradapter::GatewayStatus::Ok||!request.state)return original(context,descriptor);
    auto* current=binding.load(std::memory_order_acquire);
    const auto current_base=binding_base.load(std::memory_order_acquire);
    if(current&&current_base!=base)return original(context,descriptor);
    bool expected=false;
    if(!current&&attempted.compare_exchange_strong(expected,true,std::memory_order_acq_rel)){
        try{
            HMODULE self{};
            if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCWSTR>(&NRMenuAppend_v1),&self)){
                auto* created=new nrlive::PinnedBinding(base);
                auto gpu=nrscale::make_d3d12_backend();
                auto* neural=new BindingPipeline(*created);
                auto adapter=nrscale::make_d3d12_input_adapter();
                auto* coordinator=gpu?new nrscale::Coordinator(*gpu,*neural,adapter.get()):nullptr;
                if(!coordinator){delete neural;delete created;return original(context,descriptor);}
                (void)gpu.release();
                (void)adapter.release();
                binding_base.store(base,std::memory_order_release);
                binding.store(created,std::memory_order_release);
                scale_coordinator.store(coordinator,std::memory_order_release);
                current=created;
            }
        }catch(...){return original(context,descriptor);}
    }
    if(!current)current=binding.load(std::memory_order_acquire);
    if(!current)return original(context,descriptor);
    NrActiveSelectionV1 selection{};
    selection.size=sizeof(selection);selection.version=NR_ACTIVE_ABI_VERSION;
    selection.master_enabled=1;
    selection.stage=request.state->requested.pre_upscale?NR_ACTIVE_STAGE_PRE:NR_ACTIVE_STAGE_POST;
    selection.scale_percent=request.state->requested.percent;
    selection.generation=request.state->generation;
    nrffx::Descriptor scale_descriptor{};

    if(nesting==0&&nrffx::host_reset(descriptor))nrreusostate::pedir_cheio();
    bool decoded=nrffx::decode(descriptor,scale_descriptor);
    if(!decoded){nrffx::RawDescriptor raw{};if(nrffx::decode_raw(descriptor,raw)&&!raw.reset){scale_descriptor=raw;decoded=true;}}
    if(decoded && nesting==0){

        auto* bytes=static_cast<char*>(base);
        if(nradapter::accessible(bytes+0x949E0,4,nradapter::Access::Read) &&
           nradapter::accessible(bytes+0x94B9C,4,nradapter::Access::Write)){
            LONG frame{};std::memcpy(&frame,bytes+0x949E0,4);
            InterlockedExchange(reinterpret_cast<volatile LONG*>(bytes+0x94B9C),frame);
        }
    }
    {const auto geo=nrprescale::geometria(selection.stage==NR_ACTIVE_STAGE_PRE,current->snapshot().effective_stage,selection.scale_percent,decoded,
        scale_descriptor.render_w,scale_descriptor.render_h,scale_descriptor.output_w,scale_descriptor.output_h);

     if(decoded)nrprescale::quadro.store(nrprescale::pack_quadro({scale_descriptor.output_w,scale_descriptor.output_h,
         scale_descriptor.render_w,scale_descriptor.render_h,selection.stage==NR_ACTIVE_STAGE_POST}),std::memory_order_release);
     nrprescale::geometry.store(geo,std::memory_order_release);}
    NrActiveDispatchResultV1 result{};

    auto* coordinator=scale_coordinator.load(std::memory_order_acquire);
    const auto code=!host_resolution_owned.load(std::memory_order_acquire)&&coordinator?
        coordinator->dispatch(original,context,descriptor,nesting,selection,handled,&result):
        current->dispatch(original,context,descriptor,nesting,selection,handled,&result);
    update_menu(base,request);
    return code;
}
std::uint32_t compose(void* base,void* command,void* destination,std::uint32_t state,
    std::int32_t job,float exposure,std::int32_t alternate,std::uint32_t inline_mode){

    static const bool skip=GetEnvironmentVariableW(L"NR_DIAG_SKIP_COMPOSE",nullptr,0)>0;
    if(skip)return 1;
    const auto request=snapshot_com_espera(base);
    auto* current=binding.load(std::memory_order_acquire);
    if(request.status!=nradapter::GatewayStatus::Ok||!request.state||!current||binding_base.load(std::memory_order_acquire)!=base)return 1;
    auto saved=nrstate::save(command);
    if(!saved){nrrecusa::registrar("export_estado_da_lista",job,0,0,0);return 1;}
    if(current->compose(command,destination,state,job,exposure,alternate,inline_mode)!=0)return 1;

    const auto controls=nrcompose::state.requested.load();
    if(nrcompose::valid(controls)&&current->reproject(command,destination,state,job,exposure,controls)==0)
        nrcompose::state.recorded.store(controls);
    if(!saved->restore())current->report_error("async_composition_state_restore_failed");
    return 1;
}
void tick(void* base){

    if(nrmenucompacto::pedido&&base){
        auto& vis=*reinterpret_cast<volatile std::uint8_t*>(static_cast<std::uint8_t*>(base)+0x923B0);
        if(vis==0){nrmenucompacto::desenhando.store(true,std::memory_order_relaxed);vis=1;}
        nrmenucompacto::pedido=false;
    }
    const auto request=nradapter::runtime_snapshot(base);
    auto* current=binding.load(std::memory_order_acquire);
    if(request.status!=nradapter::GatewayStatus::Ok||!current||binding_base.load(std::memory_order_acquire)!=base)return;

    struct Call{void* base;const nradapter::GatewayResult* request;std::string line;};
    Call call{base,&request,{}};
    current->tick([](void* c){auto* x=static_cast<Call*>(c);x->line=menu_line(x->base,*x->request);},&call);
    write_menu_line(call.line);
}
void reload(void* base){(void)nradapter::runtime_reload(base);tick(base);}
constexpr nrabi::PipelineApiV1 api{40,1,dispatch,compose,tick,reload};
struct LegacyCall {nrabi::FfxDispatch original{};void* context{};void* descriptor{};};
thread_local LegacyCall* legacy_call{};
std::int32_t legacy_original(void* context,void*){
    auto* call=legacy_call;
    if(!call||call->context!=context)return 6;
    return call->original(context,call->descriptor);
}
std::int32_t dispatch_fsr3(void* base,nrabi::FfxDispatch original,void* context,
    void* descriptor,std::int32_t nesting,std::uint32_t* handled){
    if(handled)*handled=1;if(!original)return 6;
    if(nrpresentintegration::selected(base))return original(context,descriptor);
    if(nesting!=0)return original(context,descriptor);
    std::array<std::byte,nrffx::descriptor_size> generic{};
    const bool converted=nrffx::legacy_fsr3_to_generic(descriptor,generic.data(),generic.size());
    static std::atomic<bool> probed{};bool expected=false;

if(probed.compare_exchange_strong(expected,true,std::memory_order_acq_rel))try{
        HMODULE self{};wchar_t path[32768]{};
        if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&NRMenuAppend_v1),&self)){
            const auto length=GetModuleFileNameW(self,path,32768);if(length&&length<32768)
                std::ofstream(std::filesystem::path(path).parent_path()/L"nr_legacy_fsr3.log",std::ios::app)<<"entered=1 converted="<<converted<<" nesting="<<nesting
                  <<" render="<<*reinterpret_cast<const std::uint32_t*>(generic.data()+0x178)<<"x"<<*reinterpret_cast<const std::uint32_t*>(generic.data()+0x17c)
                  <<" output="<<*reinterpret_cast<const std::uint32_t*>(generic.data()+0x180)<<"x"<<*reinterpret_cast<const std::uint32_t*>(generic.data()+0x184)
                  <<" motion="<<*reinterpret_cast<const float*>(generic.data()+0x170)<<","<<*reinterpret_cast<const float*>(generic.data()+0x174)
                  <<" preExposure="<<*reinterpret_cast<const float*>(generic.data()+0x194)<<" reset="<<unsigned(generic[0x198])
                  <<" exposure="<<(*reinterpret_cast<void* const*>(generic.data()+0xa8)!=nullptr)<<'\n';
        }
    }
catch(...){ }
    if(!converted)return original(context,descriptor);
    LegacyCall call{original,context,descriptor};
    struct Scope {LegacyCall* previous;Scope(LegacyCall* value):previous(legacy_call){legacy_call=value;}~Scope(){legacy_call=previous;}} scope(&call);
    return dispatch(base,legacy_original,context,generic.data(),nesting,handled);
}
constexpr nrabi::LegacyPipelineApiV1 legacy_api{16,1,dispatch_fsr3};
nrhost::Status host_request(nrhost::Request* out)noexcept{
    if(!out||out->size!=sizeof(*out)||out->version!=1)return nrhost::Status::Failed;
    auto* current=binding.load(std::memory_order_acquire);auto* base=binding_base.load(std::memory_order_acquire);
    if(!current||!base)return nrhost::Status::Pending;
    const auto state=nradapter::runtime_snapshot(base);if(state.status!=nradapter::GatewayStatus::Ok||!state.state)return nrhost::Status::Pending;
    out->percent=state.state->requested.percent;out->stage=state.state->requested.pre_upscale?1:2;out->request_id=state.state->generation;
    out->current_generation=current->snapshot().generation;return nrhost::Status::Ready;
}
nrhost::Status host_set(unsigned percent)noexcept{
    auto* base=binding_base.load(std::memory_order_acquire);if(!base)return nrhost::Status::Pending;
    const auto state=nradapter::runtime_request_scale(base,percent);
    return state.status==nradapter::GatewayStatus::Ok?nrhost::Status::Ready:nrhost::Status::Failed;
}
nrhost::Status host_prepare(nrhost::Ticket* t)noexcept{
    if(!t)return nrhost::Status::Failed;host_resolution_owned.store(true,std::memory_order_release);auto* current=binding.load(std::memory_order_acquire);if(!current)return nrhost::Status::Pending;
    return current->prepare_resolution(*t);
}
nrhost::Status host_install(const nrhost::Ticket* t,const nrhost::Resources* r)noexcept{
    auto* current=binding.load(std::memory_order_acquire);if(!t||!r||!current)return nrhost::Status::Failed;return current->install_resolution(*t,*r);
}
nrhost::Status host_poll(std::uint64_t id)noexcept{
    auto* current=binding.load(std::memory_order_acquire);if(!current)return nrhost::Status::Pending;
    const auto result=current->poll_resolution(id);
    if(result==nrhost::Status::Ready||result==nrhost::Status::DisabledReady)(void)nradapter::runtime_acknowledge(binding_base.load(std::memory_order_acquire),id);
    return result;
}
nrhost::Status host_cancel(std::uint64_t id)noexcept{
    auto* current=binding.load(std::memory_order_acquire);if(!current)return nrhost::Status::Ready;return current->cancel_resolution(id);
}
constexpr nrhost::Api host_api{sizeof(nrhost::Api),1,host_request,host_set,host_prepare,host_install,host_poll,host_cancel};
}
extern "C" __declspec(dllexport) const nrabi::PipelineApiV1* NRGetPipelineApi_v1() noexcept{return &api;}
extern "C" __declspec(dllexport) const nrabi::LegacyPipelineApiV1* NRGetLegacyPipelineApi_v1() noexcept{return &legacy_api;}
extern "C" __declspec(dllexport) const nrhost::Api* NRGetHostResolutionApi_v1() noexcept{return &host_api;}
