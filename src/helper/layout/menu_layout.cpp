#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../runtime/nr_publico.h"
#include <windows.h>
#include <dxgi.h>
#include <array>
#include <atomic>
#include <cstring>
#include <cstdio>
#include <memory>
#include <mutex>
#include <limits>
#include <stdexcept>
#include "menu_layout.h"
#include "geometry.h"
#include "stub.h"
#include "menu_bridge.h"
#include "menu_i18n.h"
#include "menu_frame.h"
#include "menu_compacto.h"
#include "../runtime/painel_rede.h"
#include "live_image.h"
#include "../source/implementation/compat-diag/compat.h"
#include "../runtime/overlay_amd.h"

namespace nrmenulayout {
namespace {
struct NativeVector { Vertex* begin;Vertex* end;Vertex* capacity;float scale; };
thread_local Context* current{};
std::mutex install_mutex;
std::atomic<void*> installed_base{};
bool installation_attempted{};
void* stub_allocation{};
constexpr std::size_t hook_rva=0x20521;
constexpr unsigned char original[]{0x48,0x8b,0x85,0xd0,0x01,0x00,0x00};
void log(const char* message) noexcept {
(void)message;
}
bool disk_admitted(void* base) {
    wchar_t path[32768]{};
    if(!GetModuleFileNameW(static_cast<HMODULE>(base),path,32768))return false;
    HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE)return false;
    LARGE_INTEGER size{};bool ok=GetFileSizeEx(h,&size)&&size.QuadPart==nrliveimage::file_size;
    std::vector<unsigned char> bytes;
    if(ok){bytes.resize(nrliveimage::file_size);DWORD got{};ok=ReadFile(h,bytes.data(),DWORD(bytes.size()),&got,nullptr)&&got==bytes.size();}
    CloseHandle(h);return ok&&nrliveimage::inspect_disk_snapshot(bytes.data(),bytes.size());
}
void destroy_string(void* base,nrmenu::NativeString& value) noexcept {
    if(value.capacity<16)return;

    void* pointer{};std::memcpy(&pointer,value.text,sizeof pointer);
    std::size_t count=std::size_t(value.capacity)+1;
    if(count>=0x1000){pointer=*(static_cast<void**>(pointer)-1);count=std::size_t(value.capacity)+0x28;}
    using Free=void(__cdecl*)(void*,std::size_t);
    reinterpret_cast<Free>(static_cast<unsigned char*>(base)+0x38510)(pointer,count);
}
struct StringGuard {void* base;nrmenu::NativeString value{};~StringGuard(){destroy_string(base,value);}};

std::string native_text(const unsigned char* base,std::size_t rva,std::size_t limit) {
    const auto* value=reinterpret_cast<const nrmenu::NativeString*>(base+rva);
    if(!value->size||value->size>limit||value->capacity<value->size)return {};
    const char* text=value->text;if(value->capacity>=16)std::memcpy(&text,value->text,sizeof text);
    if(!text)return {};
    return std::string(text,std::size_t(value->size));
}

void fps_texts(const nrmenufps::Display& d,const std::string& upscaler,nrmenu::Language lang,Frame& f) {
    using nrmenu::Ui;
    char text[64]{};
    if(!d.read_ok){f.fps=nrmenu::ui(Ui::FpsCollecting,lang);}
    else if(d.display_valid){std::snprintf(text,sizeof text,"%.0f FPS",d.display_fps);f.fps=text;}
    else if(d.input_valid){std::snprintf(text,sizeof text,"%.0f FPS",d.input_fps);f.fps=text;}
    else f.fps="-- FPS";
    std::string detail;
    auto add=[&](const std::string& part){if(part.empty())return;if(!detail.empty())detail+=" · ";detail+=part;};

    if(f.status.find(upscaler)==std::string::npos)add(upscaler);
    if(d.read_ok&&d.display_valid&&d.input_valid){std::snprintf(text,sizeof text,"%s %.1f",nrmenu::ui(Ui::Input,lang),d.input_fps);add(text);}
    if(d.read_ok&&!d.display_valid)add(nrmenu::ui(Ui::DisplayUnavailable,lang));
    add(nrmenu::ui(d.fg==2?Ui::FgActive:d.fg==1?Ui::FgIdle:Ui::FgNone,lang));
    f.fps_detail=detail;
}
Navigator navigator;
Vertex* __cdecl rebuild(NativeVector* native) noexcept {
    if(!current||!native)return native?native->begin:nullptr;
    try {
        auto* base=static_cast<unsigned char*>(current->base);
        auto* begin=*reinterpret_cast<nrmenu::NativeRow**>(base+0x923d0);
        auto* end=*reinterpret_cast<nrmenu::NativeRow**>(base+0x923d8);
        const auto a=reinterpret_cast<std::uintptr_t>(begin),b=reinterpret_cast<std::uintptr_t>(end);
        if(b<a||(b-a)%sizeof(nrmenu::NativeRow)||(b-a)/sizeof(nrmenu::NativeRow)>128)return native->begin;
        const std::size_t count=(b-a)/sizeof(nrmenu::NativeRow);
        const auto lang=nrmenu::language();

        std::vector<NativeView> views(count);
        for(std::size_t i=0;i<count;++i)views[i]={begin[i].label,begin[i].help};
        const auto order=catalog_order(views);
        auto* selection=reinterpret_cast<int*>(base+0x923b4);
        const unsigned native_selected=unsigned(std::max(0,*selection));
        const unsigned corrected=navigator.update(order,native_selected,begin);
        if(corrected!=native_selected&&corrected<count)*selection=int(corrected);
        auto f=compose_frame(views,order,corrected,lang,[&](unsigned index){
            const auto* p=begin+index;
            StringGuard str{base};str.value.capacity=15;
            if(p->getter){using Read=nrmenu::NativeString*(__cdecl*)(void*,nrmenu::NativeString*);auto** table=*reinterpret_cast<void***>(p->getter);reinterpret_cast<Read>(table[2])(p->getter,&str.value);}
            if(str.value.size>65536||str.value.capacity>1024*1024)throw std::runtime_error("invalid menu value");
            const char* value=str.value.text;if(str.value.capacity>=16)std::memcpy(&value,str.value.text,sizeof value);
            return std::string(value,std::size_t(str.value.size));
        });
        const auto cw=*reinterpret_cast<unsigned*>(base+0x92380),ch=*reinterpret_cast<unsigned*>(base+0x92384),
            aw=*reinterpret_cast<unsigned*>(base+0x92388),ah=*reinterpret_cast<unsigned*>(base+0x9238c);
        if(!cw||!ch||!aw||!ah||cw>128||ch>128)return native->begin;
        const std::string status=native_text(base,0x923e8,1024);
        if(!status.empty())f.status=nrmenu::status(status,lang);

        if(const int med=nrpainel::mediana_ms.load(std::memory_order_relaxed);med>=0&&!f.status.empty()){
            const auto sp=network_ms_span(f.status);
            if(sp.end>sp.begin+3)f.status=f.status.substr(0,sp.begin)+std::to_string(med)+f.status.substr(sp.end-3);
        }

        if(*reinterpret_cast<const volatile std::uint8_t*>(base+0x94A8C)!=0){
            const auto motivo=nrcompat::last_reason();
            if(motivo.id[0])f.status=nrmenu::network_off(motivo.id,motivo.campo,lang);
        }

        if(nroverlayamd::estado_aviso.load(std::memory_order_relaxed)==1)f.status=nrmenu::network_off("overlay_amd",nullptr,lang);
        else if(nroverlayamd::pausa_recriacao.load(std::memory_order_relaxed))f.status=nrmenu::network_off("overlay_amd_recriacao",nullptr,lang);
        fps_texts(current->counters,native_text(base,0x94880,64),lang,f);
        auto canvas=nrmenucompacto::desenhando.load(std::memory_order_relaxed)?build_compacto(current->width,current->height,f,float(aw),float(ah),float(cw),float(ch),
                                                                                            *reinterpret_cast<const volatile std::uint8_t*>(base+0x94A8C)!=0?1:0)
                                                                             :build(current->width,current->height,f,float(aw),float(ah),float(cw),float(ch));
        if(canvas.vertices.empty()||canvas.vertices.size()*sizeof(Vertex)>0x100000)return native->begin;

        native->end=native->begin;
        using Append=void(__cdecl*)(NativeVector*,Vertex*,const Vertex*,std::size_t);
        reinterpret_cast<Append>(base+0x26870)(native,native->end,canvas.vertices.data(),canvas.vertices.size());
        static std::once_flag once;std::call_once(once,[]{log("CPU_LAYOUT_ACTIVE AMDLSS5 MOD compact rows, catalog order, 3 languages");});
    }catch(...){log("CPU_LAYOUT_EXCEPTION original native menu retained where possible");}
    return native->begin;
}
bool install(void* address) noexcept {
    if(address&&installed_base.load(std::memory_order_acquire)==address)return true;
    std::lock_guard<std::mutex> lock(install_mutex);
    if(installation_attempted)return installed_base.load(std::memory_order_relaxed)==address;
    installation_attempted=true;
    try {
        auto* base=static_cast<unsigned char*>(address);auto* site=base+hook_rva;
        constexpr unsigned char append_bytes[]{0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x56,0x57,0x55,0x53,0x48,0x83,0xec,0x48};
        constexpr unsigned char free_bytes[]{0xe9,0xdb,0x08,0,0,0xcc,0xcc,0xcc};
        if(!address||!disk_admitted(address)||std::memcmp(site,original,sizeof original)!=0||
           std::memcmp(base+0x26870,append_bytes,sizeof append_bytes)!=0||std::memcmp(base+0x38510,free_bytes,sizeof free_bytes)!=0){log("REJECTED module hash or renderer/allocator bytes");return false;}
        SYSTEM_INFO info{};GetSystemInfo(&info);const std::uintptr_t gran=info.dwAllocationGranularity;
        const auto center=reinterpret_cast<std::uintptr_t>(site)&~(gran-1);
        unsigned char* memory{};
        for(std::uintptr_t delta=gran;delta<0x70000000&&!memory;delta+=gran) {
            if(center>delta)memory=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(center-delta),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
            if(!memory&&center+delta>center)memory=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(center+delta),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        }
        if(!memory){log("REJECTED no near trampoline allocation");return false;}
        const auto code=detail::make_stub(reinterpret_cast<std::uint64_t>(&rebuild));std::memcpy(memory,code.data(),code.size());

        const unsigned char unwind[]{1,7,2,0,7,1,0x19,0};
        std::memcpy(memory+0x200,unwind,sizeof unwind);
        auto* runtime=reinterpret_cast<RUNTIME_FUNCTION*>(memory+0x220);*runtime={0,DWORD(code.size()),0x200};
        DWORD prior{};
        if(!VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&prior)||!RtlAddFunctionTable(runtime,1,reinterpret_cast<DWORD64>(memory))){VirtualFree(memory,0,MEM_RELEASE);log("REJECTED trampoline protection/unwind");return false;}
        const auto displacement=memory-(site+5);
        if(displacement<std::numeric_limits<std::int32_t>::min()||displacement>std::numeric_limits<std::int32_t>::max()){RtlDeleteFunctionTable(runtime);VirtualFree(memory,0,MEM_RELEASE);return false;}
        unsigned char patch[7]{0xe8,0,0,0,0,0x90,0x90};const auto rel=std::int32_t(displacement);std::memcpy(patch+1,&rel,4);
        if(!VirtualProtect(site,7,PAGE_EXECUTE_READWRITE,&prior)){RtlDeleteFunctionTable(runtime);VirtualFree(memory,0,MEM_RELEASE);return false;}
        std::memcpy(site,patch,7);DWORD unused{};const bool protection_ok=VirtualProtect(site,7,prior,&unused)!=0;
        FlushInstructionCache(GetCurrentProcess(),site,7);FlushInstructionCache(GetCurrentProcess(),memory,code.size());
        stub_allocation=memory;installed_base.store(address,std::memory_order_release);
        log(protection_ok?"HOOK_INSTALLED rva=0x20521 native CPU vertices only; FP8 proxy file unchanged":"HOOK_INSTALLED protection restore failed");
        return true;
    }catch(...){log("REJECTED installation exception");return false;}
}
}
Scope::Scope(void* base,void* swapchain) noexcept {
    previous_=current;
    if(!base||!swapchain||!install(base))return;
    DXGI_SWAP_CHAIN_DESC desc{};
    if(FAILED(static_cast<IDXGISwapChain*>(swapchain)->GetDesc(&desc)))return;
    unsigned w=desc.BufferDesc.Width,h=desc.BufferDesc.Height;

    if(!w||!h){RECT rect{};if(GetClientRect(desc.OutputWindow,&rect)){w=unsigned(std::max(0L,rect.right-rect.left));h=unsigned(std::max(0L,rect.bottom-rect.top));}}
    if(w<320||h<300)return;
    context_.base=base;context_.width=w;context_.height=h;
    context_.counters=nrmenufps::observe(base,swapchain,w,h);current=&context_;
}
Scope::~Scope(){current=static_cast<Context*>(previous_);}
}
