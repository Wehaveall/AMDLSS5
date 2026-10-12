#include "../../../runtime/nr_publico.h"
#include "job_timeline.h"
#include "../../../runtime/post_teto.h"
#include <windows.h>
#include <algorithm>
#include <cstdio>
namespace nrjob {
const char* name(Stamp s) noexcept {
    switch(s){
    case Capturado: return "capt";
    case Submetido: return "subm";
    case EvalInicio: return "eval_ini";
    case EvalFim: return "eval_fim";
    case Publicado: return "publ";
    case Notado: return "notado";
    case PrimeiroUso: return "uso";
    case GpuAntesBegin: return "gpu_antes_begin";
    case GpuAntesNativa: return "gpu_antes_nativa";
    case GpuFimCopia: return "gpu_fim_copia";
    case GpuDepoisNativa: return "gpu_depois_nativa";
    case CercaPronta: return "cerca";
    default: return "?";
    }
}
std::int64_t qpc() noexcept {LARGE_INTEGER v{};QueryPerformanceCounter(&v);return v.QuadPart;}
std::int64_t qpc_frequency() noexcept {LARGE_INTEGER v{};QueryPerformanceFrequency(&v);return v.QuadPart;}

void Timeline::stamp(int job, Stamp s, std::int64_t t) noexcept {
    if(job<=0||s>=StampCount)return;
    auto& slot=slots_[unsigned(job)&15u];
    if(s==Capturado){

        slot.job.store(0,std::memory_order_release);
        for(auto& v:slot.t)v.store(0,std::memory_order_relaxed);
        slot.t[Capturado].store(t,std::memory_order_relaxed);
        slot.job.store(job,std::memory_order_release);
        return;
    }
    if(slot.job.load(std::memory_order_acquire)!=job)return;

    std::int64_t none=0;
    if(!slot.t[s].compare_exchange_strong(none,t,std::memory_order_acq_rel))return;
    if(s==EvalFim){
        const auto begin=slot.t[EvalInicio].load(std::memory_order_acquire);
        if(begin>0&&t>begin&&frequency_>0){
            const std::int64_t us=(t-begin)*1000000/frequency_;
            const auto old=eval_ema_us_.load(std::memory_order_relaxed);
            eval_ema_us_.store(old?(old*3+us)/4:us,std::memory_order_relaxed);
        }
    }
}
std::int64_t Timeline::when(int job, Stamp s) const noexcept {
    if(job<=0||s>=StampCount)return 0;
    const auto& slot=slots_[unsigned(job)&15u];
    if(slot.job.load(std::memory_order_acquire)!=job)return 0;
    return slot.t[s].load(std::memory_order_acquire);
}

void Ages::composed(int job, unsigned age_frames, std::int64_t now, bool first) {
    ++composed_;
    hist_[age_frames>max_age?max_age:age_frames]++;
    age_sum_+=age_frames;
    const auto captured=timeline_.when(job,Capturado);
    if(captured>0&&now>=captured)ages_ms_.push_back(timeline_.ms(now-captured));
    if(!first)return;
    ++first_uses_;first_age_sum_+=age_frames;
    std::int64_t t[StampCount]{};
    for(unsigned s=0;s<StampCount;++s)t[s]=timeline_.when(job,Stamp(s));
    if(!t[PrimeiroUso])t[PrimeiroUso]=now;
    static constexpr Stamp from[6]={Capturado,Submetido,EvalInicio,EvalFim,Publicado,Notado};
    static constexpr Stamp to[6]={Submetido,EvalInicio,EvalFim,Publicado,Notado,PrimeiroUso};
    for(unsigned i=0;i<6;++i)if(t[from[i]]>0&&t[to[i]]>=t[from[i]]){phase_sum_[i]+=timeline_.ms(t[to[i]]-t[from[i]]);++phase_n_[i];}
    if(t[EvalInicio]>0&&t[EvalFim]>t[EvalInicio])job_ms_.push_back(timeline_.ms(t[EvalFim]-t[EvalInicio]));
    if(job_bytes_<job_bytes_limit){
        char line[512];
        const int n=NRLOGF(line,sizeof(line),"JOB job=%d capt=%lld subm=%lld eval_ini=%lld eval_fim=%lld publ=%lld notado=%lld uso=%lld idade_uso=%u eval_ms=%.3f idade_uso_ms=%.3f\n",
            job,static_cast<long long>(t[Capturado]),static_cast<long long>(t[Submetido]),static_cast<long long>(t[EvalInicio]),
            static_cast<long long>(t[EvalFim]),static_cast<long long>(t[Publicado]),static_cast<long long>(t[Notado]),static_cast<long long>(t[PrimeiroUso]),
            age_frames,(t[EvalInicio]>0&&t[EvalFim]>t[EvalInicio])?timeline_.ms(t[EvalFim]-t[EvalInicio]):-1.0,
            t[Capturado]>0?timeline_.ms(t[PrimeiroUso]-t[Capturado]):-1.0);
        if(n>0){job_lines_.append(line,std::size_t(std::min(n,int(sizeof(line)-1))));job_bytes_+=unsigned(n);}
    }

    if(t[CercaPronta]>0){
        static constexpr Stamp de[7]={Submetido,GpuAntesBegin,GpuAntesNativa,GpuFimCopia,GpuDepoisNativa,CercaPronta,Submetido};
        static constexpr Stamp ate[7]={GpuAntesBegin,GpuAntesNativa,GpuFimCopia,GpuDepoisNativa,CercaPronta,EvalInicio,EvalInicio};
        double v[7];
        for(unsigned i=0;i<7;++i){
            const bool ok=t[de[i]]>0&&t[ate[i]]>0;
            v[i]=ok?timeline_.ms(t[ate[i]]-t[de[i]]):-1.0;
            if(ok){capt_sum_[i]+=v[i];++capt_n_[i];}
        }
        ++capt_jobs_;
        if(job_bytes_<job_bytes_limit){
            char line[320];
            const int n=NRLOGF(line,sizeof(line),"CAPT job=%d subm_antes=%.3f begin=%.3f copia=%.3f compoe=%.3f resto=%.3f acordar=%.3f subm_eval=%.3f\n",
                job,v[0],v[1],v[2],v[3],v[4],v[5],v[6]);
            if(n>0){job_lines_.append(line,std::size_t(std::min(n,int(sizeof(line)-1))));job_bytes_+=unsigned(n);}
        }
    }
}
namespace {
double percentile(std::vector<double>& v,double q){
    if(v.empty())return -1;std::sort(v.begin(),v.end());
    std::size_t i=std::size_t(q*double(v.size())+.999999);if(i)--i;if(i>=v.size())i=v.size()-1;return v[i];
}
}
std::string Ages::take(const Tuning& tuning) {
    const unsigned long pid=GetCurrentProcessId();
    char head[1400];
    std::string hist;
    unsigned p50=0,p90=0;{unsigned run=0;bool h50=false,h90=false;
        for(unsigned a=0;a<=max_age;++a){if(!hist_[a])continue;char b[24];NRLOGF(b,sizeof b,"%s%u:%u",hist.empty()?"":",",a,hist_[a]);hist+=b;
            run+=hist_[a];if(!h50&&run*2>=composed_){p50=a;h50=true;}if(!h90&&run*10>=composed_*9){p90=a;h90=true;}}}
    double mean_ms=0;for(double v:ages_ms_)mean_ms+=v;mean_ms=ages_ms_.empty()?-1:mean_ms/double(ages_ms_.size());
    const double p90_ms=percentile(ages_ms_,.9);
    const double job_med=percentile(job_ms_,.5),job_p90=percentile(job_ms_,.9);
    double phase[6];for(unsigned i=0;i<6;++i)phase[i]=phase_n_[i]?phase_sum_[i]/phase_n_[i]:-1;

    static unsigned long long segurados_antes=0;
    const unsigned long long segurados_agora=nrposteto::segurados_total.load(std::memory_order_relaxed),segurados=segurados_agora-segurados_antes;
    segurados_antes=segurados_agora;
    NRLOGF(head,sizeof head,
        "IDADE pid=%lu quadros=%u compostos=%u segurados=%llu jobs=%u P=%.3f hist=%s media=%.3f p50=%u p90=%u media_ms=%.3f p90_ms=%.3f L=%.3f "
        "job_ms_med=%.3f job_ms_p90=%.3f jobs_medidos=%zu fases_ms=capt_subm:%.3f,subm_eval:%.3f,eval:%.3f,eval_publ:%.3f,publ_notado:%.3f,notado_uso:%.3f "
        "espera_curta=%u adiantados=%u estouros=%u espera_curta_ms=%.3f janela=%u janela_antes=%u janela_ms=%.3f "
        "chave_espera_curta_ms=%u chave_janela_ms=%u ema_rede_ms=%.3f qpc=%lld\n",
        pid,frames_,composed_,segurados,captured_,captured_?double(frames_)/captured_:-1.0,hist.c_str(),composed_?age_sum_/composed_:-1.0,p50,p90,mean_ms,p90_ms,
        first_uses_?first_age_sum_/first_uses_:-1.0,job_med,job_p90,job_ms_.size(),phase[0],phase[1],phase[2],phase[3],phase[4],phase[5],
        waits_.short_waits,waits_.short_early,waits_.short_timeouts,waits_.short_ms,waits_.window_waits,waits_.window_early,waits_.window_ms,
        tuning.short_wait_ms,tuning.exclusive_ms,double(timeline_.eval_ema_us())/1000.0,static_cast<long long>(qpc()));
    std::string out(head);
    {
        const auto& a=late_now_;const auto& b=late_prev_;
        if(a.armed!=b.armed||a.recorded!=b.recorded||a.resolved!=b.resolved||a.desligada!=b.desligada){
            const double dg=a.gain_frames-b.gain_frames,dm=a.gain_ms-b.gain_ms;
            char late[512];
            NRLOGF(late,sizeof late,
                "TARDIA pid=%lu armados=%llu tardia_gravadas=%llu tardia_resolvidas=%llu tardia_executou_J=%llu tardia_recusas=%llu tardia_falhas_B=%llu tardia_inconsistentes=%llu "
                "ganho_quadros=%.3f ganho_ms=%.3f media_efetiva=%.3f media_ms_efetiva=%.3f tardia_desligada=%u\n",
                pid,static_cast<unsigned long long>(a.armed-b.armed),static_cast<unsigned long long>(a.recorded-b.recorded),static_cast<unsigned long long>(a.resolved-b.resolved),
                static_cast<unsigned long long>(a.executed-b.executed),static_cast<unsigned long long>(a.refused-b.refused),static_cast<unsigned long long>(a.b_failed-b.b_failed),
                static_cast<unsigned long long>(a.inconsistent-b.inconsistent),composed_?dg/composed_:0.0,composed_?dm/composed_:0.0,
                composed_?(age_sum_-dg)/composed_:-1.0,(mean_ms>=0&&composed_)?mean_ms-dm/composed_:-1.0,a.desligada?1u:0u);
            out+=late;
        }
        late_prev_=late_now_;
    }
    if(capt_jobs_){
        double m[7];for(unsigned i=0;i<7;++i)m[i]=capt_n_[i]?capt_sum_[i]/capt_n_[i]:-1.0;
        char capt[400];
        NRLOGF(capt,sizeof capt,"CAPTURA pid=%lu n=%u subm_antes=%.3f begin=%.3f copia=%.3f compoe=%.3f resto=%.3f acordar=%.3f subm_eval=%.3f\n",
            pid,capt_jobs_,m[0],m[1],m[2],m[3],m[4],m[5],m[6]);
        out+=capt;
    }
    out+=job_lines_;
    frames_=captured_=composed_=first_uses_=0;for(auto& h:hist_)h=0;age_sum_=first_age_sum_=0;ages_ms_.clear();job_ms_.clear();
    for(unsigned i=0;i<6;++i){phase_sum_[i]=0;phase_n_[i]=0;}for(unsigned i=0;i<7;++i){capt_sum_[i]=0;capt_n_[i]=0;}capt_jobs_=0;waits_={};job_lines_.clear();
    return out;
}
Timeline& timeline() noexcept {static Timeline* t=new Timeline(qpc_frequency());return *t;}
Ages& ages() {static Ages* a=new Ages(timeline());return *a;}
void* completion_event() noexcept {
    static HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    return event;
}
void signal_completion() noexcept {if(auto* e=completion_event())SetEvent(e);}
}
