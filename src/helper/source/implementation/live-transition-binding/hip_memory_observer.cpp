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
    if(!enabled())return observer.load()?validate(base):true;
    Locked guard(installation_lock);
    if(observer.load())return validate(base);

    HMODULE hip=GetModuleHandleW(L"amdhip64_7.dll"),pinned{},self{};
    if(!base||!hip||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN,L"amdhip64_7.dll",&pinned)||pinned!=hip||
       !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(&install_if_requested),&self))return false;
    auto* p=new(std::nothrow) Observer;if(!p)return false;p->base=base;p->owner=GetCurrentThreadId();
    p->hooks[0]=reinterpret_cast<void*>(&on_allocate);p->hooks[1]=reinterpret_cast<void*>(&on_free);p->hooks[2]=reinterpret_cast<void*>(&on_import);p->hooks[3]=reinterpret_cast<void*>(&on_map);p->hooks[4]=reinterpret_cast<void*>(&on_destroy);
    MEMORY_BASIC_INFORMATION page{};
    auto* first=static_cast<char*>(base)+0x86B58;
    if(!VirtualQuery(first,&page,sizeof(page))||page.AllocationBase!=base||page.Type!=MEM_IMAGE||page.State!=MEM_COMMIT||
       (page.Protect&(PAGE_GUARD|PAGE_NOACCESS))||reinterpret_cast<std::uintptr_t>(static_cast<char*>(base)+0x86BE8)>reinterpret_cast<std::uintptr_t>(page.BaseAddress)+page.RegionSize){delete p;return false;}
    for(unsigned i=0;i<5;++i){
        const auto target=GetProcAddress(hip,names[i]);HMODULE owner{};
        if(!target||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(target),&owner)||owner!=hip||
           !copy_pointer(static_cast<char*>(base)+slots[i],p->originals[i])||p->originals[i]!=reinterpret_cast<void*>(target)){delete p;return false;}
    }
    const auto get=GetProcAddress(hip,"hipGetDevice"),mem=GetProcAddress(hip,"hipMemGetInfo");HMODULE get_owner{},mem_owner{};
    if(get&&mem&&GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(get),&get_owner)&&get_owner==hip&&
       GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(mem),&mem_owner)&&mem_owner==hip){
        p->get_device=reinterpret_cast<decltype(p->get_device)>(get);p->mem_info=reinterpret_cast<decltype(p->mem_info)>(mem);
    }
    p->forwarder.reset(new(std::nothrow) Forwarder(p->ledger,{reinterpret_cast<Allocate>(p->originals[0]),reinterpret_cast<Release>(p->originals[1]),reinterpret_cast<Import>(p->originals[2]),reinterpret_cast<Map>(p->originals[3]),reinterpret_cast<Release>(p->originals[4])}));
    if(!p->forwarder){delete p;return false;}
    p->file=CreateFileW(L"nr_memory.log",GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(p->file==INVALID_HANDLE_VALUE){delete p;return false;}
    observer.store(p);
    WindowsPatch transaction{base,first};const auto patched=detail::patch({&transaction,protect_slots,exchange_slot},p->originals,p->hooks,5);
    p->installed=patched.installed;
    char header[384]{};NRLOGF(header,sizeof(header),"NR_MEMORY installed=%u restore_protection=%u rollback=%u pid=%lu owner=%lu base=%p preexisting_allocations=unknown imported_bytes_overlap_mapped=true\n",p->installed?1u:0u,patched.restored?1u:0u,patched.rollback?1u:0u,GetCurrentProcessId(),p->owner,base);(void)write(*p,header);

    return p->installed&&!p->io_failed;
}
bool validate(void* base) noexcept {
    auto* p=observer.load();if(!p)return true;if(!p->installed||p->base!=base||p->io_failed)return false;
    for(unsigned i=0;i<5;++i){void* actual{};if(!copy_pointer(static_cast<char*>(base)+slots[i],actual)||actual!=p->hooks[i])return false;}
    return true;
}
void flush(bool force) noexcept {
    auto* p=observer.load();if(!p||p->file==INVALID_HANDLE_VALUE||GetCurrentThreadId()!=p->owner)return;
    const auto now=GetTickCount64();if(!force&&now-p->last_flush<500)return;p->last_flush=now;
    Event events[128];char line[768];std::size_t count{};
    while((count=p->ledger.drain(events,128))){for(std::size_t i=0;i<count;++i){const auto& e=events[i];
        NRLOGF(line,sizeof(line),"NR_MEMORY_EVENT seq=%llu time=%llu op=%s ptr=%llx related=%llx bytes=%llu result=%d thread=%u caller=%llx caller_rva=%llx\n",e.sequence,e.time,names[e.operation],e.pointer,e.related,e.bytes,e.result,e.thread,e.caller,e.caller-reinterpret_cast<std::uintptr_t>(p->base));if(!write(*p,line))return;
    }}
    const auto s=p->ledger.summary();NRLOGF(line,sizeof(line),"NR_MEMORY_SUMMARY time=%llu seq=%llu malloc_live=%llu mapped_live=%llu imported_live=%llu malloc_freed=%llu mapped_freed=%llu import_destroyed=%llu ok=%llu,%llu,%llu,%llu,%llu fail=%llu,%llu,%llu,%llu,%llu unknown_free=%llu unknown_destroy=%llu duplicate=%llu ledger_overflow=%llu events_dropped=%llu unreadable=%llu\n",now,s.sequence,s.live_malloc_bytes,s.live_mapped_bytes,s.live_import_bytes,s.freed_malloc_bytes,s.freed_mapped_bytes,s.destroyed_import_bytes,s.success[0],s.success[1],s.success[2],s.success[3],s.success[4],s.failure[0],s.failure[1],s.failure[2],s.failure[3],s.failure[4],s.unknown_free,s.unknown_destroy,s.duplicate_pointer,s.ledger_overflow,s.events_dropped,s.unreadable);(void)write(*p,line);
    if(p->get_device&&p->mem_info){
        int device=-1;std::size_t free_bytes{},total_bytes{};const int device_result=p->get_device(&device);

        const int memory_result=device_result?device_result:p->mem_info(&free_bytes,&total_bytes);
        NRLOGF(line,sizeof(line),"NR_MEMORY_DEVICE time=%llu device=%d get_result=%d mem_result=%d free=%llu total=%llu used=%llu scope=device_wide\n",now,device,device_result,memory_result,static_cast<unsigned long long>(free_bytes),static_cast<unsigned long long>(total_bytes),static_cast<unsigned long long>(total_bytes>=free_bytes?total_bytes-free_bytes:0));(void)write(*p,line);
    }
}
}
