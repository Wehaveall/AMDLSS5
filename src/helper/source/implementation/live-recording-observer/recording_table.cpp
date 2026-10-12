#include "recording_table.h"
#include "compute_state.h"
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
    in_use_n_=0;
    for(unsigned i=0;i<count&&in_use_n_<in_use_.size();++i)if(commands&&commands[i])in_use_[in_use_n_++]=commands[i];
}
unsigned RecordingTable::evict_dead() noexcept {
    unsigned n=0;
    try{
        for(auto it=entries_.begin();it!=entries_.end();){
            void* const c=it->first;bool usada=false;
            for(unsigned i=0;i<in_use_n_;++i)usada=usada||in_use_[i]==c;

            if(usada||it->second->snapshot().busy||!morta(c)||!forget(c)){++it;continue;}
            (void)nrstate::forget(c);
            graveyard_.push_back(std::move(it->second));it=entries_.erase(it);++n;
        }
    }catch(...){}
    evicted_+=n;return n;
}
RecordingTable::Result RecordingTable::find_or_install(void* command,std::uint64_t device,std::uint32_t owner) noexcept {
    Result r{};
    std::pair<void*,std::unique_ptr<Observer>>* known=nullptr;
    for(auto& entry:entries_)if(entry.first==command){known=&entry;break;}
    if(!known&&command&&despejar_&&(entries_.size()>=cap||nrstate::count()>=cap))r.evicted=evict_dead();
    if(!nrstate::observe(command)){r.stage="observe";r.reason=nrstate::observe_refusal();return r;}
    if(known){r.observer=known->second.get();return r;}
    if(entries_.size()>=cap){r.stage="host_recordings_cap_64";return r;}
    try{
        auto observer=make_windows_observer(command,device,owner);
        if(!observer){r.stage="install";r.reason=last_refusal();return r;}
        entries_.emplace_back(command,std::move(observer));
        r.observer=entries_.back().second.get();r.installed=true;return r;
    }catch(...){r.stage="host_exception";return r;}
}
}
