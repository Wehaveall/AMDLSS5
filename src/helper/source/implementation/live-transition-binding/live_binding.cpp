#include "../../../runtime/nr_publico.h"
#include "live_binding.h"
#include "wait_trace.h"
#include "retry_policy.h"
#include "../active-scheduling/ffx_descriptor.h"
#include "../async-reprojection/reproject_controls.h"
#include "../async-reprojection/recusa_log.h"
#include "../../../runtime/rosto_jogador_state.h"
#include "../active-scheduling/admission_reason.h"
#include "../compat-diag/compat.h"
#include "../../../runtime/pre_scale_state.h"
#include "../../../runtime/post_teto.h"
#include "../../../runtime/reuso_state.h"
#include "../../../runtime/overlay_amd.h"
#include "../../../runtime/rede_cada.h"
#include "../../../runtime/custo_perfil.h"
#include "../../../runtime/expo_trava.h"
#include <atomic>
#include <cstring>
#include <limits>
#include <cstdio>
#include <cmath>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>
#include <optional>
namespace nrlive {
namespace wt=nr::wait_trace;
namespace {
using nrtransition::Kind;using nrtransition::State;using nrtransition::Stage;
template<class T>T field(const nractive::Globals& g,unsigned r)noexcept{T v{};std::memcpy(&v,g.data()+r-nractive::globals_begin,sizeof(v));return v;}
nractive::Config fixed_config(const nractive::Globals& g){auto c=nractive::configuration(g);c[0]=0;return c;}
nractive::Config lifecycle_config(const nractive::Globals& g){

    auto c=fixed_config(g);c[7]=0;c[8]=0;c[9]=0;c[10]=0;c[11]=0;c[12]=0;return c;
}

unsigned config_mask(const nractive::Config& a,const nractive::Config& b){
    unsigned m=0;for(unsigned i=0;i<a.size()&&i<32;++i)if(a[i]!=b[i])m|=1u<<i;return m;
}
bool structure_valid(const nractive::Globals& g){
    const float local=field<float>(g,0x94AA4),skin=field<float>(g,0x94AA8);
    return std::isfinite(local)&&local>=0.f&&local<=2.f&&std::isfinite(skin)&&
        (skin==-1.f||(skin>=0.f&&skin<=2.f));
}
bool supported_percent(unsigned percent){for(const auto value:{100u,85u,70u,65u,50u,40u,30u})if(percent==value)return true;return false;}
constexpr int NoHip=std::numeric_limits<int>::min();
std::atomic<PinnedBinding*> published{};
std::uint64_t begin_callback(void* q,unsigned n,void** l)noexcept{auto* p=published.load();return p?p->queue_begin(q,n,l):0;}
void end_callback(std::uint64_t t,void* q,unsigned n,void** l)noexcept{auto* p=published.load();if(p)p->queue_end(t,q,n,l);}
void hip_callback(int s,int j)noexcept{auto* p=published.load();if(p)p->hip_complete(s,j);}
unsigned reset_callback(unsigned k)noexcept{auto* p=published.load();return p?p->reset_gate(k):0;}
const NRLifecycleCallbacks callbacks{begin_callback,end_callback,hip_callback,reset_callback};
constexpr unsigned PresentQueueSlots=16;
std::atomic<std::uint64_t> present_queues[PresentQueueSlots]{};
std::atomic<unsigned> present_queue_next{0};
std::atomic<bool> frame_generation{false};
}
void note_present_queue(std::uint64_t queue)noexcept{
    if(!queue||present_queue_known(queue))return;
    const unsigned slot=present_queue_next.fetch_add(1,std::memory_order_relaxed)%PresentQueueSlots;
    present_queues[slot].store(queue,std::memory_order_release);
}
bool present_queue_known(std::uint64_t queue)noexcept{
    if(!queue)return false;
    for(auto& slot:present_queues)if(slot.load(std::memory_order_acquire)==queue)return true;
    return false;
}
void forget_present_queues()noexcept{for(auto& slot:present_queues)slot.store(0);present_queue_next.store(0);frame_generation.store(false);}
void note_frame_generation()noexcept{frame_generation.store(true,std::memory_order_relaxed);nrreproject::fg_marca();}
bool frame_generation_seen()noexcept{return frame_generation.load(std::memory_order_relaxed);}
bool post_reset_matches(const nractive::Globals& before,const nractive::Globals& after)noexcept{
    for(auto r:{0x949A8u,0x948E0u,0x94708u,0x94800u,0x94758u,0x949D0u,0x94718u,0x94720u})if(field<std::uint64_t>(after,r))return false;
    for(auto r:{0x949B0u,0x948E8u,0x949DCu,0x94740u,0x94744u,0x94748u,0x948F0u,0x948C4u,0x94848u})if(field<unsigned>(after,r))return false;
    for(auto r:{0x949B8u,0x948F5u,0x949F4u,0x94711u})if(field<std::uint8_t>(after,r))return false;
    for(auto r:{0x949BCu,0x949C8u,0x949D8u,0x949E4u,0x94A60u,0x94808u})if(field<int>(after,r)!=-1)return false;
    for(auto r:{0x94850u,0x94858u,0x94860u,0x94868u})if(field<std::uint64_t>(after,r))return false;
    if(field<std::uint16_t>(after,0x94870))return false;
    for(auto c:{0x94298u,0x94398u,0x94498u,0x94598u}){

        for(unsigned off=0x48;off<0x78;++off)
            if(field<std::uint8_t>(before,c+off)!=field<std::uint8_t>(after,c+off))return false;

        for(auto off:{0u,8u,0x10u,0x18u,0x40u,0x80u,0x88u,0x90u,0x98u,0xA0u,0xA8u,0xB0u,0xB8u,0xC0u,0xC8u,0xD0u,0xD8u,0xE0u,0xE8u,0xF0u,0xF8u})if(field<std::uint64_t>(after,c+off))return false;
        if(field<unsigned>(after,c+0x30)||field<unsigned>(after,c+0x34)||field<int>(after,c+0x38)!=-1||field<std::uint8_t>(after,c+0x78))return false;
        if(field<unsigned>(after,c+0x3C)!=field<unsigned>(before,c+0x3C))return false;
    }
    for(auto c:{0x946C0u,0x946E0u,0x948F8u,0x94918u,0x94938u,0x94958u,0x94A08u})for(unsigned i=0;i<4;++i)if(field<std::uint64_t>(after,c+8*i))return false;
    for(unsigned i=0;i<4;++i)if(field<unsigned>(after,0x94A28+4*i))return false;
    for(unsigned i=0;i<0x60;++i)if(field<std::uint8_t>(after,0x94780+i))return false;
    if(field<unsigned>(after,0x94844)!=field<unsigned>(after,0x94840))return false;
    for(auto r:{0x93F38u,0x93F40u,0x948B8u,0x949C0u,0x94728u,0x94760u,0x94A00u,0x94A38u,0x94A40u,0x94A48u,0x94A50u})if(field<std::uint64_t>(before,r)!=field<std::uint64_t>(after,r))return false;
    for(auto r:{0x94A58u,0x94A5Cu,0x94BA0u})if(field<unsigned>(before,r)!=field<unsigned>(after,r))return false;
    return fixed_config(before)==fixed_config(after);
}
struct PinnedBinding::Impl {
    std::recursive_mutex state_mutex;
    std::unique_ptr<Host> host;std::unique_ptr<nrtransition::Controller> controller;std::unique_ptr<nrfence::OwnedFence> fence;
    NrActiveSchedulerV1 scheduler{},retired_scheduler{},baseline{};bool retired_valid{},baseline_valid{};nractive::Config config{};nractive::Globals reset_before{},stopped_globals{},queue_before{};
    bool valid{},production{},failed{},in_dispatch{},in_capture{},attempted{},maintenance{},reset_consumed{},reset_returned{},layout_used{},stopped{},producer_closed{};

    bool restage_pending{};unsigned restage_count{};std::uint64_t restage_window{};

    static constexpr std::uint64_t abandono_ms=1000;
    bool abandono_armado{};std::uint64_t abandono_desde{};unsigned abandonos{};
    bool pode_aposentar()noexcept{
        const auto g=controller->generation();
        if(recording->can_retire(g)){abandono_armado=false;return true;}
        if(!recording->retirable_if_abandoned(g)){abandono_armado=false;return false;}
        const auto now=host->now();
        if(!abandono_armado){abandono_armado=true;abandono_desde=now;return false;}
        if(now-abandono_desde<abandono_ms)return false;
        ++abandonos;diagnostic="ffx_geometry_abandoned_list_retiring";return true;
    }
    bool reproject_authorized{},capture_invoked{};void* reproject_command{};void* reproject_destination{};int reproject_job{};
    int async_pending_job{},last_completed_async{};std::uint64_t async_started{};

    int prev_completed_async{},reproject_job_nativo{};bool reproject_segura{};unsigned segura_seguidos{};

    std::uint64_t frame_count{},job_frame[16]{},age_waits{},age_wait_ms{},age_wait_timeouts{};

    std::uint64_t rede_cada_quadro{};bool rede_cada_tem{};

    int first_used_job{},window_job{};long long window_left_us{};nrjob::Tuning tuning{};bool tuning_read{};
    unsigned owner{};std::uint64_t device{},queue{},presentation_queue{},proof_device{},proof_queue{},menu_generation{},epoch{},ticket_sequence{},fence_sequence{},batch_fence{};
    bool relevant_wrapper{},internal_wrapper{};unsigned wrapper_count{},wrapper_thread{};std::array<void*,64> wrapper_lists{};
    Stage requested{Stage::Pre};bool enabled{};float local_structure{},skin_structure{};const char* diagnostic="not_admitted";
    char queue_diagnostic[512]{};

    std::atomic<std::uint64_t> fast_queue{0},fast_observed{0},fast_list{0};std::atomic<bool> fast_live{false};

    struct Dispatched{void* command{};nrrecord::Observer* observer{};};
    static constexpr unsigned DispatchedSlots=8;
    Dispatched dispatched[DispatchedSlots]{};unsigned dispatched_next{};
    std::atomic<std::uint64_t> fast_dispatched[DispatchedSlots]{};
    bool fg_queue_learning{true},render_queue_proven{},render_proof_reported{};std::uint64_t render_proof_deferrals{};
    const Dispatched* dispatched_find(void* command)const noexcept{
        if(command)for(const auto& d:dispatched)if(d.command==command)return &d;
        return nullptr;
    }
    void dispatched_note(void* command,nrrecord::Observer* observer)noexcept{
        if(!command||!observer)return;
        for(auto& d:dispatched)if(d.command==command){d.observer=observer;return;}
        dispatched[dispatched_next++%DispatchedSlots]={command,observer};
    }
    void render_proved()noexcept{render_queue_proven=true;for(auto& d:dispatched)d={};dispatched_next=0;}

    bool render_learning()const noexcept{
        return fg_queue_learning&&!render_queue_proven&&frame_generation_seen()&&!attempted&&!list&&!baseline_valid&&
               !native_job&&queue==presentation_queue;
    }

    bool adopt_render_queue(std::uint64_t selected){
        auto adopted=host->fence_on_queue(1,selected);
        if(!adopted||adopted->snapshot().status!=nrfence::Status::Ready||adopted->snapshot().identity.device!=proof_device){fail("fsr_queue_device_or_fence_failed");return false;}
        const auto proof=adopted->snapshot().identity;
        auto fresh=std::make_unique<nrtransition::Controller>(nrtransition::Config{proof.device,proof.queue,requested,enabled,1,transition_deadline_ms},host->now());
        fence=std::move(adopted);controller=std::move(fresh);queue=selected;proof_queue=proof.queue;render_proved();
        diagnostic="fsr_render_queue_adopted_before_neural";return true;
    }
    void mirror()noexcept{
        for(unsigned i=0;i<DispatchedSlots;++i)fast_dispatched[i].store(reinterpret_cast<std::uint64_t>(dispatched[i].command),std::memory_order_relaxed);
        fast_queue.store(queue,std::memory_order_relaxed);
        fast_observed.store(reinterpret_cast<std::uint64_t>(observed_command),std::memory_order_relaxed);
        fast_list.store(reinterpret_cast<std::uint64_t>(list),std::memory_order_relaxed);
        fast_live.store(valid&&!failed&&controller!=nullptr,std::memory_order_release);
    }

    bool fast_foreign(void* q,unsigned count,void** lists,unsigned thread)const noexcept{
        if(!fast_live.load(std::memory_order_acquire)||count>64||(!lists&&count))return false;
        const auto render=fast_queue.load(std::memory_order_relaxed);
        if(!render||(reinterpret_cast<std::uint64_t>(q)==render&&thread==owner))return false;
        const auto observed=fast_observed.load(std::memory_order_relaxed),tracked=fast_list.load(std::memory_order_relaxed);
        for(unsigned i=0;i<count;++i){const auto l=reinterpret_cast<std::uint64_t>(lists[i]);if(l&&(l==observed||l==tracked))return false;

            if(l)for(const auto& d:fast_dispatched)if(d.load(std::memory_order_relaxed)==l)return false;}
        return true;
    }
    struct MirrorOnExit{Impl& p;~MirrorOnExit(){p.mirror();}};
    nrrecord::Observer* recording{};void* observed_command{};std::uint64_t observed_batch_epoch{},wrapper_epoch{},wrapper_queue{};
    bool capture_deferred{};

    static constexpr std::uint64_t transition_deadline_ms=30000;
    nrretry::Policy list_switch_retry{{0,0,30000}},async_wait_retry{{0,0,25000}},frame_wait_retry{{0,0,25000}};
    std::uint64_t list_switch_deferrals{};bool list_switch_stuck_reported{};

