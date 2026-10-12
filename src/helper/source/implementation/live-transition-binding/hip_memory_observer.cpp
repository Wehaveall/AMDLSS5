#include "../../../runtime/nr_publico.h"
#include "hip_memory_observer.h"
#include <intrin.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <new>
namespace nrmem {
detail::PatchResult detail::patch(PatchOps ops,void* const* originals,void* const* hooks,unsigned count) noexcept {
    PatchResult result;
    if(!ops.protect(ops.context,false))return result;
    unsigned written=0;
    for(;written<count;++written)if(ops.exchange(ops.context,written,hooks[written],originals[written])!=originals[written])break;
    const bool complete=written==count;
    if(!complete)while(written){--written;if(ops.exchange(ops.context,written,originals[written],hooks[written])!=hooks[written])result.rollback=false;}
    result.restored=ops.protect(ops.context,true);result.installed=complete&&result.restored;return result;
}
namespace {
struct Locked {SRWLOCK& lock;explicit Locked(SRWLOCK& l):lock(l){AcquireSRWLockExclusive(&lock);}~Locked(){ReleaseSRWLockExclusive(&lock);}};
std::uint64_t& live(Summary& s,unsigned kind){return kind==0?s.live_malloc_bytes:kind==1?s.live_mapped_bytes:s.live_import_bytes;}
std::uint64_t& freed(Summary& s,unsigned kind){return kind==0?s.freed_malloc_bytes:kind==1?s.freed_mapped_bytes:s.destroyed_import_bytes;}
}
void Ledger::record(Operation op,std::uintptr_t pointer,std::uintptr_t related,std::uint64_t bytes,int result,unsigned thread,std::uintptr_t caller) noexcept {
    Locked guard(lock_);const unsigned operation=static_cast<unsigned>(op);
    if(result)++summary_.failure[operation];else ++summary_.success[operation];
    const bool adding=op==Operation::Malloc||op==Operation::Map||op==Operation::Import;
    const unsigned kind=op==Operation::Import?2u:op==Operation::Map?1u:0u;
    if(!result&&pointer){

        const bool external=op==Operation::Import||op==Operation::Destroy;
        const std::size_t start=((pointer>>4)^(pointer>>19)^(external?0xAF3u:0u))%capacity;
        Allocation* found=nullptr;Allocation* vacant=nullptr;
        for(std::size_t i=0;i<capacity;++i){auto& cell=allocations_[(start+i)%capacity];
            if(cell.state==0){if(!vacant)vacant=&cell;break;}
            if(cell.state==2){if(!vacant)vacant=&cell;continue;}
            if(cell.pointer==pointer&&(cell.kind==2)==external){found=&cell;break;}
        }
        if(adding){
            if(found){++summary_.duplicate_pointer;live(summary_,found->kind)-=found->bytes;}
            auto* target=found?found:vacant;
            if(target){*target={pointer,bytes,kind,1};live(summary_,kind)+=bytes;}
            else ++summary_.ledger_overflow;
        }else if(found){bytes=found->bytes;live(summary_,found->kind)-=bytes;freed(summary_,found->kind)+=bytes;found->state=2;}
        else if(external)++summary_.unknown_destroy;else ++summary_.unknown_free;
    }
    const Event event{++summary_.sequence,GetTickCount64(),pointer,related,bytes,caller,thread,operation,result};
    if(event_count_<event_capacity){events_[(event_read_+event_count_)%event_capacity]=event;++event_count_;}
    else ++summary_.events_dropped;
}
void Ledger::unreadable() noexcept {Locked guard(lock_);++summary_.unreadable;}
Summary Ledger::summary() noexcept {Locked guard(lock_);return summary_;}
std::size_t Ledger::drain(Event* out,std::size_t maximum) noexcept {
    Locked guard(lock_);const auto n=maximum<event_count_?maximum:event_count_;
    for(std::size_t i=0;i<n;++i)out[i]=events_[(event_read_+i)%event_capacity];
    event_read_=(event_read_+n)%event_capacity;event_count_-=n;return n;
}
std::uint64_t Forwarder::read(const void* address,std::size_t offset) noexcept {
    std::uint64_t value{};SIZE_T done{};
    if(!address||!ReadProcessMemory(GetCurrentProcess(),static_cast<const char*>(address)+offset,&value,sizeof(value),&done)||done!=sizeof(value)){ledger_.unreadable();return 0;}
    return value;
}
int Forwarder::allocate(void** out,std::size_t bytes,unsigned thread,std::uintptr_t caller) noexcept {
    const int result=calls_.allocate(out,bytes);
    ledger_.record(Operation::Malloc,result?0:static_cast<std::uintptr_t>(read(out,0)),0,bytes,result,thread,caller);return result;
}
int Forwarder::free(void* ptr,unsigned thread,std::uintptr_t caller) noexcept {
    const int result=calls_.free(ptr);ledger_.record(Operation::Free,reinterpret_cast<std::uintptr_t>(ptr),0,0,result,thread,caller);return result;
}
int Forwarder::import_memory(void** out,const void* desc,unsigned thread,std::uintptr_t caller) noexcept {
    const int result=calls_.import_memory(out,desc);

    ledger_.record(Operation::Import,result?0:static_cast<std::uintptr_t>(read(out,0)),0,result?0:read(desc,24),result,thread,caller);return result;
}
int Forwarder::map(void** out,void* external,const void* desc,unsigned thread,std::uintptr_t caller) noexcept {
    const int result=calls_.map(out,external,desc);
    ledger_.record(Operation::Map,result?0:static_cast<std::uintptr_t>(read(out,0)),reinterpret_cast<std::uintptr_t>(external),result?0:read(desc,8),result,thread,caller);return result;
}
int Forwarder::destroy(void* ptr,unsigned thread,std::uintptr_t caller) noexcept {
    const int result=calls_.destroy(ptr);ledger_.record(Operation::Destroy,reinterpret_cast<std::uintptr_t>(ptr),0,0,result,thread,caller);return result;
}
namespace {
constexpr unsigned slots[5]{0x86BE0,0x86BA8,0x86BD0,0x86BA0,0x86B58};
constexpr const char* names[5]{"hipMalloc","hipFree","hipImportExternalMemory","hipExternalMemoryGetMappedBuffer","hipDestroyExternalMemory"};
struct Observer {
    void* base{};void* originals[5]{};void* hooks[5]{};Ledger ledger;
    std::unique_ptr<Forwarder> forwarder;
    HANDLE file=INVALID_HANDLE_VALUE;DWORD owner{};std::uint64_t last_flush{};
    bool installed{},io_failed{};
    int(__cdecl* get_device)(int*){};
    int(__cdecl* mem_info)(std::size_t*,std::size_t*){};
};
std::atomic<Observer*> observer{};
SRWLOCK installation_lock=SRWLOCK_INIT;
int __cdecl on_allocate(void** out,std::size_t n){return observer.load()->forwarder->allocate(out,n,GetCurrentThreadId(),reinterpret_cast<std::uintptr_t>(_ReturnAddress()));}
int __cdecl on_free(void* ptr){return observer.load()->forwarder->free(ptr,GetCurrentThreadId(),reinterpret_cast<std::uintptr_t>(_ReturnAddress()));}
int __cdecl on_import(void** out,const void* desc){return observer.load()->forwarder->import_memory(out,desc,GetCurrentThreadId(),reinterpret_cast<std::uintptr_t>(_ReturnAddress()));}
int __cdecl on_map(void** out,void* external,const void* desc){return observer.load()->forwarder->map(out,external,desc,GetCurrentThreadId(),reinterpret_cast<std::uintptr_t>(_ReturnAddress()));}
int __cdecl on_destroy(void* ptr){return observer.load()->forwarder->destroy(ptr,GetCurrentThreadId(),reinterpret_cast<std::uintptr_t>(_ReturnAddress()));}
bool enabled() noexcept {char value[4]{};return GetEnvironmentVariableA("NR_MEMORY_DIAGNOSTICS",value,sizeof(value))==1&&value[0]=='1';}
bool copy_pointer(const void* address,void*& out) noexcept {SIZE_T done{};return ReadProcessMemory(GetCurrentProcess(),address,&out,sizeof(out),&done)&&done==sizeof(out);}
bool write(Observer& p,const char* line) noexcept {
    if(p.io_failed)return false;DWORD done{};const auto size=static_cast<DWORD>(std::strlen(line));
    if(!WriteFile(p.file,line,size,&done,nullptr)||done!=size){p.io_failed=true;OutputDebugStringA("NR_MEMORY_DIAGNOSTICS log write failed\n");return false;}return true;
}
struct WindowsPatch {void* base;void* first;DWORD old{};};
bool protect_slots(void* context,bool restore) noexcept {
    auto& p=*static_cast<WindowsPatch*>(context);DWORD ignored{};
    return restore?VirtualProtect(p.first,0x90,p.old,&ignored)!=FALSE:VirtualProtect(p.first,0x90,PAGE_READWRITE,&p.old)!=FALSE;
}
void* exchange_slot(void* context,unsigned i,void* replacement,void* expected) noexcept {
    auto& p=*static_cast<WindowsPatch*>(context);return InterlockedCompareExchangePointer(reinterpret_cast<void**>(static_cast<char*>(p.base)+slots[i]),replacement,expected);
}
}
bool install_if_requested(void* base) noexcept {
(void)base;return true;
}
bool validate(void* base) noexcept {
    auto* p=observer.load();if(!p)return true;if(!p->installed||p->base!=base||p->io_failed)return false;
    for(unsigned i=0;i<5;++i){void* actual{};if(!copy_pointer(static_cast<char*>(base)+slots[i],actual)||actual!=p->hooks[i])return false;}
    return true;
}
void flush(bool force) noexcept {
(void)force;
}
}
