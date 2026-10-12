#pragma once
#include "compute_state.h"
namespace nrstate {
enum class PreserveResult { Skipped, Restored, RestoreFailed };
template<class F> PreserveResult preserve_capture(void* command,F&& capture){
    auto saved=save(command);
    if(!saved)return PreserveResult::Skipped;
    if(!saved->suspend_predication())return PreserveResult::Skipped;
    try{capture();}catch(...){(void)saved->restore();throw;}
    return saved->restore()?PreserveResult::Restored:PreserveResult::RestoreFailed;
}
}