    nrposteto::Estado& teto_estado=nrposteto::estado_latencia;std::int64_t teto_qpc_anterior{};double teto_quadro_ms{},teto_job_pre{},teto_px_pre{};bool teto_em_post{};
    void passo_teto_post()noexcept{

        if(!nrposteto::automatico()){nrprescale::post_teto_centi.store(10000u,std::memory_order_relaxed);return;}
        const auto agora=nrjob::qpc();
        if(teto_qpc_anterior>0){const double ms=nrjob::timeline().ms(agora-teto_qpc_anterior);
            if(ms>0&&ms<250)teto_quadro_ms=teto_quadro_ms>0?teto_quadro_ms+.05*(ms-teto_quadro_ms):ms;}
        teto_qpc_anterior=agora;
        const auto efetivo=controller?controller->effective():std::optional<Stage>{};
        const double job_ms=double(nrjob::timeline().eval_ema_us())/1000.;
        const auto q=nrprescale::unpack_quadro(nrprescale::quadro.load(std::memory_order_acquire));

        if(requested!=Stage::Post){

            if(efetivo&&*efetivo==Stage::Pre&&job_ms>0&&q.render_w&&q.render_h){const double e=nrprescale::efetivo.load(std::memory_order_relaxed)/100.;
                teto_job_pre=job_ms;teto_px_pre=double(q.render_w)*q.render_h*e*e;}

            if(q.saida_w&&q.saida_h){const double a=teto_estado.teto;
                if(nrposteto::entrar_post(teto_estado,std::uint64_t(q.saida_w)*q.saida_h)!=a){char d[160];NRLOGF(d,sizeof d,"teto %.1f%% -> %.1f%% (PRE) saida=%ux%u provado=%.0f px",
                    a,teto_estado.teto,q.saida_w,q.saida_h,teto_estado.px_provado);nrcompat::report("post_teto_inicio",d);}}
            teto_em_post=false;
        }else{
            nrposteto::Entrada x{};const double pedido=nrprescale::post_percent(nrprescale::post_chave.load(std::memory_order_relaxed));
            x.pedido=pedido>0?pedido:100.;x.quadro_ms=teto_quadro_ms;x.job_ms=job_ms;x.agora_ms=host->now();
            x.saida_px=std::uint64_t(q.saida_w)*q.saida_h;x.teto_vram=double(nrposteto::teto_vram_centi.load(std::memory_order_relaxed))/100.;
            double antes=teto_estado.teto;
            if(!teto_em_post){

                teto_em_post=true;
                if(nrposteto::entrar_post(teto_estado,x.saida_px)!=antes){char d[160];NRLOGF(d,sizeof d,"teto %.1f%% -> %.1f%% pedido=%.1f%% saida=%ux%u provado=%.0f px",
                    antes,teto_estado.teto,x.pedido,q.saida_w,q.saida_h,teto_estado.px_provado);nrcompat::report("post_teto_inicio",d);antes=teto_estado.teto;}
                double p=x.pedido<teto_estado.teto?x.pedido:teto_estado.teto;if(x.teto_vram>0&&x.teto_vram<p)p=x.teto_vram;p/=100.;
                x.job_ms=nrposteto::job_previsto(teto_job_pre,teto_px_pre,double(x.saida_px)*p*p);
                if(x.job_ms>0){teto_estado.trocou=false;(void)nrposteto::passo_latencia(teto_estado,x,false);}
                teto_estado.ultima_ms=teto_estado.ultima_janela_ms=x.agora_ms;teto_estado.trocou=true;teto_estado.boas=0;
            }else if(efetivo&&*efetivo==Stage::Post)(void)nrposteto::passo_latencia(teto_estado,x);
            if(teto_estado.teto!=antes){char d[160];NRLOGF(d,sizeof d,"teto %.1f%% -> %.1f%% pedido=%.1f%% quadro=%.2f ms rede=%.2f ms L=%.2f",antes,teto_estado.teto,x.pedido,x.quadro_ms,x.job_ms,
                x.quadro_ms>0?(x.job_ms+nrposteto::folga_ms)/x.quadro_ms:0.);nrcompat::report("post_teto_latencia",d);}
        }
        const double vram=double(nrposteto::teto_vram_centi.load(std::memory_order_relaxed))/100.;
        const double teto=teto_estado.teto<vram?teto_estado.teto:vram;
        const unsigned centi=unsigned(teto*100.+.5);
        if(nrprescale::post_teto_centi.exchange(centi,std::memory_order_relaxed)!=centi&&vram<100.&&teto==vram){char d[120];
            NRLOGF(d,sizeof d,"teto_vram=%.1f%% folga_pre=%llu MB",vram,static_cast<unsigned long long>(nrposteto::folga_pre.load()>>20));nrcompat::report("post_teto_vram",d);}
    }
    unsigned ini_clamps{};
    std::uint64_t enabled_dispatches{},captured_jobs{},pending_unsubmitted{},pending_submitted{},compose_accepted{},compose_skipped{};

    std::vector<void*> rotating_resources;
    void rebind_equivalent(NrFfxDispatch original,void* context,void* descriptor){
        if(!scheduler.frozen||list||native_job||wrapper_open||in_capture||maintenance||candidate_pending||original!=scheduler.original||context!=scheduler.context)return;
        nrffx::Descriptor d{};if(!nrffx::decode(descriptor,d))return;
        void* objects[5]{d.input,d.output,d.motion,d.depth,d.exposure};
        for(unsigned n=0;n<5;++n){
            if(!objects[n]){if(scheduler.resources[n])return;continue;}
            NrResourceDesc rd{};std::uint64_t dev{};
            if(!host->describe(objects[n],rd)||std::memcmp(&rd,&scheduler.descriptions[n],sizeof(rd))||!host->device_of(objects[n],dev)||dev!=scheduler.selected_device)return;
        }
        for(unsigned n=0;n<5;++n){
            auto* object=objects[n];if(!object||object==scheduler.resources[n])continue;
            bool owned=false;for(unsigned r=0;r<scheduler.retained_count;++r)owned|=scheduler.retained[r]==object;
            owned|=std::find(rotating_resources.begin(),rotating_resources.end(),object)!=rotating_resources.end();
            if(!owned){
                if(rotating_resources.size()>=256){fail("resource_rotation_capacity_exceeded");return;}
                rotating_resources.reserve(rotating_resources.size()+1);
                if(!host->retain(object)){fail("resource_rotation_retain_failed");return;}
                rotating_resources.push_back(object);
            }
        }
        for(unsigned n=0;n<5;++n){scheduler.resources[n]=objects[n];if(baseline_valid)baseline.resources[n]=objects[n];}

        auto retired=std::remove_if(rotating_resources.begin(),rotating_resources.end(),[&](void* resource){
            for(auto* object:objects)if(object==resource)return false;
            host->release(resource);return true;
        });
        rotating_resources.erase(retired,rotating_resources.end());
    }

    bool baseline_equivalent(char* campo,std::size_t nc,char* detalhe,std::size_t nd)noexcept{
        static const char* const nome[5]{"cor","saida","mv","prof","exp"};
        void* adotar[5]{};
        for(unsigned i=0;i<5;++i){
            void* const agora=scheduler.resources[i];void* const antes=baseline.resources[i];
            const NrResourceDesc& a=baseline.descriptions[i];const NrResourceDesc& b=scheduler.descriptions[i];
            const bool mesma=std::memcmp(&a,&b,sizeof a)==0;
            if(agora==antes&&mesma)continue;
            std::uint64_t dev{};
            if(agora&&antes&&mesma&&host->device_of(agora,dev)&&dev==scheduler.selected_device){adotar[i]=agora;continue;}
            if(!agora!=!antes)NRLOGF(campo,nc,"%s %s",nome[i],agora?"apareceu":"sumiu");
            else if(mesma)NRLOGF(campo,nc,"%s dispositivo",nome[i]);
            else if(a.format!=b.format)NRLOGF(campo,nc,"%s formato %u->%u",nome[i],a.format,b.format);
            else if(a.width!=b.width||a.height!=b.height)NRLOGF(campo,nc,"%s %llux%u->%llux%u",nome[i],static_cast<unsigned long long>(a.width),a.height,static_cast<unsigned long long>(b.width),b.height);
            else if(a.flags!=b.flags)NRLOGF(campo,nc,"%s flags 0x%x->0x%x",nome[i],a.flags,b.flags);
            else if(a.mip_levels!=b.mip_levels||a.sample_count!=b.sample_count||a.depth_or_array_size!=b.depth_or_array_size||a.dimension!=b.dimension)
                NRLOGF(campo,nc,"%s dim/mips/amostras",nome[i]);
            else NRLOGF(campo,nc,"%s layout/alinhamento",nome[i]);
            NRLOGF(detalhe,nd,"papel=%s ponteiro=%p->%p fmt=%u->%u tam=%llux%u->%llux%u flags=0x%x->0x%x dim=%u->%u mips=%u->%u amostras=%u->%u camadas=%u->%u layout=%u->%u alinhamento=%llu->%llu",
                nome[i],antes,agora,a.format,b.format,static_cast<unsigned long long>(a.width),a.height,static_cast<unsigned long long>(b.width),b.height,
                a.flags,b.flags,a.dimension,b.dimension,unsigned(a.mip_levels),unsigned(b.mip_levels),a.sample_count,b.sample_count,
                unsigned(a.depth_or_array_size),unsigned(b.depth_or_array_size),a.layout,b.layout,
                static_cast<unsigned long long>(a.alignment),static_cast<unsigned long long>(b.alignment));
            return false;
        }

        for(unsigned i=0;i<5;++i)if(adotar[i])baseline.resources[i]=adotar[i];
        return true;
    }

    bool geometry_changed(NrFfxDispatch original,void* context,void* descriptor){
        if(!scheduler.frozen||scheduler.poisoned||list||native_job||wrapper_open||in_capture||maintenance||candidate_pending||
           resolution_pending||restage_pending||!controller||controller->state()!=State::Active||original!=scheduler.original)return false;
        nrffx::Descriptor d{};if(!nrffx::decode(descriptor,d))return false;

        if(d.motion_x!=static_cast<float>(d.render_w)||d.motion_y!=static_cast<float>(d.render_h))return false;
        void* objects[5]{d.input,d.output,d.motion,d.depth,d.exposure};bool resized=false;
        for(unsigned n=0;n<5;++n){
            if(!objects[n]!=!scheduler.resources[n])return false;if(!objects[n])continue;
            NrResourceDesc rd{};std::uint64_t dev{};const auto& old=scheduler.descriptions[n];
            if(!host->describe(objects[n],rd)||!host->device_of(objects[n],dev)||dev!=scheduler.selected_device||rd.dimension!=old.dimension||
               rd.format!=old.format||rd.flags!=old.flags||rd.depth_or_array_size!=old.depth_or_array_size||rd.mip_levels!=old.mip_levels||rd.sample_count!=old.sample_count)return false;
            const std::uint64_t w=n==1?d.output_w:n==4?1:d.render_w;const std::uint32_t h=n==1?d.output_h:n==4?1:d.render_h;
            if(rd.width!=w||rd.height!=h)return false;
            resized|=rd.width!=old.width||rd.height!=old.height;
        }
        return resized||context!=scheduler.context;
    }
    bool resolution_pending{},resolution_installed{},host_reset_clean{},candidate_pending{};
    nrhost::Ticket resolution_ticket{};nrhost::Resources candidate{};
    std::uint64_t completed_request{};unsigned working_percent=100;
    void* list{};std::uint64_t list_epoch{},ticket{};int native_job{},previous_published{};
    bool submitted{},wrapper_open{},sealed{},controller_signal{},trace_fence_observed{};
    std::atomic<int> expected_job{0},hip_status{NoHip};std::atomic<unsigned> expected_worker{0};std::atomic<bool> foreign_failure{false};
    explicit Impl(std::unique_ptr<Host> h):host(std::move(h)){wt::InitializeFromEnvironment();nr_active_scheduler_init_v1(&scheduler);valid=host&&host->verify();if(valid){owner=host->thread();diagnostic="pinned_waiting_first_dispatch";}else{diagnostic=host?host->verification_diagnostic():"host_missing";report_failure(diagnostic,nullptr,nullptr,nullptr);}}
    const nrjob::Tuning& async_tuning()noexcept{if(!tuning_read){tuning=host->async_tuning();tuning_read=true;}return tuning;}

    double wait_for_job(unsigned us,bool& adiantou)noexcept{
        const int job=async_pending_job;const auto inicio=nrjob::qpc();adiantou=false;
        const int r=host->wait_async_completion(job,us);
        if(r<0)return -1;
        pump();
        if(r==1){const auto limite=nrjob::qpc()+nrjob::timeline().frequency()/1000;
            while(!failed&&async_pending_job==job&&nrjob::qpc()<limite){(void)host->wait_async_completion(job,0);pump();}}
        adiantou=!failed&&async_pending_job!=job;
        return nrjob::timeline().ms(nrjob::qpc()-inicio);
    }

