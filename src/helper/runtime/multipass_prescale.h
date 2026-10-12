#pragma once

#include "multipass_math.h"
#include "pre_scale_state.h"
#include <atomic>
namespace nrmpprescale {

inline std::atomic<bool> sem_teto{false};

inline bool prescale_chains(unsigned request){
    const unsigned v=nrmp::valid_config(request);
    return (v&255u)>1u && nrmp::multiplier(v)!=0.f;
}

inline double multipass_cap_percent(int width,int height,double percent,unsigned request){
    if(!prescale_chains(request)||sem_teto.load(std::memory_order_relaxed))return percent;
    const double teto=nrprescale::accepted_percent(nrprescale::auto_percent(width,height,nrprescale::auto_alvo_padrao));
    return percent<teto?percent:teto;
}
}
