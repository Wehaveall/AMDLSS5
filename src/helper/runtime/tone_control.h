#pragma once
#include <atomic>
#include <cmath>
#include <cstring>
namespace nrtone {
inline constexpr unsigned failed=~0u;
struct State {std::atomic<unsigned> requested{0},effective{0};};
inline State state;
inline std::atomic<void*> owner{nullptr};
struct Ticket {
    State& state; unsigned selection; unsigned& last; void*& last_context; void* context;
    bool changed=false,valid=true,done=false;
    Ticket(const Ticket&)=delete;
    Ticket(State& s,unsigned v,unsigned& l,void*& lc,void* c,bool ch,bool ok)
        :state(s),selection(v),last(l),last_context(lc),context(c),changed(ch),valid(ok) {}
    void complete() noexcept {
        if(valid){last=selection;last_context=context;state.effective.store(selection,std::memory_order_release);done=true;}
    }
    ~Ticket() {if(!done){last=failed;state.effective.store(failed,std::memory_order_release);}}
};

struct Worker {
    unsigned last=0; void* last_context=nullptr; float previous_local=0;
    Ticket prepare(State& s,void* context,float local_structure) noexcept {
        const unsigned request=s.requested.load(std::memory_order_acquire);
        const bool valid=request<=9 && context && (request==0 ||
            (std::isfinite(local_structure)&&local_structure>=0 && local_structure<=2));
        const bool changed=valid && (request!=last || (request!=0 && (context!=last_context || previous_local!=local_structure)));
        if(valid) {
            auto bytes=static_cast<char*>(context);
            previous_local=local_structure;

            if(request) {float tone=float(request-1)*.25f;std::memcpy(bytes+0x20,&tone,4);std::memcpy(bytes+0x24,&local_structure,4);}
            if(changed) {
                const unsigned zero=0;void* empty=nullptr;
                std::memcpy(bytes+0xf0,&zero,4);std::memcpy(bytes+0x120,&zero,4);
                std::memcpy(bytes+0x38,&zero,4);std::memcpy(bytes+0x118,&empty,sizeof(empty));
            }
        }
        return Ticket(s,request,last,last_context,context,changed,valid);
    }
};
inline Worker worker;
}