    void report_failure(const char* why,const char* id,const char* detalhe,const char* campo)noexcept{
        char gid[64];if(id)std::snprintf(gid,sizeof gid,"%s",id);else nrcompat::gate_id(why,gid,sizeof gid);
        char texto[1200];
        NRLOGF(texto,sizeof texto,"estado=7 diagnostico=%s%s%s jobs=%llu despachos=%llu geracao=%llu cerca=\"%s\"",why?why:"",
            detalhe?" ":"",detalhe?detalhe:"",static_cast<unsigned long long>(captured_jobs),static_cast<unsigned long long>(enabled_dispatches),
            static_cast<unsigned long long>(controller?controller->generation():0),fence?fence->failure():"");
        nrcompat::block(nrcompat::Origem::Binding,gid,texto,campo);
    }
    void fail(const char* why)noexcept{fail_with(why,nullptr,nullptr,nullptr);}
    void fail_with(const char* why,const char* id,const char* detalhe,const char* campo)noexcept{if(!failed)report_failure(why,id,detalhe,campo);wt::Record(wt::Event::Failure,native_job,queue,batch_fence);failed=true;diagnostic=why;if(controller)controller->accept({Kind::EvidenceLost,controller->generation(),proof_device,proof_queue,0,0,false},host->now());}
    bool thread_ok()noexcept{return host&&host->thread()!=0;}
    void event(Kind k,std::uint64_t id=0,std::uint64_t related=0,bool success=true)noexcept{
        if(!controller||failed)return;controller->accept({k,controller->generation(),proof_device,proof_queue,id,related,success},host->now());
        if(controller->state()==State::FailedFsrOnly){report_failure(controller->failure(),nullptr,nullptr,nullptr);failed=true;diagnostic=controller->failure();}
    }
    bool globals(nractive::Globals& g)noexcept{
        if(!host->read(g)){fail("global_snapshot_failed");return false;}

        {const float escala=field<float>(g,0x94AAC);if(std::isfinite(escala)&&escala>0.f)nrreproject::escala_nativa.store(escala,std::memory_order_relaxed);}
        if(!structure_valid(g)||field<unsigned>(g,0x94AB0)>1u){

            const float local=field<float>(g,0x94AA4),skin=field<float>(g,0x94AA8);const unsigned mask=field<unsigned>(g,0x94AB0);
            const float fixed_local=std::isfinite(local)?(std::min)((std::max)(local,0.f),2.f):1.f;
            const float fixed_skin=!std::isfinite(skin)||skin<0.f?-1.f:(std::min)(skin,2.f);
            const unsigned fixed_mask=mask>1u?1u:mask;
            const bool structure=!structure_valid(g);
            char detalhe[200],campo[48];
            NRLOGF(detalhe,sizeof detalhe,"LocalStructure=%.6g SkinStructure=%.6g faixa=0..2 (SkinStructure -1 = auto) UseAutoMask=%u faixa=0..1",
                          static_cast<double>(local),static_cast<double>(skin),mask);
            const bool local_ok=std::isfinite(local)&&local>=0.f&&local<=2.f;
            if(structure)NRLOGF(campo,sizeof campo,"%s=%.3g",local_ok?"SkinStructure":"LocalStructure",static_cast<double>(local_ok?skin:local));
            else NRLOGF(campo,sizeof campo,"UseAutoMask=%u",mask);
            const bool local_bad=!(std::isfinite(local)&&local>=0.f&&local<=2.f);
            const bool skin_bad=!(std::isfinite(skin)&&(skin==-1.f||(skin>=0.f&&skin<=2.f)));
            const unsigned which=(local_bad?1u:0u)|(skin_bad?2u:0u)|(mask>1u?4u:0u);
            if(!host->clamp_structure(fixed_local,fixed_skin,fixed_mask,which)||!host->read(g)||!structure_valid(g)||field<unsigned>(g,0x94AB0)>1u){
                fail_with(structure?"invalid_structure_scalar":"invalid_auto_mask",nullptr,detalhe,campo);return false;}
            char limitado[320];
            NRLOGF(limitado,sizeof limitado,"%s -> LocalStructure=%.6g SkinStructure=%.6g UseAutoMask=%u (limitado no proxy, rede segue)",
                          detalhe,static_cast<double>(fixed_local),static_cast<double>(fixed_skin),fixed_mask);
            nrcompat::report("ini_value_clamped",limitado);
            ++ini_clamps;diagnostic="ini_value_clamped";
        }

        if(controller&&(field<std::uint64_t>(g,0x93F38)!=device||(field<std::uint64_t>(g,0x93F40)!=presentation_queue&&field<std::uint64_t>(g,0x93F40)!=queue&&!present_queue_known(field<std::uint64_t>(g,0x93F40)))||lifecycle_config(g)!=config||(field<int>(g,0x949BC)!=-1&&field<int>(g,0x949BC)!=async_pending_job&&field<int>(g,0x949BC)!=native_job))){
            NRLOGF(queue_diagnostic,sizeof(queue_diagnostic),"runtime_drift_device_%u_queue_%llx_present_%llx_render_%llx_config_%u_mask_%x_async_%d",
              field<std::uint64_t>(g,0x93F38)==device?1u:0u,
              static_cast<unsigned long long>(field<std::uint64_t>(g,0x93F40)),
              static_cast<unsigned long long>(presentation_queue),static_cast<unsigned long long>(queue),
              lifecycle_config(g)==config?1u:0u,config_mask(lifecycle_config(g),config),field<int>(g,0x949BC));
            const auto q=field<std::uint64_t>(g,0x93F40);
            const char* id=field<std::uint64_t>(g,0x93F38)!=device?"runtime_drift_device":
                (q!=presentation_queue&&q!=queue&&!present_queue_known(q))?"runtime_drift_queue":
                lifecycle_config(g)!=config?"runtime_drift_config":"runtime_drift_async_job";
            char campo[48];NRLOGF(campo,sizeof campo,"mask 0x%x",config_mask(lifecycle_config(g),config));
            fail_with(queue_diagnostic,id,nullptr,std::strcmp(id,"runtime_drift_config")?nullptr:campo);return false;
        }
        local_structure=field<float>(g,0x94AA4);skin_structure=field<float>(g,0x94AA8);return true;
    }
    bool ensure(const NrActiveSelectionV1& selection){
        if(controller)return true;nractive::Globals g{};if(!globals(g))return false;
        device=field<std::uint64_t>(g,0x93F38);queue=presentation_queue=field<std::uint64_t>(g,0x93F40);if(!device||!queue)return false;
        fg_queue_learning=host->fg_render_queue_learning();
        if(field<std::uint8_t>(g,0x94268)!=1||(field<std::uint8_t>(g,0x9426A)&1))return false;

        if(field<std::uint8_t>(g,0x94710)!=0){
            if(!nractive::first_use_empty(g)||!host->set_inline_mode(0)||!host->read(g)){fail("initial_async_install_failed");return false;}
        }
        if(field<std::uint8_t>(g,0x94710)!=0){fail("invalid_inline_mode");return false;}
        config=lifecycle_config(g);enabled=field<std::uint8_t>(g,0x94A8C)!=0;requested=selection.stage==NR_ACTIVE_STAGE_PRE?Stage::Pre:Stage::Post;
        fence=host->fence(1);if(!fence||fence->snapshot().status!=nrfence::Status::Ready){fail("fresh_owned_fence_failed");return false;}
        proof_device=fence->snapshot().identity.device;proof_queue=fence->snapshot().identity.queue;
        controller=std::make_unique<nrtransition::Controller>(nrtransition::Config{proof_device,proof_queue,requested,enabled,1,transition_deadline_ms},host->now());return true;
    }
    bool associate(void* command)noexcept{
        if(list){if(list!=command)fail("multiple_outstanding_recording_lists");return list==command&&!failed;}
        if(!command||epoch==UINT64_MAX||!host->retain(command)){fail("recording_epoch_or_retain_failed");return false;}
        if(!recording){fail("missing_recording_observer");return false;}const auto observed=recording->snapshot();
        if(observed.phase!=nrrecord::Phase::Open||!observed.neural||observed.generation!=controller->generation()){fail("recording_association_not_open_neural");return false;}
        list=command;observed_batch_epoch=observed.epoch;list_epoch=++epoch;event(Kind::RecordedList,list_epoch);

        wt::Record(wt::Event::CaptureRecorded,static_cast<std::uint64_t>(native_job),queue,
                   list_epoch,observed_batch_epoch,
                   static_cast<std::uint32_t>(observed.phase));
        return !failed;
    }
    bool mark_recording(void* command,std::uint64_t generation)noexcept{

        {void* em_uso[2+DispatchedSlots]{observed_command,list};for(unsigned i=0;i<DispatchedSlots;++i)em_uso[2+i]=dispatched[i].command;
         host->recordings_in_use(em_uso,2+DispatchedSlots);}
        auto* observer=host->recording(command,proof_device,host->thread());
        if(!observer){fail("recording_observer_install_or_identity_failed");return false;}
        if(recording&&(recording!=observer||observed_command!=command)){
            auto previous=recording->snapshot();

            if(previous.busy&&maintenance){
                const auto limite=nrjob::qpc()+nrjob::timeline().frequency()/500;
                while(previous.busy&&previous.phase!=nrrecord::Phase::Failed&&nrjob::qpc()<limite){std::this_thread::yield();previous=recording->snapshot();}
            }
            if(previous.phase==nrrecord::Phase::Failed){fail("command_list_switch_with_pending_recording");return false;}
            if(list||previous.busy||(previous.neural&&!previous.submitted)){

                if(maintenance){fail("command_list_switch_with_pending_recording");return false;}
                ++list_switch_deferrals;

                char detalhe[220];
                NRLOGF(detalhe,sizeof detalhe,"lista anterior busy=%u neural=%u submitted=%u lista_pendente=%u; captura adiada ha %llu ms",
                    previous.busy?1u:0u,previous.neural?1u:0u,previous.submitted?1u:0u,list?1u:0u,
                    static_cast<unsigned long long>(list_switch_retry.elapsed(host->now())));
                if(list_switch_retry.failure(host->now())==nrretry::Verdict::GiveUp){
                    fail_with("command_list_switch_with_pending_recording",nullptr,detalhe,"lista anterior presa 30 s");return false;}
                const bool stuck=list_switch_retry.elapsed(host->now())>=5000;
                diagnostic=stuck?"capture_deferred_pending_recording_stuck":"capture_deferred_pending_recording";

                if(list_switch_retry.attempts()==1)nrcompat::report("command_list_switch_with_pending_recording",detalhe);
                if(stuck&&!list_switch_stuck_reported){
                    list_switch_stuck_reported=true;
                    nrcompat::block(nrcompat::Origem::Binding,"command_list_switch_pending_stuck",detalhe,"lista anterior presa");
                }
                capture_deferred=true;return false;
            }
        }
        if(list_switch_stuck_reported&&!failed)nrcompat::unblock(nrcompat::Origem::Binding);
        list_switch_retry.success();list_switch_stuck_reported=false;
        recording=observer;observed_command=command;const auto current=recording->snapshot();
        if(current.phase==nrrecord::Phase::Failed||current.busy){fail(current.diagnostic);return false;}
        if(fg_queue_learning&&!render_queue_proven&&frame_generation_seen())dispatched_note(command,observer);
        if(!current.primed||current.phase==nrrecord::Phase::Unobserved){capture_deferred=true;diagnostic="waiting_reset_close";return false;}
        if(current.phase!=nrrecord::Phase::Open){fail("neural_recording_not_open");return false;}

        if(fg_queue_learning&&!render_queue_proven&&frame_generation_seen()&&!attempted&&!maintenance&&!baseline_valid&&!list&&!native_job){
            capture_deferred=true;diagnostic="waiting_render_queue_proof";
            if(++render_proof_deferrals>=600&&!render_proof_reported){
                render_proof_reported=true;
                char detalhe[200];
                NRLOGF(detalhe,sizeof detalhe,"FG ligado: nenhuma lista do upscale foi vista numa fila em %llu quadros; fila inicial %llx (NeuralFgFilaRender=0 desliga a espera)",
                    static_cast<unsigned long long>(render_proof_deferrals),static_cast<unsigned long long>(queue));
                nrcompat::report("waiting_render_queue_proof",detalhe);
            }
            return false;
        }
        if(!recording->mark_neural(generation)){fail("mark_neural_failed");return false;}return true;
    }
    void clear_batch()noexcept{
        if(list)host->release(list);list=nullptr;list_epoch=0;observed_batch_epoch=0;ticket=0;native_job=0;previous_published=0;submitted=wrapper_open=sealed=controller_signal=trace_fence_observed=false;batch_fence=0;expected_job.store(0);expected_worker.store(0);hip_status.store(NoHip);
    }
    static unsigned NR_ACTIVE_CALL describe(void* u,void* o,NrResourceDesc* d){return d&&static_cast<Impl*>(u)->host->describe(o,*d);}
    static unsigned NR_ACTIVE_CALL get_device(void* u,void* o,std::uint64_t* d){return d&&static_cast<Impl*>(u)->host->device_of(o,*d);}
    static unsigned NR_ACTIVE_CALL retain(void* u,void* o){return static_cast<Impl*>(u)->host->retain(o);}
    static void NR_ACTIVE_CALL release(void* u,void* o){static_cast<Impl*>(u)->host->release(o);}
    static unsigned NR_ACTIVE_CALL environment(void* u,NrActiveEnvironmentV1* e){
        auto& p=*static_cast<Impl*>(u);if(!e||p.failed||!p.thread_ok())return 0;nractive::Globals g{};if(!p.globals(g))return 0;
        *e={};e->size=sizeof(*e);e->version=1;e->first_use_empty=p.maintenance||p.host_reset_clean||(!p.attempted&&nractive::first_use_empty(g));
        e->engine_ready=field<std::uint8_t>(g,0x94268);e->error_clear=(field<std::uint8_t>(g,0x9426A)&1)==0;e->enabled=field<std::uint8_t>(g,0x94A8C);e->use_fsr_inputs=field<std::uint8_t>(g,0x94A8E);e->requested_inline=field<std::uint8_t>(g,0x94710);e->requested_interop=field<std::uint8_t>(g,0x948F4);
        e->use_depth=field<std::uint8_t>(g,0x94A8F);e->tone=field<unsigned>(g,0x94A90);e->pending_reset=p.maintenance?0:field<std::uint8_t>(g,0x94B78);e->active_inline=field<std::uint8_t>(g,0x94711);e->active_interop=field<std::uint8_t>(g,0x948F5);e->current_thread=p.host->thread();e->owner_thread=p.owner;e->selected_device=p.device;e->selected_queue=p.queue;
        if(e->use_depth!=1||field<std::uint8_t>(g,0x94BD0)!=1)return 0;e->reserved44=0xA51Cu;e->caches_match=1;
        if(p.scheduler.frozen&&!p.maintenance){const bool copied=field<int>(g,0x949DC)!=0;const unsigned color=p.scheduler.selection.stage==NR_ACTIVE_STAGE_PRE?0:1;e->caches_match=nractive::cache_matches(g,0x94298,&p.scheduler.descriptions[color],copied)&&nractive::cache_matches(g,0x94398,&p.scheduler.descriptions[2],copied)&&nractive::cache_matches(g,0x94498,&p.scheduler.descriptions[3],copied)&&nractive::cache_matches(g,0x94598,p.scheduler.resources[4]?&p.scheduler.descriptions[4]:nullptr,copied);}return 1;
    }
    static void NR_ACTIVE_CALL capture(void* u,const NrCaptureRecord* r){
        auto& p=*static_cast<Impl*>(u);p.reproject_authorized=false;p.capture_invoked=true;if(!r||p.failed||p.in_capture||p.wrapper_open||(p.list&&p.list!=r->command_list)){p.fail("capture_overlap");return;}
        if(!p.host->hip_imports_valid()){p.fail("HIP_import_provenance_failed");return;}
        if(p.candidate_pending){
            void* expected[5]{p.candidate.input,p.candidate.output,p.candidate.motion,p.candidate.depth,nullptr};
            if(p.scheduler.context!=p.candidate.context){p.fail("resolution_candidate_context_changed");return;}
            for(unsigned i=0;i<5;++i)if(p.scheduler.resources[i]!=expected[i]){p.fail("resolution_candidate_resource_changed");return;}
        }
        if(p.baseline_valid){
            if(p.scheduler.original!=p.baseline.original||p.scheduler.context!=p.baseline.context){p.fail("fixed_FFX_context_changed");return;}
            char campo[48],detalhe[400];
            if(!p.baseline_equivalent(campo,sizeof campo,detalhe,sizeof detalhe)){p.fail_with("fixed_FFX_resources_changed",nullptr,detalhe,campo);return;}
        }
        nractive::Globals before{};if(!p.globals(before))return;p.observe_async(before);if(p.failed)return;const int accepted=field<int>(before,0x949DC);
        if(accepted<0||accepted>=INT32_MAX-1||field<std::uint64_t>(before,0x949D0)||field<int>(before,0x949D8)!=-1){p.fail("pending_capture_or_native_wrap");return;}
        const bool resetting=p.maintenance;

        if(resetting)p.host->retire_reprojection();
        if(!p.mark_recording(r->command_list,p.controller->generation()+(resetting?1:0))){

            if(p.capture_deferred&&!p.failed&&p.list_switch_retry.pending())p.capture_invoked=false;
            return;
        }
        p.in_capture=true;p.reset_consumed=false;p.reset_returned=false;p.layout_used=false;

        {const bool vetar=!resetting&&p.controller->state()==State::Active&&
             nrredecada::vetar(p.host->rede_cada(),p.frame_count,p.rede_cada_quadro,p.rede_cada_tem);
         p.host->capture_sem_job(vetar);}
        try{p.host->capture(*r);}catch(...){p.reproject_authorized=false;p.in_capture=false;p.fail("original_A310_exception_command_state_unknown");throw;}p.reproject_authorized=false;p.in_capture=false;if(p.failed)return;
        nractive::Globals after{};if(!p.globals(after))return;p.observe_async(after);if(p.failed)return;
        if(resetting){
            if(!p.reset_consumed||!p.reset_returned||field<std::uint8_t>(after,0x94B78)){p.fail("missing_checked_reset_return");return;}p.host->clear_async_completion();p.last_completed_async=0;p.prev_completed_async=0;p.event(Kind::ResetFinished);if(p.failed)return;
            p.fence=p.host->fence_on_queue(p.controller->generation(),p.queue);p.fence_sequence=0;p.epoch=0;p.stopped=p.producer_closed=false;
            if(!p.fence||p.fence->snapshot().status!=nrfence::Status::Ready||p.fence->snapshot().identity.device!=p.proof_device||p.fence->snapshot().identity.queue!=p.proof_queue){p.fail("new_generation_fence_identity_failed");return;}
            if(p.retired_valid){nr_active_scheduler_destroy_v1(&p.retired_scheduler);p.retired_valid=false;
                for(auto* resource:p.rotating_resources)p.host->release(resource);p.rotating_resources.clear();}
        }
        p.maintenance=false;p.host_reset_clean=false;p.candidate_pending=false;p.attempted=true;if(!p.baseline_valid){p.baseline=p.scheduler;p.baseline_valid=true;}const int expected=resetting?1:accepted+1;const int actual=field<int>(after,0x949DC);if(!p.associate(r->command_list))return;
        if(actual==(resetting?0:accepted)){if(field<std::uint64_t>(after,0x949D0)||field<int>(after,0x949D8)!=-1)p.fail("jobless_capture_changed_pending_tuple");return;}
        if(actual!=expected||field<std::uint64_t>(after,0x949D0)!=reinterpret_cast<std::uint64_t>(r->command_list)||field<int>(after,0x949D8)!=actual||field<std::uint8_t>(after,0x94711)!=0||field<std::uint8_t>(after,0x948F5)!=1){p.fail("capture_publication_tuple_failed");return;}
        ++p.captured_jobs;p.native_job=actual;p.job_frame[unsigned(actual)&15u]=p.frame_count;p.rede_cada_quadro=p.frame_count;p.rede_cada_tem=true;nrjob::timeline().stamp(actual,nrjob::Capturado,nrjob::qpc());nrjob::ages().captured();p.previous_published=actual-1;p.expected_worker.store(field<unsigned>(after,0x949B0));p.expected_job.store(actual);wt::Record(wt::Event::CaptureRecorded,actual,p.queue,0,p.controller->generation());if(!p.expected_worker.load())p.fail("missing_worker_identity");p.diagnostic="capture_recorded_waiting_checked_submission";
    }
    void observe_async(const nractive::Globals& g)noexcept {
        if(!async_pending_job||failed)return;
        const int proof=host->async_completion(async_pending_job);
        if(proof!=NoHip&&proof!=0){fail("async_worker_completion_failed");return;}
        if(proof==0&&field<int>(g,0x949C8)==async_pending_job&&field<int>(g,0x949BC)==-1){
            event(Kind::HipCompleted,static_cast<std::uint64_t>(async_pending_job));
            if(failed)return;nrjob::timeline().stamp(async_pending_job,nrjob::Notado,nrjob::qpc());prev_completed_async=last_completed_async;last_completed_async=async_pending_job;async_pending_job=0;
            async_wait_retry.success();
        }else if(host->now()-async_started>=5000){
            if(async_wait_retry.failure(host->now())==nrretry::Verdict::GiveUp)fail("async_worker_completion_timeout");
            else{diagnostic="async_worker_completion_late_waiting";
                if(async_wait_retry.attempts()==1)nrcompat::report("async_worker_completion_late","job assincrono sem conclusao em 5 s; espera ate 30 s sem state=7 (tela de carga, alt-tab)");}
        }
    }
    bool install_mode() {
        const auto token=controller->reset_authorization();
        if(!restage_pending||!token||list||async_pending_job)return false;
        nractive::Globals before{},after{};if(!globals(before))return false;
        event(Kind::BeginReset,0,token->serial);if(failed)return false;
        if(!host->reset_staging()||!host->read(after)||!post_reset_matches(before,after)){fail("ffx_geometry_retired_reset_failed");return false;}

        host->clear_async_completion();
        config=lifecycle_config(after);last_completed_async=0;prev_completed_async=0;
        event(Kind::ResetFinished);if(failed)return false;
        nr_active_scheduler_destroy_v1(&scheduler);nr_active_scheduler_init_v1(&scheduler);
        for(auto* resource:rotating_resources)host->release(resource);rotating_resources.clear();
        baseline_valid=false;attempted=false;host_reset_clean=true;stopped=producer_closed=false;
        fence=host->fence_on_queue(controller->generation(),queue);fence_sequence=0;epoch=0;
        if(!fence||fence->snapshot().status!=nrfence::Status::Ready||fence->snapshot().identity.device!=proof_device||fence->snapshot().identity.queue!=proof_queue){fail("mode_new_fence_failed");return false;}
        abandono_armado=false;
        diagnostic="ffx_geometry_retired_waiting_neural_init";
        restage_pending=false;return true;
    }

