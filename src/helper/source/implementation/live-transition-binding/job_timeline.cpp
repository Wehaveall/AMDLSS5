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

    if(NR_LOG_ATIVO&&job_bytes_<job_bytes_limit){
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
        if(NR_LOG_ATIVO&&job_bytes_<job_bytes_limit){
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

    (void)tuning;std::string out;late_prev_=late_now_;
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
