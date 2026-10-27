#include "recording_table.h"
#include "compute_state.h"
#include <algorithm>
#include <windows.h>
#include <unknwn.h>
namespace nrrecord {
namespace {

bool morta(void* command) noexcept {
    auto* u=static_cast<IUnknown*>(command);
    u->AddRef();const ULONG n=u->Release();
    const ULONG nossas=1u+(nrstate::tracked(command)?1u:0u);
    return n==nossas;
}
}
void RecordingTable::in_use(void* const* commands,unsigned count) noexcept {
    std::lock_guard<std::mutex> trava(mutex_);
    in_use_n_=0;
    for(unsigned i=0;i<count&&in_use_n_<in_use_.size();++i)if(commands&&commands[i])in_use_[in_use_n_++]=commands[i];
}
unsigned RecordingTable::evict_dead() noexcept {
    unsigned n=0;
    try{
        for(auto it=entries_.begin();it!=entries_.end();){
            void* const c=it->first;bool usada=false;
            for(unsigned i=0;i<in_use_n_;++i)usada=usada||in_use_[i]==c;

            if(usada||it->second.observer->snapshot().busy||!morta(c)||!forget(c)){++it;continue;}
            (void)nrstate::forget(c);
            graveyard_.push_back(std::move(it->second.observer));it=entries_.erase(it);++n;
        }
    }catch(...){}
    evicted_+=n;return n;
}

unsigned RecordingTable::evict_lru(bool so_sem_fsr) noexcept {
    unsigned n=0;
    try{
        std::vector<std::pair<std::uint64_t,void*>> velhas;velhas.reserve(entries_.size());
        constexpr std::uint64_t ja_levou=std::uint64_t{1}<<63;

        for(const auto& [c,item]:entries_){bool usada=false;for(unsigned i=0;i<in_use_n_;++i)usada=usada||in_use_[i]==c;
            if(!usada&&!(so_sem_fsr&&item.fsr))velhas.emplace_back((std::max)(item.uso,item.observer->uso_reset())|(item.fsr?ja_levou:0),c);}
        std::sort(velhas.begin(),velhas.end());
        const std::size_t lote=std::max<std::size_t>(1,cap/16);
        for(const auto& [uso,c]:velhas){
            if(n>=lote)break;
            auto it=entries_.find(c);if(it==entries_.end())continue;
            const auto s=it->second.observer->snapshot();
            if(s.busy||s.neural||!forget(c))continue;
            (void)nrstate::forget(c);
            graveyard_.push_back(std::move(it->second.observer));entries_.erase(it);++n;
        }
    }catch(...){}
    evicted_+=n;return n;
}

bool RecordingTable::rearmar_falha(void* command,Item& item) noexcept {
    try{
        const auto s=item.observer->snapshot();
        if(s.phase!=Phase::Failed||s.neural||s.busy)return false;
        if(graveyard_.size()==graveyard_.capacity())graveyard_.reserve(graveyard_.size()*2+16);
        if(!forget(command))return false;
        (void)nrstate::forget(command);
        graveyard_.push_back(std::move(item.observer));entries_.erase(command);++rearmadas_;return true;
    }catch(...){return false;}
}
RecordingTable::Result RecordingTable::find_or_install(void* command,std::uint64_t device,std::uint32_t owner,Origem origem) noexcept {
    std::lock_guard<std::mutex> trava(mutex_);
    Result r{};
    const bool pelo_reset=origem==Origem::Reset;
    Item* known=nullptr;
    if(command){const auto it=entries_.find(command);if(it!=entries_.end())known=&it->second;}
    if(known&&!pelo_reset&&!known->fsr&&rearmar_falha(command,*known)){known=nullptr;r.rearmada=true;}
    if(!known&&pelo_reset&&instalados_>=teto_instalados_reset_){r.stage="reset_teto_instalados";return r;}
    if(!known&&pelo_reset&&recusadas_reset_.count(command)){r.stage="reset_recusada";return r;}
    if(!known&&command&&despejar_&&(entries_.size()>=cap||nrstate::count()>=cap)){
        if(pelo_reset){
            const std::uint64_t agora=GetTickCount64();
            if(ultimo_despejo_reset_&&agora-ultimo_despejo_reset_<intervalo_reset_ms_){r.stage="reset_tabela_cheia";return r;}
            ultimo_despejo_reset_=agora;
        }
        r.evicted=evict_dead();
        if(entries_.size()>=cap||nrstate::count()>=cap)r.evicted+=evict_lru(pelo_reset);
    }
    if(!nrstate::observe(command)){r.stage="observe";r.reason=nrstate::observe_refusal();
        if(!known&&pelo_reset){try{if(recusadas_reset_.size()>=cap)recusadas_reset_.clear();recusadas_reset_.insert(command);}catch(...){}}
        return r;}
    if(known){known->uso=avancar_uso();known->fsr=known->fsr||!pelo_reset;r.observer=known->observer.get();return r;}
    if(entries_.size()>=cap){r.stage="host_recordings_cap";return r;}
    try{
        auto observer=make_windows_observer(command,device,owner);
        if(!observer){r.stage="install";r.reason=last_refusal();

            if(pelo_reset){(void)nrstate::forget(command);if(recusadas_reset_.size()>=cap)recusadas_reset_.clear();recusadas_reset_.insert(command);}
            return r;}
        auto& item=entries_[command];item.observer=std::move(observer);item.uso=avancar_uso();item.fsr=!pelo_reset;++instalados_;
        r.observer=item.observer.get();r.installed=true;return r;
    }catch(...){r.stage="host_exception";return r;}
}
}