    void fence_failure(const char* why,std::uint64_t alvo)noexcept{
        const long removido=fence->removed_reason();
        char detalhe[256],campo[48];
        NRLOGF(detalhe,sizeof detalhe,"motivo=\"%s\" removed_reason=0x%08lX valor_lido=%llu alvo=%llu",fence->failure(),static_cast<unsigned long>(removido),
            static_cast<unsigned long long>(fence->last_read()),static_cast<unsigned long long>(alvo));
        NRLOGF(campo,sizeof campo,"HRESULT 0x%08lX",static_cast<unsigned long>(removido));
        fail_with(why,nullptr,detalhe,campo);
    }

    const char* admission_reported{};
    static void roles(char* out,std::size_t n,const nrffx::Descriptor& d,const NrResourceDesc (&x)[5],const bool (&ok)[5])noexcept{
        std::size_t used=0;
        auto add=[&](const char* format,auto... values){if(used<n){const int w=NRLOGF(out+used,n-used,format,values...);if(w>0)used+=std::size_t(w);}};
        add("render=%ux%u saida=%ux%u mvScale=%.6g,%.6g",d.render_w,d.render_h,d.output_w,d.output_h,static_cast<double>(d.motion_x),static_cast<double>(d.motion_y));
        const char* nome[5]{"cor","saida","mv","prof","exp"};
        for(unsigned i=0;i<5;++i){
            if(i==4&&!d.exposure){add(" exp=nenhuma");continue;}
            if(!ok[i]){add(" %s=sem_descricao",nome[i]);continue;}
            add(" %s=fmt%u_%llux%u_dim%u_mips%u_amostras%u_flags0x%x",nome[i],x[i].format,static_cast<unsigned long long>(x[i].width),x[i].height,
                x[i].dimension,x[i].mip_levels,x[i].sample_count,x[i].flags);
        }
    }

