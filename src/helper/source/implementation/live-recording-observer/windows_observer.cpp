#include "observer_platform.h"
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <cstring>
#include <cstdio>
namespace nrrecord {
bool detail::table_protection_matches(std::uint32_t initial,std::uint32_t actual,bool checked) noexcept {
    return (initial==PAGE_READONLY||initial==PAGE_READWRITE||initial==PAGE_WRITECOPY)&&
        (initial==actual||(checked&&initial==PAGE_WRITECOPY&&actual==PAGE_READWRITE));
}
detail::TableRegion detail::classify_table_region(std::uint32_t state,std::uint32_t type,std::uint32_t protect,bool hooked_slots_in_d3d12) noexcept {
    if(state!=MEM_COMMIT||(protect&(PAGE_GUARD|PAGE_NOACCESS)))return TableRegion::Refused;
    if(type==MEM_IMAGE)return protect==PAGE_READONLY||protect==PAGE_READWRITE||protect==PAGE_WRITECOPY?TableRegion::Image:TableRegion::Refused;
    if(type==MEM_PRIVATE&&(protect==PAGE_READONLY||protect==PAGE_READWRITE)&&hooked_slots_in_d3d12)return TableRegion::Private;
    return TableRegion::Refused;
}
namespace {
bool code_in(const void* address,HMODULE& module) noexcept {
    MEMORY_BASIC_INFORMATION info{};
    if(!address||!VirtualQuery(address,&info,sizeof(info))||info.State!=MEM_COMMIT||info.Type!=MEM_IMAGE||
       (info.Protect!=PAGE_EXECUTE_READ&&info.Protect!=PAGE_EXECUTE_WRITECOPY))return false;
    return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,static_cast<LPCWSTR>(address),&module)!=FALSE;
}
}
bool detail::table_inside_object(std::uintptr_t object,std::uintptr_t table) noexcept {
    return object&&table%sizeof(void*)==0&&table>=object+sizeof(void*)&&table-object<0x10000u;
}
bool detail::same_private_offset(std::uintptr_t object,std::uintptr_t table,std::uintptr_t offset) noexcept {
    return table_inside_object(object,table)&&table-object==offset;
}
bool detail::d3d12_runtime_or_driver_code(const void* address) noexcept {
    HMODULE module{};if(!code_in(address,module))return false;
    wchar_t path[MAX_PATH]{};const DWORD length=GetModuleFileNameW(module,path,MAX_PATH);if(!length||length>=MAX_PATH)return false;
    const wchar_t* name=wcsrchr(path,L'\\');name=name?name+1:path;
    if(_wcsicmp(name,L"d3d12.dll")==0||_wcsicmp(name,L"D3D12Core.dll")==0)return true;
    wchar_t store[MAX_PATH]{};const UINT n=GetSystemDirectoryW(store,MAX_PATH);
    if(!n||n>=MAX_PATH-16||wcscat_s(store,L"\\DriverStore\\"))return false;
    return _wcsnicmp(path,store,wcslen(store))==0;
}
namespace {
using Microsoft::WRL::ComPtr;
bool read(const void* address,void* out,std::size_t size) noexcept {SIZE_T count{};return ReadProcessMemory(GetCurrentProcess(),address,out,size,&count)&&count==size;}
bool pin(void* address,HMODULE expected=nullptr) noexcept {
    HMODULE module{};return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(address),&module)&&(!expected||module==expected);
}
bool range(void* address,std::size_t bytes,MEMORY_BASIC_INFORMATION& info,bool code) noexcept {
    if(!VirtualQuery(address,&info,sizeof(info))||info.State!=MEM_COMMIT||info.Type!=MEM_IMAGE||info.Protect&(PAGE_GUARD|PAGE_NOACCESS))return false;
    const auto offset=reinterpret_cast<std::uintptr_t>(address)-reinterpret_cast<std::uintptr_t>(info.BaseAddress);
    if(offset>info.RegionSize||bytes>info.RegionSize-offset)return false;
    const auto p=info.Protect;
    return code?(p==PAGE_EXECUTE_READ||p==PAGE_EXECUTE_WRITECOPY):(p==PAGE_READONLY||p==PAGE_READWRITE||p==PAGE_WRITECOPY);
}