    bool admission_nova(const char* id)noexcept{
        return !(admission_reported&&!std::strcmp(admission_reported,id)&&
                 (!std::strcmp(id,"ffx_reset_flag")||nrcompat::blocked(nrcompat::Origem::Admissao)));
    }
    void report_admission(void* descriptor)noexcept{
        if(const char* recusa=nrffx::refusal(descriptor,nullptr,0,nullptr,0)){
            if(!std::strcmp(recusa,"ffx_reset_flag"))nrreusostate::pedir_cheio();
            if(!admission_nova(recusa))return;
            admission_reported=recusa;
            char detalhe[900],campo[48];(void)nrffx::refusal(descriptor,detalhe,sizeof detalhe,campo,sizeof campo);

            if(!std::strcmp(recusa,"ffx_reset_flag"))nrcompat::report(recusa,detalhe);
            else nrcompat::block(nrcompat::Origem::Admissao,recusa,detalhe,campo);
            return;
        }
        nrffx::Descriptor d{};(void)nrffx::decode(descriptor,d);
        NrActiveEnvironmentV1 e{};const char* id=nullptr;const char* campo_env=nullptr;
        NrResourceDesc x[5]{};bool ok[5]{};bool mesmo=true,lista_ok=true;
        const bool env_lido=environment(this,&e)!=0;
        if(!env_lido){if(failed)return;id="environment_read";}
        else if((campo_env=nractive::environment_refusal(e))!=nullptr)id="environment_ok";
        else{
            void* o[5]{d.input,d.output,d.motion,d.depth,d.exposure};
            for(unsigned n=0;n<(d.exposure?5u:4u);++n){ok[n]=host->describe(o[n],x[n]);std::uint64_t dev{};if(!host->device_of(o[n],dev)||dev!=e.selected_device)mesmo=false;}
            {std::uint64_t dev{};if(!host->device_of(d.command,dev)||dev!=e.selected_device)lista_ok=false;}
            bool todos=true;for(unsigned n=0;n<(d.exposure?5u:4u);++n)todos&=ok[n];
            id=!todos?"describe_resource_layout":(!mesmo||!lista_ok)?"describe_device_and_direct_list":nractive::resource_refusal(d,x,nullptr,0);
        }
        if(!id)id="scheduler_fsr_only";
        if(!admission_nova(id))return;
        admission_reported=id;
        char detalhe[900]{},campo[48]{};
        if(!env_lido){
            nractive::Globals g{};const bool lido=host->read(g);
            NRLOGF(detalhe,sizeof detalhe,"UseDepth=%u profundidade_global=%u (as duas precisam ser 1)",lido?field<std::uint8_t>(g,0x94A8F):255u,lido?field<std::uint8_t>(g,0x94BD0):255u);
            NRLOGF(campo,sizeof campo,"UseDepth=%u",lido?field<std::uint8_t>(g,0x94A8F):255u);
        }else if(campo_env){
            NRLOGF(detalhe,sizeof detalhe,"campo=%s ready=%u error_clear=%u enabled=%u use_fsr_inputs=%u inline=%u interop=%u pending_reset=%u",campo_env,
                e.engine_ready,e.error_clear,e.enabled,e.use_fsr_inputs,e.requested_inline,e.requested_interop,e.pending_reset);
            NRLOGF(campo,sizeof campo,"%s",campo_env);
        }else if(!std::strcmp(id,"scheduler_fsr_only"))NRLOGF(detalhe,sizeof detalhe,"%s",queue_diagnostic);
        else{
            roles(detalhe,sizeof detalhe,d,x,ok);
            if(!std::strcmp(id,"describe_resource_layout"))NRLOGF(campo,sizeof campo,"%s",!ok[0]?"cor":!ok[1]?"saida":!ok[2]?"mv":!ok[3]?"prof":"exp");
            else if(!std::strcmp(id,"describe_device_and_direct_list"))NRLOGF(campo,sizeof campo,"%s",!mesmo?"recurso em outro dispositivo":"lista nao DIRECT ou outro dispositivo");
            else (void)nractive::resource_refusal(d,x,campo,sizeof campo);
        }
        nrcompat::block(nrcompat::Origem::Admissao,id,detalhe,campo);
    }
    void pump()noexcept{
        if(!valid||!thread_ok()||failed)return;if(foreign_failure.load()){fail("invalid_async_callback");return;}if(recording){const auto r=recording->snapshot();if(r.phase==nrrecord::Phase::Failed){fail(r.diagnostic);return;}if(r.busy)return;}if(!controller)return;
        nractive::Globals g{};if(!globals(g))return;if(field<std::uint8_t>(g,0x94268)!=1||(field<std::uint8_t>(g,0x9426A)&1)){fail("original_engine_error_or_readiness_lost");return;}enabled=field<std::uint8_t>(g,0x94A8C)!=0;controller->set_enabled(enabled);controller->tick(host->now());if(controller->state()==State::FailedFsrOnly){report_failure(controller->failure(),nullptr,nullptr,nullptr);failed=true;diagnostic=controller->failure();return;}
        observe_async(g);if(failed)return;
        const auto state=controller->state();
        if(state==State::Quiescing){
            if(!recording||!pode_aposentar())return;
            if(!stopped){if(!host->read(stopped_globals)||!host->stop_producers(controller->generation())){fail("stop_thread_identity_failed");return;}stopped=true;}
            const auto threads=host->producers(controller->generation());if(threads==Proof::Failed){fail("producer_termination_failed");return;}if(threads==Proof::Pending)return;
            if(!producer_closed){event(Kind::EnqueuersClosed);producer_closed=true;}if(controller->reset_authorization())return;
            const auto hip=host->hip_barrier(controller->generation());if(hip==Proof::Failed){fail("final_HIP_barrier_failed");return;}if(hip==Proof::Complete){event(Kind::GenerationHipCompleted);diagnostic="closed_generation_reset_authorized";}return;
        }
        if(state!=State::Active&&state!=State::Initializing&&state!=State::Closing&&state!=State::WaitingForSubmission&&state!=State::WaitingForCompletion)return;
        if(wrapper_open||(list&&!submitted))return;
        const bool transition=state==State::Closing||state==State::WaitingForSubmission||state==State::WaitingForCompletion;if(!list&&!transition)return;
        if((transition||state==State::Initializing)&&!sealed){event(Kind::SubmissionsClosed);sealed=true;if(failed)return;}
        if(!batch_fence){if(fence_sequence==UINT64_MAX-1){fail("fence_counter_wrap");return;}batch_fence=++fence_sequence;wt::Record(wt::Event::FenceSignal,native_job,queue,batch_fence);if(fence->signal(batch_fence).status!=nrfence::Status::Pending){fence_failure("owned_fence_signal_failed",batch_fence);return;}}
        if((transition||state==State::Initializing)&&!controller_signal){event(Kind::FenceSignaled,batch_fence);controller_signal=true;if(failed)return;}
        const auto proof=fence->poll();if(proof.status==nrfence::Status::Failed){fence_failure("owned_fence_poll_failed",proof.target);return;}if(proof.status!=nrfence::Status::Completed)return;
        if(!trace_fence_observed){wt::Record(wt::Event::FenceComplete,native_job,queue,batch_fence);trace_fence_observed=true;}
        if(state!=State::Active&&async_pending_job)return;
        if(state==State::Active){event(Kind::RetireGraphicsCompleted,batch_fence);if(!failed)clear_batch();return;}
        if(transition&&(!recording||!pode_aposentar()))return;
        event(Kind::FenceCompleted,batch_fence);if(failed)return;
        if(state==State::Initializing){
            if(native_job){NrActiveEnvironmentV1 env{};if(!environment(this,&env)||!env.caches_match||env.active_inline!=0||env.active_interop!=1){NRLOGF(queue_diagnostic,sizeof(queue_diagnostic),"initialized_staging_cache_%u_inline_%u_interop_%u_color_%ux%u_mv_%ux%u_depth_%ux%u_mvmeta_%u_%u_%u_%u_%u_%u",env.caches_match,env.active_inline,env.active_interop,field<unsigned>(g,0x942C8),field<unsigned>(g,0x942CC),field<unsigned>(g,0x943C8),field<unsigned>(g,0x943CC),field<unsigned>(g,0x944C8),field<unsigned>(g,0x944CC),field<unsigned>(g,0x943E0),field<unsigned>(g,0x943E4),field<unsigned>(g,0x943E8),field<unsigned>(g,0x943EC),field<unsigned>(g,0x943F0),field<unsigned>(g,0x943F4));fail(queue_diagnostic);return;}event(Kind::InitChecked);if(!failed)diagnostic="checked_generation_active";}else event(Kind::RetireCompleted,batch_fence);
            if(!failed)clear_batch();
        }else{event(Kind::BeginQuiesce);if(!failed)clear_batch();}
    }
};
PinnedBinding::PinnedBinding(void* base):PinnedBinding(make_windows_host(base)){(void)publish_callbacks();}
PinnedBinding::PinnedBinding(std::unique_ptr<Host> h):impl_(std::make_shared<Impl>(std::move(h))){}
PinnedBinding::PinnedBinding(std::shared_ptr<Impl> retained):impl_(std::move(retained)){}
bool PinnedBinding::publish_callbacks()noexcept{
    auto& p=*impl_;std::lock_guard<std::recursive_mutex> lock(p.state_mutex);if(!p.valid||p.controller||!p.thread_ok())return false;
    try{auto* endpoint=new PinnedBinding(impl_);PinnedBinding* none=nullptr;
        if(!published.compare_exchange_strong(none,endpoint)){delete endpoint;p.fail("callback_endpoint_already_owned");return false;}
        p.production=true;
        if(!p.host->publish(&callbacks)){p.fail("immutable_callback_publication_failed");return false;}
        return true;
    }catch(...){p.fail("callback_endpoint_allocation_failed");return false;}
}
PinnedBinding::~PinnedBinding()=default;
int PinnedBinding::dispatch(NrFfxDispatch original,void* context,void* descriptor,int nesting,const NrActiveSelectionV1& selection,unsigned* handled,NrActiveDispatchResultV1* result){
    auto& p=*impl_;std::lock_guard<std::recursive_mutex> lock(p.state_mutex);Impl::MirrorOnExit mirror_on_exit{p};if(handled)*handled=1;if(result){*result={};result->size=sizeof(*result);result->version=1;}static std::uint64_t despachos_ffx=0;++despachos_ffx;nrreproject::base_recurso.store(nullptr);

    if(nrrj::ativo()&&((despachos_ffx&63u)==1u||nrrj::cam_fov.load(std::memory_order_relaxed)==0.f)){nrffx::Descriptor dc{};if(nrffx::decode(descriptor,dc)){nrrj::cam_near.store(dc.camera_near);nrrj::cam_far.store(dc.camera_far);nrrj::cam_fov.store(dc.camera_fov);}}

    auto despejar=[&](unsigned modo){
        const unsigned faltam=nrreproject::despejo_final.load();

        const bool preparo=!faltam&&nrreproject::final_quer_preparo();
        if(faltam||preparo){
            nrffx::Descriptor d{};
            const bool decodificado=nrffx::decode(descriptor,d)&&d.command&&d.input&&d.output;
            if(decodificado){auto& x=nrreproject::final_extras;x.motion=d.motion;x.depth=d.depth;x.exposure=d.exposure;
                x.motion_state=d.motion_state;x.depth_state=d.depth_state;x.exposure_state=d.exposure_state;
                x.jitter_x=d.jitter_x;x.jitter_y=d.jitter_y;x.mv_x=d.motion_x;x.mv_y=d.motion_y;
                x.render_w=d.render_w;x.render_h=d.render_h;x.output_w=d.output_w;x.output_h=d.output_h;x.valido=true;
                x.camera_near=d.camera_near;x.camera_far=d.camera_far;x.camera_fov=d.camera_fov;

                if(nrrj::ativo()){const auto& q=nrrj::quadro_render();x.rj_on=q.on;x.rj_nivel=q.nivel;x.rj_ganho=q.ganho;x.rj_cx=q.cx;x.rj_cy=q.cy;x.rj_lado=q.lado;
                    x.rj_rx=q.recorte_x;x.rj_ry=q.recorte_y;x.rj_rl=q.recorte_l;x.rj_composto=q.composto;x.rj_job=q.job;x.rj_idade=q.idade;}else x.rj_on=-1;
                for(int kn=0;kn<nrrj::npc_slots&&kn<7;++kn){nrrj::Inst i1(1+kn);x.rjn_on[kn]=-1;
                 if(nrrj::ativo()){const auto& q=nrrj::quadro_render();x.rjn_on[kn]=q.on;x.rjn_nivel[kn]=q.nivel;x.rjn_ganho[kn]=q.ganho;x.rjn_cx[kn]=q.cx;x.rjn_cy[kn]=q.cy;x.rjn_lado[kn]=q.lado;
                    x.rjn_rx[kn]=q.recorte_x;x.rjn_ry[kn]=q.recorte_y;x.rjn_rl[kn]=q.recorte_l;x.rjn_composto[kn]=q.composto;x.rjn_job[kn]=q.job;}}
                }
            if(faltam){
                if(decodificado&&
                   p.host->dump_final(d.command,d.input,d.input_state,d.output,d.output_state,modo,despachos_ffx))
                    nrreproject::despejo_final.store(faltam-1);
            }else if(decodificado)p.host->dump_final_prepare(d.command,d.input,d.output,modo);
            nrreproject::final_extras.valido=false;
        }
        p.host->dump_final_tick();
    };
    auto forward=[&](){p.host->invalidate_reprojection();const auto r=original?original(context,descriptor):6;if(result)result->original_result=r;despejar(2u);return r;};
    if(!p.valid||!p.thread_ok())return forward();if(p.in_dispatch||p.in_capture){p.fail("recursive_dispatch_or_capture_overlap");return forward();}if(p.wrapper_open)return forward();p.pump();if(p.failed)return forward();
    if(selection.size!=sizeof(selection)||selection.version!=1||!supported_percent(selection.scale_percent)||(selection.stage!=NR_ACTIVE_STAGE_PRE&&selection.stage!=NR_ACTIVE_STAGE_POST)){p.fail("unsupported_stage_scale_selection");return forward();}
    if(!p.ensure(selection))return forward();p.menu_generation=p.resolution_pending?p.resolution_ticket.request_id:selection.generation;p.requested=(p.resolution_pending?p.resolution_ticket.stage:selection.stage)==NR_ACTIVE_STAGE_PRE?Stage::Pre:Stage::Post;p.controller->request(p.requested,p.host->now());p.pump();if(p.failed)return forward();

    const auto wait_started=p.host->now();
    const auto wait_job=p.native_job;bool traced_wait=false;
    if(!(p.list&&p.submitted))p.frame_wait_retry.success();

    const std::uint64_t wait_budget=p.frame_wait_retry.pending()?0:5000;
    unsigned wait_turns=0;
    while(p.enabled&&p.list&&p.submitted&&!p.wrapper_open&&!p.failed&&
          (p.controller->state()==State::Active||p.controller->state()==State::Initializing)){
        if(!traced_wait){wt::Record(wt::Event::CpuWaitBegin,wait_job,p.queue,p.batch_fence);traced_wait=true;p.host->frame_wait_begin(p.fence.get());}
        ++wait_turns;
        if(p.host->now()-wait_started>=wait_budget){
            if(p.frame_wait_retry.failure(p.host->now())==nrretry::Verdict::GiveUp)p.fail("submitted_frame_completion_timeout");
            else{p.diagnostic="submitted_frame_completion_late_fsr_only";
                if(p.frame_wait_retry.attempts()==1)nrcompat::report("submitted_frame_completion_late","lote do quadro sem completar em 5 s; quadros seguintes so FSR sem bloquear, ate 30 s sem state=7");}
            break;
        }
        if(!p.host->wait_batch(p.fence.get(),p.async_pending_job>0))break;
        p.pump();
    }
    if(traced_wait)wt::Record(wt::Event::CpuWaitEnd,wait_job,p.queue,p.batch_fence,p.host->now()-wait_started,p.failed?1u:0u);
    p.host->frame_wait_end(wait_turns,p.expected_worker.load());
    if(p.failed)return forward();
    if(p.resolution_pending&&!p.resolution_installed)return forward();
    if(p.restage_pending&&p.controller->reset_authorization()){(void)p.install_mode();if(p.failed)return forward();}
    const bool reset=p.controller->reset_authorization().has_value();
    if(reset)nrreusostate::pedir_cheio();
    if(reset&&p.enabled){if(p.retired_valid||p.host->producers(p.controller->generation())!=Proof::Complete||!p.host->set_pending_reset()){p.fail("maintenance_readiness_failed");return forward();}p.retired_scheduler=p.scheduler;p.retired_valid=true;nr_active_scheduler_init_v1(&p.scheduler);p.attempted=false;p.maintenance=true;}
    if(p.enabled)p.passo_teto_post();

    if(p.enabled&&nroverlayamd::vigiar_entrada.load(std::memory_order_relaxed)){nrffx::Descriptor e{};if(nrffx::decode(descriptor,e))nroverlayamd::nota_entrada(e.input,e.render_w,e.render_h);}
    if(p.enabled){++p.enabled_dispatches;++p.frame_count;if(p.list){if(p.submitted)++p.pending_submitted;else ++p.pending_unsubmitted;}
        auto& idades=nrjob::ages();idades.set_period(p.async_tuning().age_log_frames);idades.frame();if(idades.due()){idades.late(p.host->late_totals());p.host->age_log(idades.take(p.async_tuning()));}}
    if(!p.enabled||p.list||(!reset&&!p.controller->neural_allowed()&&!p.controller->initialization_capture_allowed()))return forward();
    if(!reset&&p.geometry_changed(original,context,descriptor)){
        const auto now=p.host->now();if(!p.restage_count||now-p.restage_window>=60000){p.restage_window=now;p.restage_count=0;}

        if(++p.restage_count>30){p.fail("ffx_geometry_restage_limit");return forward();}
        p.restage_pending=true;p.controller->request_mode_change(now);p.diagnostic="ffx_geometry_changed_waiting_retirement";p.pump();return forward();
    }

    if(!reset&&!p.restage_pending&&!p.resolution_pending&&
       p.controller->state()==State::Active&&p.async_pending_job>0&&p.last_completed_async>0){
        const unsigned k=unsigned(nrreproject::value(nrreproject::controls,nrreproject::MaxAge)+.5f);
        if(k>0&&p.frame_count-p.job_frame[unsigned(p.last_completed_async)&15u]>k){
            const auto inicio=p.host->now();++p.age_waits;
            for(unsigned volta=0;p.async_pending_job>0&&!p.failed&&volta<100000u;++volta){
                if(p.host->now()-inicio>=100){++p.age_wait_timeouts;break;}

                if(!p.host->yield_gpu_progress())break;
                p.pump();
            }
            p.age_wait_ms+=p.host->now()-inicio;
            if(p.failed)return forward();
        }
    }

    if(!reset&&!p.restage_pending&&!p.resolution_pending&&
       p.controller->state()==State::Active&&p.async_pending_job>0&&p.last_completed_async>0){
        const auto& ajuste=p.async_tuning();auto& esperas=nrjob::ages().waits();
        if(ajuste.exclusive_ms>0){

            const int job=p.async_pending_job;
            if(p.window_job!=job){p.window_job=job;p.window_left_us=static_cast<long long>(ajuste.exclusive_ms)*1000;}
            if(p.window_left_us>0&&p.host->async_remaining_us(job)>=0){
                const long long fatia=(std::min)(p.window_left_us,(static_cast<long long>(ajuste.exclusive_ms)*1000+2)/3);
                bool adiantou=false;const double ms=p.wait_for_job(unsigned(fatia),adiantou);

                if(ms>=0){p.window_left_us-=adiantou?static_cast<long long>(ms*1000.0):(std::max)(static_cast<long long>(ms*1000.0),fatia);
                    ++esperas.window_waits;esperas.window_ms+=ms;if(adiantou)++esperas.window_early;}
                if(p.failed)return forward();
            }
        }
        if(ajuste.short_wait_ms>0&&p.async_pending_job>0){

            const int job=p.async_pending_job;const long long falta=p.host->async_remaining_us(job);
            if(falta>=0&&falta<=static_cast<long long>(ajuste.short_wait_ms)*1000){
                bool adiantou=false;const double ms=p.wait_for_job(ajuste.short_wait_ms*1000u,adiantou);
                if(ms>=0){++esperas.short_waits;esperas.short_ms+=ms;if(adiantou)++esperas.short_early;else ++esperas.short_timeouts;}
                if(p.failed)return forward();
            }
        }
    }
    {
        nrffx::Descriptor dj{};const bool ok=nrffx::decode(descriptor,dj)&&std::isfinite(dj.jitter_x)&&std::isfinite(dj.jitter_y)&&
            std::fabs(dj.jitter_x)<=4.f&&std::fabs(dj.jitter_y)<=4.f;
        nrreproject::jitter_atual_x.store(ok?dj.jitter_x:0.f);nrreproject::jitter_atual_y.store(ok?dj.jitter_y:0.f);nrreproject::jitter_valido.store(ok);
        nrreproject::near_atual.store(ok&&std::isfinite(dj.camera_near)&&dj.camera_near>0.f?dj.camera_near:0.f);
    }

    if(p.enabled)nrexpotrava::tick(p.host->base(),nullptr);
    p.in_dispatch=true;struct Exit{bool& v;~Exit(){v=false;}}exit{p.in_dispatch};
    NrActiveDispatchV1 input{};input.size=sizeof(input);input.version=1;input.image_base=p.host->base();input.original=original;input.context=context;input.descriptor=descriptor;input.nesting=nesting;input.selection=selection;input.selection.master_enabled=1;input.selection.generation=p.controller->generation()+(reset?1:0);input.selection.stage=(reset?p.requested:p.controller->initializing_stage())==Stage::Pre?NR_ACTIVE_STAGE_PRE:NR_ACTIVE_STAGE_POST;
    input.hooks={sizeof(NrActiveHooksV1),1,&p,Impl::capture,Impl::describe,Impl::environment,Impl::get_device,Impl::retain,Impl::release};
    p.capture_deferred=false;
    p.rebind_equivalent(original,context,descriptor);if(p.failed)return forward();
    p.capture_invoked=false;
    void* custo_lista=nullptr;
    if(nrcusto::marca_fn().load(std::memory_order_relaxed)){nrffx::Descriptor dc{};if(nrffx::decode(descriptor,dc)&&dc.command){custo_lista=dc.command;nrcusto::marca(custo_lista,nrcusto::DispInicio);}}
    const auto value=nr_active_schedule_dispatch_v1(&p.scheduler,&input,handled,result);
    if(!p.capture_invoked)p.host->invalidate_reprojection();

    if(!p.failed&&p.enabled&&p.capture_invoked&&
       nrreproject::value(nrreproject::controls,nrreproject::ComposeAfter)>.5f){
        nrffx::Descriptor d{};
        if(nrffx::decode(descriptor,d)&&d.command&&d.output){
            if(p.host->reproject_post(d.command,d.output,d.output_state)>1)p.fail("post_fsr_reprojection_failed");
        }
    }
    if(custo_lista)nrcusto::marca(custo_lista,nrcusto::DispFim);

    if(!p.failed&&p.enabled)despejar(0u);
    if(result&&!result->capture_attempted&&!p.failed&&p.enabled&&!p.scheduler.frozen){
        nractive::Globals observed{};
        if(p.host->read(observed)){
            NrResourceDesc rd[4]{};nrffx::Descriptor descriptor_view{};
            const bool decoded=nrffx::decode(descriptor,descriptor_view);
            void* objects[4]{descriptor_view.input,descriptor_view.output,descriptor_view.motion,descriptor_view.depth};
            unsigned described=0;for(unsigned i=0;i<4&&decoded;++i)if(p.host->describe(objects[i],rd[i]))++described;
            NRLOGF(p.queue_diagnostic,sizeof(p.queue_diagnostic),
              "scheduler_fsr_only_empty_%u_ready_%u_error_%u_fsr_%u_inline_%u_interop_%u_pending_%u_depth_%u_activeinline_%u_activeinterop_%u_described_%u_formats_%u_%u_%u_%u_flags_%u_%u",
              nractive::first_use_empty(observed)?1u:0u,field<std::uint8_t>(observed,0x94268),
              (field<std::uint8_t>(observed,0x9426A)&1)?0u:1u,field<std::uint8_t>(observed,0x94A8E),
              field<std::uint8_t>(observed,0x94710),field<std::uint8_t>(observed,0x948F4),
              field<std::uint8_t>(observed,0x94B78),field<std::uint8_t>(observed,0x94A8F),
              field<std::uint8_t>(observed,0x94711),field<std::uint8_t>(observed,0x948F5),described,
              rd[0].format,rd[1].format,rd[2].format,rd[3].format,rd[0].flags,rd[1].flags);
            p.diagnostic=p.queue_diagnostic;
        }
        p.report_admission(descriptor);
    }

    if(result&&result->capture_attempted&&!p.failed){nrcompat::unblock(nrcompat::Origem::Admissao);p.admission_reported=nullptr;}
    if(p.capture_deferred&&!p.failed&&!p.attempted&&!p.maintenance&&!p.baseline_valid&&!p.list&&!p.native_job){

        nr_active_scheduler_destroy_v1(&p.scheduler);nr_active_scheduler_init_v1(&p.scheduler);
        if(result){*result={};result->size=sizeof(*result);result->version=1;result->outcome=NR_ACTIVE_FSR_ONLY;result->original_result=value;}return value;
    }
    if(p.scheduler.poisoned&&!p.failed){
        nrffx::Descriptor d{};NrActiveEnvironmentV1 e{};unsigned pointers=0,descriptions=0,caches=0;
        const bool decoded=nrffx::decode(descriptor,d);
        void* objects[5]{d.input,d.output,d.motion,d.depth,d.exposure};
        for(unsigned n=0;n<5;++n){
            if(objects[n]!=p.scheduler.resources[n])pointers|=1u<<n;
            NrResourceDesc rd{};if(objects[n]&&(!p.host->describe(objects[n],rd)||std::memcmp(&rd,&p.scheduler.descriptions[n],sizeof(rd))))descriptions|=1u<<n;
        }
        Impl::environment(&p,&e);nractive::Globals g{};
        if(p.host->read(g))for(unsigned n=0;n<4;++n){const unsigned index=n==0?(p.scheduler.selection.stage==NR_ACTIVE_STAGE_PRE?0:1):n+1;
            if(nractive::cache_matches(g,0x94298+n*0x100,n==3?(p.scheduler.resources[4]?&p.scheduler.descriptions[4]:nullptr):&p.scheduler.descriptions[index],true))caches|=1u<<n;}
        NRLOGF(p.queue_diagnostic,sizeof(p.queue_diagnostic),"scheduler_drift_decoded_%u_pointers_%u_desc_%u_caches_%u_context_%u_env_%u_%u_%u_%u_%u",decoded?1u:0u,pointers,descriptions,caches,context==p.scheduler.context?1u:0u,e.engine_ready,e.error_clear,e.pending_reset,e.active_inline,e.active_interop);
        NrResourceDesc mv{};if(d.motion)p.host->describe(d.motion,mv);
        const auto& old=p.scheduler.descriptions[2];const auto length=std::strlen(p.queue_diagnostic);
        NRLOGF(p.queue_diagnostic+length,sizeof(p.queue_diagnostic)-length,
            "_mv_old_%llux%u_fmt%u_flags%u_mips%u_samples%u_new_%llux%u_fmt%u_flags%u_mips%u_samples%u_scale_%.3f_%.3f",
            static_cast<unsigned long long>(old.width),old.height,old.format,old.flags,old.mip_levels,old.sample_count,
            static_cast<unsigned long long>(mv.width),mv.height,mv.format,mv.flags,mv.mip_levels,mv.sample_count,d.motion_x,d.motion_y);

        const bool transitorio=decoded&&!pointers&&!descriptions&&caches==15u&&context==p.scheduler.context&&
            e.engine_ready==1&&e.error_clear==1&&e.active_interop==1;
        if(p.restage_pending)return value;
        if(transitorio&&!p.maintenance&&p.controller&&p.controller->state()==State::Active){
            const auto now=p.host->now();if(!p.restage_count||now-p.restage_window>=60000){p.restage_window=now;p.restage_count=0;}
            if(++p.restage_count<=30){
                char detalhe[640];NRLOGF(detalhe,sizeof detalhe,"motivo=%s %s",nractive::ultimo_veneno(),p.queue_diagnostic);
                nrcompat::report("scheduler_drift_transitorio_religa",detalhe);
                p.restage_pending=true;p.controller->request_mode_change(now);p.diagnostic="scheduler_drift_transitorio_waiting_retirement";
                p.pump();return value;
            }
        }
        {const auto n=std::strlen(p.queue_diagnostic);NRLOGF(p.queue_diagnostic+n,sizeof(p.queue_diagnostic)-n,"_motivo_%s",nractive::ultimo_veneno());}
        p.fail(p.queue_diagnostic);
    }return value;
}
unsigned PinnedBinding::compose(void* command,void* destination,unsigned,int job,float,int,unsigned inline_mode)noexcept{
    auto& p=*impl_;std::lock_guard<std::recursive_mutex> lock(p.state_mutex);++p.compose_skipped;if(!p.valid||!p.thread_ok()||p.failed||!p.controller||!p.controller->neural_allowed()||inline_mode!=0||p.wrapper_open||p.submitted||p.sealed){
        if(p.valid&&!p.failed)nrrecusa::registrar(inline_mode!=0?"compose_proxy_sincrono":p.wrapper_open||p.submitted||p.sealed?"compose_lista_submetida":"compose_estado",job,p.last_completed_async,0,0);
        return 1;}
    if(p.list&&p.list!=command){nrrecusa::registrar("compose_outra_lista",job,p.last_completed_async,0,0);return 1;}
    nractive::Globals g{};std::uint64_t actual_device{};
    const unsigned color=p.scheduler.selection.stage==NR_ACTIVE_STAGE_PRE?0:1;
    if(!p.globals(g)||!field<std::uint8_t>(g,0x94A8C))return 1;
    p.observe_async(g);if(p.failed)return 1;

    int usar=job;bool segura=false;
    if(job<=0||job!=p.last_completed_async){
        usar=nrrecusa::segurar(job,p.last_completed_async,p.prev_completed_async,p.segura_seguidos,nrrecusa::segura.load(std::memory_order_relaxed),p.maintenance);
        nrrecusa::registrar(nrrecusa::motivo_job(job,p.last_completed_async,p.prev_completed_async),job,p.last_completed_async,p.prev_completed_async,p.async_pending_job,usar);
        if(!usar)return 1;segura=true;
    }
    if(job<0||destination!=p.scheduler.resources[color]||!p.host->device_of(command,actual_device)||actual_device!=p.device){p.fail("composition_resource_or_job_drift");return 1;}
    if(!p.mark_recording(command,p.controller->generation())){nrrecusa::registrar("compose_gravacao_recusada",job,0,0,0);return 1;}
    if(p.associate(command)){if(p.in_capture){p.reproject_authorized=true;p.reproject_command=command;p.reproject_destination=destination;p.reproject_job=usar;p.reproject_job_nativo=job;p.reproject_segura=segura;}
        if(segura){++p.segura_seguidos;nrrecusa::segurados.fetch_add(1);}else p.segura_seguidos=0;
        ++p.compose_accepted;--p.compose_skipped;return 0;}
    nrrecusa::registrar("compose_lista_nao_associada",job,0,0,0);return 1;
}

void PinnedBinding::tick()noexcept{tick(nullptr,nullptr);}
void PinnedBinding::tick(void(*also)(void*),void* context)noexcept{
    std::unique_lock<std::recursive_mutex> lock(impl_->state_mutex,std::try_to_lock);if(!lock.owns_lock())return;
    {Impl::MirrorOnExit mirror_on_exit{*impl_};impl_->pump();}
    if(also)also(context);
}
unsigned PinnedBinding::reproject(void* command,void* destination,unsigned state,int job,float exposure,unsigned controls)noexcept{
    auto& p=*impl_;std::lock_guard<std::recursive_mutex> lock(p.state_mutex);
    const bool authorized=p.reproject_authorized;p.reproject_authorized=false;
    const bool segura=authorized&&p.reproject_segura;p.reproject_segura=false;

    const int nativo=segura?p.reproject_job_nativo:p.reproject_job;
    if(!authorized||!p.valid||!p.thread_ok()||!p.enabled||p.failed||!p.in_capture||!p.controller||!p.controller->neural_allowed()||
       p.wrapper_open||p.submitted||p.sealed||p.reproject_command!=command||p.reproject_destination!=destination||nativo!=job||
       p.list!=command||job<=0||(!segura&&job!=p.last_completed_async)){

        if(authorized)nrrecusa::registrar(p.failed||!p.valid||!p.enabled||!p.controller||!p.controller->neural_allowed()?"reproject_estado":
            p.wrapper_open||p.submitted||p.sealed?"reproject_lista_submetida":p.reproject_command!=command||p.list!=command||p.reproject_destination!=destination?"reproject_lista_ou_destino":
            nrrecusa::motivo_job(job,p.last_completed_async,p.prev_completed_async),job,p.last_completed_async,p.prev_completed_async,p.reproject_job);
        return 1;}
    if(segura)job=p.reproject_job;

    p.host->late_candidate(!segura&&p.async_pending_job>0&&p.async_pending_job!=job?p.async_pending_job:0);
    if(segura)p.host->reproject_segurado(nativo);
    const auto result=p.host->reproject(command,destination,state,job,exposure,controls);
    if(result>1)p.fail("async_reprojection_recording_failed");
    if(result==0){

        const auto now=nrjob::qpc();const bool first=job!=p.first_used_job;
        if(first){p.first_used_job=job;nrjob::timeline().stamp(job,nrjob::PrimeiroUso,now);}
        const auto age=p.frame_count-p.job_frame[unsigned(job)&15u];
        nrjob::ages().composed(job,age>1000u?1000u:unsigned(age),now,first);
    }
    return result;
}
namespace {
bool valid_ticket(const nrhost::Ticket& t){
    return t.size==sizeof(t)&&t.version==1&&supported_percent(t.percent)&&t.request_id&&
        (t.stage==1||t.stage==2)&&t.final_width&&t.final_height&&t.final_width<=16384&&t.final_height<=16384&&
        t.working_width==std::max<std::uint64_t>(1,(std::uint64_t(t.final_width)*t.percent+50)/100)&&
        t.working_height==std::max<std::uint64_t>(1,(std::uint64_t(t.final_height)*t.percent+50)/100)&&
        t.scene_width&&t.scene_height&&t.scene_width<=t.working_width&&t.scene_height<=t.working_height;
}
bool ticket_request_equal(nrhost::Ticket a,nrhost::Ticket b){a.old_generation=b.old_generation=0;a.serial=b.serial=0;return std::memcmp(&a,&b,sizeof(a))==0;}
}
nrhost::Status PinnedBinding::prepare_resolution(nrhost::Ticket& t)noexcept{
    auto& p=*impl_;std::lock_guard<std::recursive_mutex> lock(p.state_mutex);if(!p.valid||!p.thread_ok()||p.failed||!valid_ticket(t))return nrhost::Status::Failed;
    p.pump();if(p.failed)return nrhost::Status::Failed;if(!p.controller){NrActiveSelectionV1 selection{sizeof(selection),1,1,t.stage,t.percent,t.request_id};if(!p.ensure(selection))return p.failed?nrhost::Status::Failed:nrhost::Status::Pending;}
    if(p.resolution_pending&&!ticket_request_equal(t,p.resolution_ticket))return nrhost::Status::Pending;
    if(!p.resolution_pending){
        if(t.request_id<=p.completed_request)return nrhost::Status::Failed;
        if(p.controller->state()!=State::Active&&!(p.controller->state()==State::Initializing&&!p.attempted&&!p.list))return nrhost::Status::Pending;
        p.resolution_ticket=t;p.resolution_pending=true;p.resolution_installed=false;
        p.controller->request_rebuild(t.stage==1?Stage::Pre:Stage::Post,t.request_id,p.host->now());
    }
    p.pump();if(p.failed)return nrhost::Status::Failed;
    if(!p.attempted&&!p.list&&p.controller->state()==State::Initializing){
        p.resolution_ticket.old_generation=p.controller->generation();p.resolution_ticket.serial=0;
        t=p.resolution_ticket;return nrhost::Status::Ready;
    }
    const auto token=p.controller->reset_authorization();if(!token)return nrhost::Status::Pending;
    p.resolution_ticket.old_generation=token->generation;p.resolution_ticket.serial=token->serial;t=p.resolution_ticket;return nrhost::Status::Ready;
}
nrhost::Status PinnedBinding::install_resolution(const nrhost::Ticket& t,const nrhost::Resources& r)noexcept{
    auto& p=*impl_;std::lock_guard<std::recursive_mutex> lock(p.state_mutex);if(!p.valid||!p.thread_ok()||p.failed||!p.resolution_pending||p.resolution_installed||
        std::memcmp(&t,&p.resolution_ticket,sizeof(t))||r.size!=sizeof(r)||r.version!=1||!r.context)return nrhost::Status::Failed;
    void* objects[4]{r.input,r.output,r.motion,r.depth};
    for(unsigned i=0;i<4;++i){NrResourceDesc d{};std::uint64_t device{};
        if(!objects[i]||!p.host->describe(objects[i],d)||!p.host->device_of(objects[i],device)||device!=p.device||
           d.width!=(i==1?t.working_width:t.scene_width)||d.height!=(i==1?t.working_height:t.scene_height)||d.dimension!=3||d.sample_count!=1||d.depth_or_array_size!=1||d.mip_levels!=1)return nrhost::Status::Failed;

        if(p.baseline_valid){const unsigned index=i==0?0:i==1?1:i==2?2:3;const auto& old=p.baseline.descriptions[index];if(d.format!=old.format||d.flags!=old.flags)return nrhost::Status::Failed;}
    }
    p.pump();if(p.failed)return nrhost::Status::Failed;
    if(p.attempted){
        const auto token=p.controller->reset_authorization();
        if(!token||token->serial!=t.serial||token->generation!=t.old_generation||p.list||p.host->producers(token->generation)!=Proof::Complete)return nrhost::Status::Pending;
        nractive::Globals before{},after{};if(!p.globals(before))return nrhost::Status::Failed;
        p.event(Kind::BeginReset,0,token->serial);if(p.failed)return nrhost::Status::Failed;
        if(!p.host->reset_staging()||!p.globals(after)||!post_reset_matches(before,after)){p.fail("host_resolution_reset_failed");return nrhost::Status::Failed;}
        p.host->clear_async_completion();p.last_completed_async=0;p.prev_completed_async=0;p.event(Kind::ResetFinished);if(p.failed)return nrhost::Status::Failed;
        nr_active_scheduler_destroy_v1(&p.scheduler);nr_active_scheduler_init_v1(&p.scheduler);
        p.fence=p.host->fence_on_queue(p.controller->generation(),p.queue);p.fence_sequence=0;p.epoch=0;p.stopped=p.producer_closed=false;
        if(!p.fence||p.fence->snapshot().status!=nrfence::Status::Ready||p.fence->snapshot().identity.device!=p.proof_device||p.fence->snapshot().identity.queue!=p.proof_queue){p.fail("resolution_fence_identity_failed");return nrhost::Status::Failed;}
    }
    p.baseline_valid=false;p.attempted=false;p.host_reset_clean=true;p.candidate=r;p.candidate_pending=true;
    p.resolution_installed=true;p.menu_generation=t.request_id;p.working_percent=t.percent;
    p.diagnostic="host_resolution_installed_waiting_neural_init";
    return p.enabled?nrhost::Status::Ready:nrhost::Status::DisabledReady;
}
nrhost::Status PinnedBinding::poll_resolution(std::uint64_t request)noexcept{
    auto& p=*impl_;std::lock_guard<std::recursive_mutex> lock(p.state_mutex);if(!p.valid||!p.thread_ok()||p.failed)return nrhost::Status::Failed;p.pump();if(p.failed)return nrhost::Status::Failed;
    if(request!=p.resolution_ticket.request_id||!p.resolution_installed)return nrhost::Status::Pending;
    const bool active=p.controller&&p.controller->state()==State::Active;
    const bool clean_off=!p.enabled&&!p.attempted&&p.host_reset_clean;
    if(!active&&!clean_off)return nrhost::Status::Pending;
    p.completed_request=request;p.resolution_pending=false;
    return active?nrhost::Status::Ready:nrhost::Status::DisabledReady;
}
nrhost::Status PinnedBinding::cancel_resolution(std::uint64_t request)noexcept{
    auto& p=*impl_;std::lock_guard<std::recursive_mutex> lock(p.state_mutex);if(!p.valid||!p.thread_ok()||p.failed||request!=p.resolution_ticket.request_id||p.resolution_installed)return nrhost::Status::Failed;

    return nrhost::Status::Pending;
}
MenuSnapshot PinnedBinding::snapshot()const noexcept{
    auto& p=*impl_;std::lock_guard<std::recursive_mutex> lock(p.state_mutex);MenuSnapshot s{};s.diagnostic=p.diagnostic;s.enabled=p.enabled;s.requested=p.requested;if(p.valid&&!p.failed)s.state=State::Initializing;if(p.controller){s.state=p.controller->state();s.generation=p.controller->generation();const auto e=p.controller->effective();if(e)s.effective_stage=*e==Stage::Pre?1u:2u;}if(p.failed||p.foreign_failure.load())s.state=State::FailedFsrOnly;
    s.async_pending_job=p.async_pending_job;s.async_completed_job=p.last_completed_async;
    s.local_structure=p.local_structure;s.skin_structure=p.skin_structure;
    s.working_percent=p.working_percent;
    s.enabled_dispatches=p.enabled_dispatches;s.captured_jobs=p.captured_jobs;s.pending_unsubmitted=p.pending_unsubmitted;s.pending_submitted=p.pending_submitted;s.compose_accepted=p.compose_accepted;s.compose_skipped=p.compose_skipped;
    if(p.recording){const auto r=p.recording->snapshot();s.observed_epoch=r.epoch;s.observed_resets=r.resets;s.observed_closes=r.closes;s.recording_primed=r.primed;if(r.phase==nrrecord::Phase::Failed)s.state=State::FailedFsrOnly;}
    if(p.scheduler.frozen){s.render_width=static_cast<unsigned>(p.scheduler.descriptions[0].width);s.render_height=p.scheduler.descriptions[0].height;s.output_width=static_cast<unsigned>(p.scheduler.descriptions[1].width);s.output_height=p.scheduler.descriptions[1].height;}return s;
}
const char* PinnedBinding::status()const noexcept{return impl_->diagnostic;}
void PinnedBinding::report_error(const char* why)noexcept{auto& p=*impl_;std::lock_guard<std::recursive_mutex> lock(p.state_mutex);p.fail(why);}
std::optional<std::uint64_t> PinnedBinding::acknowledged_generation()const noexcept{auto& p=*impl_;std::lock_guard<std::recursive_mutex> lock(p.state_mutex);if(p.recording){const auto r=p.recording->snapshot();if(r.phase==nrrecord::Phase::Failed||r.busy)return {};}if(!p.failed&&!p.foreign_failure.load()&&p.controller&&p.controller->state()==State::Active&&p.controller->effective()==p.requested)return p.menu_generation;return {};}
std::uint64_t PinnedBinding::queue_begin(void* queue,unsigned count,void** lists)noexcept{
    auto& p=*impl_;std::unique_lock<std::recursive_mutex> lock(p.state_mutex,std::try_to_lock);
    if(!lock.owns_lock()){if(p.fast_foreign(queue,count,lists,p.host->thread()))return 0;lock.lock();}
    Impl::MirrorOnExit mirror_on_exit{p};if(!p.valid||p.failed||!p.controller)return 0;

    if(p.render_learning()&&!p.in_capture&&!p.in_dispatch&&!p.scheduler.frozen&&count<=64&&lists){
        const auto selected=reinterpret_cast<std::uint64_t>(queue);
        for(unsigned i=0;i<count;++i){
            const auto* d=p.dispatched_find(lists[i]);if(!d)continue;
            if(selected==p.queue){p.render_proved();break;}
            const auto observed=d->observer->snapshot();
            if(p.wrapper_open||observed.phase!=nrrecord::Phase::Closed||observed.neural||observed.busy)break;
            try{(void)p.adopt_render_queue(selected);}catch(...){p.fail("fsr_queue_adoption_failed");}
            return 0;
        }
    }

    if(p.in_capture&&p.in_dispatch&&!p.wrapper_open&&(reinterpret_cast<std::uint64_t>(queue)==p.presentation_queue||reinterpret_cast<std::uint64_t>(queue)==p.queue)&&count==1&&lists&&lists[0]&&lists[0]!=p.observed_command&&lists[0]!=p.list){
        nractive::Globals g{};if(!p.globals(g))return 0;
        if(field<std::uint64_t>(g,0x93F40)==reinterpret_cast<std::uint64_t>(queue)&&field<std::uint64_t>(g,0x94A70)==reinterpret_cast<std::uint64_t>(lists[0])&&
           !field<std::uint64_t>(g,0x949D0)&&field<int>(g,0x949D8)==-1&&p.ticket_sequence!=UINT64_MAX){
            p.queue_before=g;p.wrapper_count=1;p.wrapper_lists[0]=lists[0];
            p.relevant_wrapper=false;p.internal_wrapper=true;p.wrapper_epoch=0;
            p.wrapper_queue=reinterpret_cast<std::uint64_t>(queue);p.ticket=++p.ticket_sequence;p.wrapper_open=true;p.wrapper_thread=p.host->thread();wt::Record(wt::Event::QueueBegin,0,p.wrapper_queue,p.ticket,count,2);return p.ticket;
        }
    }

    if(p.host->thread()!=p.owner&&!p.in_capture&&!p.in_dispatch&&count<=64&&(lists||!count)){
        bool tracked=false;
        for(unsigned i=0;i<count;++i)tracked|=lists[i]&&(lists[i]==p.observed_command||lists[i]==p.list);
        if(!tracked)return 0;
    }

    if(!p.in_capture&&!p.in_dispatch&&reinterpret_cast<std::uint64_t>(queue)!=p.queue){

        if(count>64||(!lists&&count)){p.fail("foreign_queue_invalid_lists");return 0;}
        for(unsigned i=0;i<count;++i)if(lists[i]&&(lists[i]==p.observed_command||lists[i]==p.list)){
            const auto observed=p.recording?p.recording->snapshot():nrrecord::Snapshot{};
            if(!p.attempted&&!p.list&&!p.baseline_valid&&!p.scheduler.frozen&&p.recording&&
               !observed.neural&&!observed.busy&&observed.phase==nrrecord::Phase::Unobserved&&p.queue==p.presentation_queue)return 0;
            if(!p.attempted&&!p.list&&!p.baseline_valid&&!p.scheduler.frozen&&p.recording&&
               !observed.neural&&!observed.busy&&observed.phase==nrrecord::Phase::Closed&&p.queue==p.presentation_queue){

                if(p.wrapper_open)return 0;

                try{(void)p.adopt_render_queue(reinterpret_cast<std::uint64_t>(queue));}catch(...){p.fail("fsr_queue_adoption_failed");}
                return 0;
            }
            p.fail("foreign_queue_tracked_recording");return 0;
        }
        return 0;
    }

    if(p.wrapper_open&&!p.in_capture&&!p.in_dispatch&&reinterpret_cast<std::uint64_t>(queue)==p.queue&&
       p.host->thread()!=p.wrapper_thread&&count<=64&&(lists||!count)){
        bool tracked=false;
        for(unsigned i=0;i<count;++i)tracked|=lists[i]&&(lists[i]==p.observed_command||lists[i]==p.list);
        if(!tracked)return 0;
    }
    if(p.in_capture||p.in_dispatch||p.wrapper_open||reinterpret_cast<std::uint64_t>(queue)!=p.queue||count>64||(!lists&&count)){NRLOGF(p.queue_diagnostic,sizeof(p.queue_diagnostic),"queue_overlap_capture_%u_dispatch_%u_open_%u_queue_%llx_expected_%llx_count_%u_lists_%llx",p.in_capture?1u:0u,p.in_dispatch?1u:0u,p.wrapper_open?1u:0u,static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(queue)),static_cast<unsigned long long>(p.queue),count,static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(lists)));p.fail(p.queue_diagnostic);return 0;}

    if(p.host->thread()==p.owner)p.pump();if(p.failed)return 0;
    unsigned object_hits=0;for(unsigned i=0;i<count;++i)if(lists[i]==p.observed_command&&p.observed_command)++object_hits;
    if(object_hits&&!p.render_queue_proven)p.render_proved();
    nrrecord::Snapshot observed{};if(p.recording)observed=p.recording->snapshot();
    if(object_hits){
        if(object_hits!=1||observed.phase==nrrecord::Phase::Failed||observed.busy||(observed.phase!=nrrecord::Phase::Closed&&observed.phase!=nrrecord::Phase::Unobserved)){p.fail("queue_recording_not_closed_or_unique");return 0;}
        if(observed.neural&&(observed.submitted||!p.list||observed.epoch!=p.observed_batch_epoch||observed.generation!=p.controller->generation())){p.fail("stale_neural_recording_replay_retained");return 0;}
        if(observed.neural&&!p.recording->mark_submitted(observed.epoch)){p.fail("recording_submission_failed");return 0;}

        if(observed.neural)wt::Record(wt::Event::QueueEnd,static_cast<std::uint64_t>(p.native_job),p.queue,
                                      observed.epoch,p.observed_batch_epoch,1u);
    }
    p.wrapper_epoch=object_hits?observed.epoch:0;
    unsigned hits=(object_hits&&p.list&&observed.neural&&observed.epoch==p.observed_batch_epoch)?1u:0u;if(hits>1||(hits&&p.submitted)||p.ticket_sequence==UINT64_MAX){
        const auto proof=p.fence->snapshot();
        NRLOGF(p.queue_diagnostic,sizeof(p.queue_diagnostic),"duplicate_queue_recording_hits_%u_submitted_%u_ticket_%llu_epoch_%llu_fence_%u_target_%llu_hip_%d_state_%u",hits,p.submitted?1u:0u,static_cast<unsigned long long>(p.ticket_sequence),static_cast<unsigned long long>(p.list_epoch),static_cast<unsigned>(proof.status),static_cast<unsigned long long>(proof.target),p.hip_status.load(),static_cast<unsigned>(p.controller->state()));
        p.fail(p.queue_diagnostic);return 0;
    }
    nractive::Globals g{};if(!p.globals(g))return 0;
    if(hits&&p.native_job){if(field<std::uint64_t>(g,0x949D0)!=reinterpret_cast<std::uint64_t>(p.list)||field<int>(g,0x949D8)!=p.native_job||field<int>(g,0x949DC)!=p.native_job){p.fail("queue_preassociation_failed");return 0;}}
    else if(hits&&(field<std::uint64_t>(g,0x949D0)||field<int>(g,0x949D8)!=-1)){p.fail("jobless_queue_unexpected_pending");return 0;}
    p.internal_wrapper=false;p.queue_before=g;p.wrapper_count=count;for(unsigned i=0;i<count;++i)p.wrapper_lists[i]=lists[i];p.relevant_wrapper=hits==1;
    p.wrapper_queue=reinterpret_cast<std::uint64_t>(queue);p.ticket=++p.ticket_sequence;p.wrapper_open=true;p.wrapper_thread=p.host->thread();if(p.relevant_wrapper){p.event(Kind::SubmissionBegin,p.ticket);p.event(Kind::SubmitList,p.list_epoch,p.ticket);}wt::Record(wt::Event::QueueBegin,p.native_job,p.wrapper_queue,p.ticket,count,p.relevant_wrapper?1u:0u);return p.failed?0:p.ticket;
}
void PinnedBinding::queue_end(std::uint64_t ticket,void* queue,unsigned count,void** lists)noexcept{
    if(!ticket)return;
    auto& p=*impl_;std::lock_guard<std::recursive_mutex> lock(p.state_mutex);Impl::MirrorOnExit mirror_on_exit{p};if(!p.valid||p.failed)return;if(!p.wrapper_open||ticket!=p.ticket||reinterpret_cast<std::uint64_t>(queue)!=p.wrapper_queue||count>64||(!lists&&count)){p.fail("queue_end_mismatch");return;}
    if(count!=p.wrapper_count){p.fail("queue_end_count_mismatch");return;}for(unsigned i=0;i<count;++i)if(lists[i]!=p.wrapper_lists[i]){p.fail("queue_end_list_mismatch");return;}nractive::Globals g{};if(!p.globals(g))return;
    if(p.internal_wrapper&&(!p.in_capture||!p.in_dispatch||count!=1||field<std::uint64_t>(g,0x94A70)!=reinterpret_cast<std::uint64_t>(lists[0]))){p.fail("internal_queue_identity_changed");return;}
    if(p.wrapper_epoch){const auto current=p.recording->snapshot();if(current.epoch!=p.wrapper_epoch||current.phase!=nrrecord::Phase::Closed||current.busy){p.fail("recording_changed_during_queue_wrapper");return;}}
    if(!p.relevant_wrapper){
        if(field<std::uint64_t>(g,0x949D0)!=field<std::uint64_t>(p.queue_before,0x949D0)||field<int>(g,0x949D8)!=field<int>(p.queue_before,0x949D8)||field<int>(g,0x949DC)!=field<int>(p.queue_before,0x949DC)||field<int>(g,0x94740)!=field<int>(p.queue_before,0x94740)){p.fail("unrelated_wrapper_changed_publication");return;}
        wt::Record(wt::Event::QueueEnd,0,p.wrapper_queue,p.ticket,count,p.internal_wrapper?2u:0u);p.wrapper_open=false;p.internal_wrapper=false;p.ticket=0;return;
    }
    if(field<std::uint64_t>(g,0x949D0)||field<int>(g,0x949D8)!=-1||field<std::uint8_t>(g,0x94711)!=0){p.fail("queue_posttuple_failed");return;}
    if(p.native_job){
        const bool job_publication=field<int>(g,0x949BC)==p.native_job||(field<int>(g,0x949BC)==-1&&field<int>(g,0x949C8)==p.native_job);
        if(!job_publication||field<int>(g,0x949DC)!=p.native_job||p.async_pending_job){p.fail("queue_postpublication_failed");return;}
        const auto previous_fence=field<std::uint64_t>(p.queue_before,0x949A0);
        if(previous_fence==UINT64_MAX||!field<std::uint64_t>(g,0x94990)||
           field<std::uint64_t>(g,0x94990)!=field<std::uint64_t>(p.queue_before,0x94990)||
           field<std::uint64_t>(g,0x949C0)!=previous_fence+1||
           field<std::uint64_t>(g,0x949C0)!=field<std::uint64_t>(g,0x949A0)){p.fail("async_capture_fence_publication_failed");return;}
        p.async_pending_job=p.native_job;p.async_started=p.host->now();nrjob::timeline().stamp(p.native_job,nrjob::Submetido,nrjob::qpc());p.host->captura_submetida(p.native_job,queue);
        p.event(Kind::JobPublished,static_cast<std::uint64_t>(p.native_job),p.ticket);
    }
    else if(field<int>(g,0x94740)!=field<int>(p.queue_before,0x94740)||field<int>(g,0x949DC)!=field<int>(p.queue_before,0x949DC)){p.fail("jobless_wrapper_changed_publication");return;}
    wt::Record(wt::Event::QueueEnd,p.native_job,p.wrapper_queue,p.ticket,count,1);p.event(Kind::SubmissionEnd,p.ticket);p.wrapper_open=false;p.submitted=true;
}
void PinnedBinding::hip_complete(int status,int job)noexcept{auto& p=*impl_;if(job<=0||job!=p.expected_job.load()||p.host->thread()!=p.expected_worker.load()){wt::Record(wt::Event::HipComplete,job,0,0,static_cast<unsigned>(status),1);p.foreign_failure.store(true);return;}int none=NoHip;const bool accepted=status!=NoHip&&p.hip_status.compare_exchange_strong(none,status);wt::Record(wt::Event::HipComplete,job,0,0,static_cast<unsigned>(status),accepted?0u:1u);if(!accepted)p.foreign_failure.store(true);}
unsigned PinnedBinding::reset_gate(unsigned kind)noexcept{
    auto& p=*impl_;std::lock_guard<std::recursive_mutex> lock(p.state_mutex);if(!p.valid||!p.thread_ok()||p.failed||!p.in_capture||!p.controller)return 0;nractive::Globals g{};if(!p.globals(g))return 0;
    if(kind==0){const auto token=p.controller->reset_authorization();if(!p.maintenance||!token||p.reset_consumed||!p.recording||!p.recording->can_retire(p.controller->generation())){p.fail("pending_reset_without_single_use_proof");return 0;}
        for(auto r:{0x949A8u,0x948E0u})if(field<std::uint64_t>(g,r)!=field<std::uint64_t>(p.stopped_globals,r)){p.fail("stopped_handle_changed");return 0;}
        for(auto r:{0x949B0u,0x948E8u,0x94BA0u})if(field<unsigned>(g,r)!=field<unsigned>(p.stopped_globals,r)){p.fail("stopped_identity_changed");return 0;}
        p.reset_before=g;p.event(Kind::BeginReset,0,token->serial);p.reset_consumed=!p.failed;return p.failed?0u:1u;
    }
    if(kind==1){if(p.layout_used){p.fail("duplicate_layout_reset");return 0;}if(!(p.maintenance&&p.reset_consumed&&p.reset_returned)&&!p.host_reset_clean&&!(!p.attempted&&nractive::first_use_empty(g))){p.fail("layout_reset_without_retired_or_initial_proof");return 0;}p.layout_used=true;p.reset_before=g;return 1;}
    if(kind==2||kind==3){if((kind==2&&!p.reset_consumed)||(kind==3&&!p.layout_used)||!post_reset_matches(p.reset_before,g)){p.fail("post_CD60_retirement_predicate_failed");return 0;}p.reset_returned=true;return 1;}
    p.fail("unknown_reset_callback");return 0;
}
}