bool private_range(void* address,std::size_t bytes,MEMORY_BASIC_INFORMATION& info,bool slots_ok) noexcept {
    if(!VirtualQuery(address,&info,sizeof(info)))return false;
    const auto offset=reinterpret_cast<std::uintptr_t>(address)-reinterpret_cast<std::uintptr_t>(info.BaseAddress);
    if(offset>info.RegionSize||bytes>info.RegionSize-offset)return false;
    return detail::classify_table_region(info.State,info.Type,info.Protect,slots_ok)==detail::TableRegion::Private;
}

bool private_slot(const void* address) noexcept {return detail::d3d12_runtime_or_driver_code(address);}
class WindowsPlatform final:public detail::Platform {
    ComPtr<ID3D12GraphicsCommandList> retained_;
    void** table_{};
    HMODULE table_module_{};
    DWORD protection_{};
    unsigned replaced_slots_{};
    bool restored_{};
    bool private_{},unprotected_{};
    void* original_slots_[2]{};

    bool table_range(void** table,MEMORY_BASIC_INFORMATION& info) noexcept {
        return private_?private_range(table+9,2*sizeof(void*),info,true):range(table+9,2*sizeof(void*),info,false);
    }
    char diagnostic_[384]{"windows_platform_initial"};
public:
    bool prepare(void* command,std::uint64_t device,std::uint32_t owner,detail::Targets& targets) noexcept override {

        using detail::refuse;
        if(!command)return refuse("prepare_null_command");
        if(!device)return refuse("prepare_null_device");
        if(owner!=GetCurrentThreadId())return refuse("prepare_owner_thread");
        ComPtr<ID3D12GraphicsCommandList> list;
        if(FAILED(static_cast<IUnknown*>(command)->QueryInterface(IID_PPV_ARGS(&list))))return refuse("prepare_qi_graphics_list_failed");
        if(list.Get()!=command)return refuse("prepare_qi_graphics_list_other_pointer");

        if(list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT&&list->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE)return refuse("prepare_type_not_direct");
        ComPtr<ID3D12Device> actual;ComPtr<IUnknown> a,b;
        if(FAILED(list->GetDevice(IID_PPV_ARGS(&actual))))return refuse("prepare_get_device_failed");
        if(FAILED(actual.As(&a)))return refuse("prepare_list_device_unknown_failed");
        if(FAILED(reinterpret_cast<IUnknown*>(device)->QueryInterface(IID_PPV_ARGS(&b))))return refuse("prepare_canonical_device_unknown_failed");
        if(a.Get()!=b.Get())return refuse("prepare_device_identity_differs");
        if(!read(command,&table_,sizeof(table_))||!table_||reinterpret_cast<std::uintptr_t>(table_)%alignof(void*))return refuse("prepare_vtable_read");
        MEMORY_BASIC_INFORMATION info{};
        if(!range(table_+9,2*sizeof(void*),info,false)){

            void* candidate[2]{};
            const bool slots_ok=read(table_+9,candidate,sizeof(candidate))&&private_slot(candidate[0])&&private_slot(candidate[1]);
            if(!detail::table_inside_object(reinterpret_cast<std::uintptr_t>(command),reinterpret_cast<std::uintptr_t>(table_)))
                return refuse("prepare_private_vtable_outside_object");
            if(!private_range(table_+9,2*sizeof(void*),info,slots_ok)){
                MEMORY_BASIC_INFORMATION probe{};
                const bool private_region=VirtualQuery(table_+9,&probe,sizeof(probe))&&probe.Type==MEM_PRIVATE;
                return refuse(private_region&&!slots_ok?"prepare_private_vtable_slots_not_d3d12":"prepare_vtable_not_image_readonly");
            }
            private_=true;
        }
        table_module_=static_cast<HMODULE>(info.AllocationBase);protection_=info.Protect;
        void* slots[2]{};if(!read(table_+9,slots,sizeof(slots))||!slots[0]||!slots[1])return refuse("prepare_slots_read");
        MEMORY_BASIC_INFORMATION target{};
        if(!range(slots[0],1,target,true))return refuse("prepare_close_not_image_code");
        if(!pin(slots[0],static_cast<HMODULE>(target.AllocationBase)))return refuse("prepare_close_module_pin");
        if(!range(slots[1],1,target,true))return refuse("prepare_reset_not_image_code");
        if(!pin(slots[1],static_cast<HMODULE>(target.AllocationBase)))return refuse("prepare_reset_module_pin");
        if(!private_&&!pin(table_,table_module_))return refuse("prepare_vtable_module_pin");
        if(!pin(reinterpret_cast<void*>(&make_windows_observer)))return refuse("prepare_self_pin");
        original_slots_[0]=slots[0];original_slots_[1]=slots[1];
        retained_=list;targets={table_,reinterpret_cast<detail::Close>(slots[0]),reinterpret_cast<detail::Reset>(slots[1])};return true;
    }
    std::uint32_t thread() noexcept override{return GetCurrentThreadId();}
    void release() noexcept override{retained_.Reset();}
    const char* diagnostic() const noexcept override{return diagnostic_;}
    bool private_table() const noexcept override{return private_;}
    bool writable(void** table,std::uint32_t& previous) noexcept override {
        restored_=false;unprotected_=false;
        MEMORY_BASIC_INFORMATION info{};if(table!=table_||!table_range(table,info)||info.AllocationBase!=table_module_)return false;

        if(private_&&info.Protect==PAGE_READWRITE){previous=PAGE_READWRITE;unprotected_=true;return true;}
        DWORD old{};if(!VirtualProtect(table+9,2*sizeof(void*),PAGE_READWRITE,&old))return false;previous=old;return true;
    }
    bool restore(void** table,std::uint32_t previous) noexcept override {
        if(unprotected_){unprotected_=false;const bool ok=table==table_&&previous==PAGE_READWRITE;restored_=ok&&previous==protection_;return ok;}
        DWORD unused{};const bool ok=table==table_&&VirtualProtect(table+9,2*sizeof(void*),previous,&unused);
        restored_=ok&&previous==protection_;return ok;
    }
    bool exchange(void** slot,void* expected,void* desired) noexcept override {
        if(slot!=table_+9&&slot!=table_+10)return false;
        const bool ok=InterlockedCompareExchangePointer(slot,desired,expected)==expected;
        const auto index=slot==table_+9?0u:1u;
        if(ok&&expected==original_slots_[index]&&desired!=expected)replaced_slots_|=1u<<index;
        if(ok&&desired==original_slots_[index])replaced_slots_&=~(1u<<index);
        return ok;
    }

    const char* drift_reason(void* command,void** table) noexcept override {
        void** current{};
        if(private_||command!=retained_.Get()||!read(command,&current,sizeof(current))||!current||current==table)return nullptr;
        return detail::table_inside_object(reinterpret_cast<std::uintptr_t>(command),reinterpret_cast<std::uintptr_t>(current))?
            "observer_list_moved_to_private_vtable":nullptr;
    }
    bool intact(void* command,void** table,void* close,void* reset) noexcept override {
        void** current{};void* slots[2]{};MEMORY_BASIC_INFORMATION info{};
        const bool object=command==retained_.Get(),same_table=table==table_;
        const bool object_read=read(command,&current,sizeof(current)),vptr=object_read&&current==table;
        const bool page=same_table&&table_range(table,info);
        const bool allocation=info.AllocationBase==table_module_;
        const bool checked_write_restore=replaced_slots_==3&&restored_;
        const bool protection=detail::table_protection_matches(protection_,info.Protect,checked_write_restore);
        const bool slots_read=same_table&&read(table+9,slots,sizeof(slots));
        const bool close_match=slots_read&&slots[0]==close,reset_match=slots_read&&slots[1]==reset;
        const bool ok=object&&same_table&&vptr&&page&&allocation&&protection&&close_match&&reset_match;

        if(ok)return true;
        std::snprintf(diagnostic_,sizeof(diagnostic_),
            "win_intact_object_%u_table_%u_vptrread_%u_vptr_%u_range_%u_allocation_%u_initialprotect_%08lX_actualprotect_%08lX_protectmatch_%u_slotsread_%u_close_%u_reset_%u_checked_cow_%u",
            object?1u:0u,same_table?1u:0u,object_read?1u:0u,vptr?1u:0u,page?1u:0u,allocation?1u:0u,
            protection_,info.Protect,protection?1u:0u,slots_read?1u:0u,close_match?1u:0u,reset_match?1u:0u,checked_write_restore?1u:0u);
        return false;
    }
};
}
std::unique_ptr<Observer> make_windows_observer(void* command,std::uint64_t device,std::uint32_t owner) noexcept {
    try{return detail::install(std::make_unique<WindowsPlatform>(),command,device,owner);}catch(...){detail::refuse("make_windows_observer_exception");return {};}
}
}
