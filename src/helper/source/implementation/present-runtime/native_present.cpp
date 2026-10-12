#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib,"bcrypt.lib")
#include "native_present.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace nrpresentnative {
bool validate(unsigned w,unsigned h,const Controls& c,std::string& error) noexcept {
    if(w<128||h<128||w>4096||h>4096||std::uint64_t(w)*h>8388608){error="native_extent_unsupported";return false;}
    if(!std::isfinite(c.structure)||c.structure<0||c.structure>2||
       !std::isfinite(c.skin)||(c.skin!=-1&&(c.skin<0||c.skin>2))||
       !std::isfinite(c.tone)||c.tone<0||c.tone>2||
       !std::isfinite(c.strength)||c.strength<0||c.strength>2||
       c.style>2||c.passes<1||c.passes>3){error="native_controls_unsupported";return false;}
    return true;
}
std::array<float,5> parameters(const Controls& c) noexcept {
    return {c.tone_channels?c.tone:0.f,c.tone_channels?c.structure:0.f,
        c.auto_mask?(c.skin<0?c.structure:c.skin):-1.f,c.auto_mask?c.structure:-1.f,.03125f*c.strength};
}
namespace {
using Eval=void(*)(void*,void*,int,int,int,int,void*,int,int,int,void*);
template<class T>T read(const void* p,size_t offset){T v{};std::memcpy(&v,static_cast<const char*>(p)+offset,sizeof(v));return v;}
template<class T>void put(void* p,size_t offset,T v){std::memcpy(static_cast<char*>(p)+offset,&v,sizeof(v));}
void check(int result,const char* what){if(result)throw std::runtime_error(what);}
struct alignas(8) Properties {char name[256],uuid[16],luid[8];unsigned node_mask,padding;unsigned char remainder[8192];};
static_assert(offsetof(Properties,luid)==272);
bool hash_matches(void*,ULONG,const char*);
struct Hip {
    HMODULE module{};
    int(*count)(int*){};int(*properties)(Properties*,int){};int(*set_device)(int){};int(*get_device)(int*){};
    int(*allocate)(void**,size_t){};int(*release)(void*){};int(*copy)(void*,const void*,size_t,int){};
    int(*sync)(){};int(*error)(){};
    template<class T>T symbol(const char* name){auto p=GetProcAddress(module,name);if(!p)throw std::runtime_error("native_hip_symbol_missing");return reinterpret_cast<T>(p);}
    void initialize(char* proxy){
        module=GetModuleHandleW(L"amdhip64_7.dll");if(!module)throw std::runtime_error("native_hip_not_loaded");
        wchar_t path[32768]{},proxy_path[32768]{};
        if(!GetModuleFileNameW(module,path,32768)||!GetModuleFileNameW(reinterpret_cast<HMODULE>(proxy),proxy_path,32768)||
           !std::filesystem::equivalent(path,std::filesystem::path(proxy_path).parent_path()/L"amdhip64_7.dll"))throw std::runtime_error("native_hip_path_mismatch");
        std::ifstream file(std::filesystem::path(path),std::ios::binary);
        std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),{});
        if(bytes.empty()||bytes.size()>128ull*1024*1024||!hash_matches(bytes.data(),static_cast<ULONG>(bytes.size()),"546fb3d6e2d2194a9526fb94ec2fd3aa5b92a48a7595f04efece80162047ef69"))throw std::runtime_error("native_hip_3581_hash_mismatch");
        count=symbol<decltype(count)>("hipGetDeviceCount");properties=symbol<decltype(properties)>("hipGetDevicePropertiesR0600");
        set_device=symbol<decltype(set_device)>("hipSetDevice");get_device=symbol<decltype(get_device)>("hipGetDevice");
        allocate=symbol<decltype(allocate)>("hipMalloc");release=symbol<decltype(release)>("hipFree");
        copy=symbol<decltype(copy)>("hipMemcpy");sync=symbol<decltype(sync)>("hipDeviceSynchronize");error=symbol<decltype(error)>("hipGetLastError");
    }
    void completed(){check(sync(),"native_hip_sync_failed");check(error(),"native_hip_error");}
};
bool hash_matches(void* bytes,ULONG size,const char* expected){
    BCRYPT_ALG_HANDLE algorithm{};if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
    unsigned char hash[32]{};const auto status=BCryptHash(algorithm,nullptr,0,static_cast<PUCHAR>(bytes),size,hash,32);
    BCryptCloseAlgorithmProvider(algorithm,0);if(status<0)return false;
    constexpr char hex[]="0123456789abcdef";
    for(unsigned i=0;i<32;++i)if(expected[i*2]!=hex[hash[i]>>4]||expected[i*2+1]!=hex[hash[i]&15])return false;
    return true;
}
bool verified(char* base){
    if(!base||read<WORD>(base,0)!=IMAGE_DOS_SIGNATURE)return false;
    const auto pe=read<LONG>(base,0x3c);if(pe<0||pe>4096)return false;
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+pe);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||nt->OptionalHeader.SizeOfImage<0x71d0a8)return false;
    return read<unsigned>(base,0x71d08c)==0x53545931&&read<unsigned>(base,0x71d098)==0x3153504d&&
      hash_matches(base+0x1ce20,0x1cfac-0x1ce20,"accab61b5ba0e5409d02955a039b106851f001b6725f5670316a4ff9cfe79ef4")&&
      hash_matches(base+0x1e120,0x1eac6-0x1e120,"3237a600040d3ed157e681c6cce0eaeafc8a1ce1a5d96f5227f8b5b63bb77127")&&
      hash_matches(base+0x2a080,0x2a9ef-0x2a080,"1f4bb782606e508ba7c825396a9bd8edf455f4ea74b23db7a7553d9f49062e2a");
}
float half_float(std::uint16_t h){const unsigned sign=unsigned(h&0x8000u)<<16,exp=(h>>10)&31,frac=h&1023;
    if(!exp)return std::copysign(std::ldexp(float(frac),-24),sign?-1.f:1.f);
    const unsigned bits=sign|(exp==31?0x7f800000u:(exp+112)<<23)|(frac<<13);float f;std::memcpy(&f,&bits,4);return f;}

std::uint16_t checked_half(std::uint16_t h){if(!std::isfinite(half_float(h)))throw std::runtime_error("native_nonfinite_output");
    return (h&0x8000)?0:std::min<std::uint16_t>(h,0x3c00);}
}
struct Engine::Impl {
    std::mutex mutex;std::atomic<std::uint64_t> requested_luid{};
    char* base{};Hip hip;int device=-1;std::uint64_t luid{};bool poisoned{};
    #pragma warning(push)
    #pragma warning(disable:4324)
    struct alignas(64) Context {unsigned char bytes[0x400]{};bool constructed{},loaded{};};
    #pragma warning(pop)
    std::array<Context,3> contexts;
    void* input{};void* output{};size_t capacity{};
    std::vector<std::uint16_t> staging;
    void initialize(char* requested){
        if(base&&base!=requested)throw std::runtime_error("native_proxy_changed");
        if(base)return;
        if(!verified(requested))throw std::runtime_error("native_proxy_fingerprint_mismatch");
        HMODULE pinned{};if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(requested),&pinned))throw std::runtime_error("native_proxy_pin_failed");
        hip.initialize(requested);int count{};check(hip.count(&count),"native_hip_count_failed");
        luid=requested_luid.load();if(!luid)throw std::runtime_error("native_adapter_luid_missing");
        for(int d=0;d<count;++d){Properties p{};check(hip.properties(&p,d),"native_hip_properties_failed");if(read<std::uint64_t>(p.luid,0)==luid){device=d;break;}}
        if(device<0)throw std::runtime_error("native_hip_adapter_mismatch");base=requested;
    }
    void* context(unsigned index,const Controls& c){
        auto& slot=contexts[index];auto p=slot.bytes;
        if(!slot.constructed){reinterpret_cast<void*(*)(void*)>(base+0x1ce20)(p);slot.constructed=true;}
        if(!slot.loaded){wchar_t path[32768]{};if(!GetModuleFileNameW(reinterpret_cast<HMODULE>(base),path,32768))throw std::runtime_error("native_proxy_path_failed");
            std::string file=(std::filesystem::path(path).parent_path()/L"dlssnr_on_amd_weights.bin").string();
            if(!reinterpret_cast<bool(*)(void*,const std::string*)>(base+0x1e120)(p,&file))throw std::runtime_error("native_weight_load_failed");
            hip.completed();if(!read<void*>(p,0x10)||!read<void*>(p,0x168)||!read<size_t>(p,0x160))throw std::runtime_error("native_weights_unbound");slot.loaded=true;}
        const auto controls=parameters(c);std::memcpy(p+0x20,controls.data(),20);
        put<unsigned>(p,0xf0,0);put<unsigned>(p,0x120,0);put<unsigned>(p,0x38,0);put<void*>(p,0x118,nullptr);
        return p;
    }
};
Engine::Engine():impl_(new Impl){}

Engine::~Engine(){(void)impl_.release();}
void Engine::set_adapter_luid(std::uint64_t luid) noexcept {impl_->requested_luid.store(luid);}
bool Engine::evaluate(void* proxy,const std::uint16_t* rgba,std::uint16_t* out,unsigned w,unsigned h,const Controls& c,std::string& error) noexcept {
    if(!validate(w,h,c,error))return false;
    if(!proxy||!rgba||!out){error="native_argument_missing";return false;}
    auto& p=*impl_;std::lock_guard<std::mutex> guard(p.mutex);
    if(p.poisoned){error="native_failed_sticky";return false;}
    try {
        p.initialize(static_cast<char*>(proxy));
        if(p.requested_luid.load()!=p.luid)throw std::runtime_error("native_adapter_changed");
        int previous=-1;check(p.hip.get_device(&previous),"native_hip_get_device_failed");check(p.hip.set_device(p.device),"native_hip_set_device_failed");
        struct Restore {Hip& hip;int previous;~Restore(){if(previous>=0)(void)hip.set_device(previous);}} restore{p.hip,previous};
        const size_t elements=size_t(w)*h*4,bytes=elements*sizeof(std::uint16_t);

        for(size_t i=0;i<elements;++i)if(i%4!=3&&(!std::isfinite(half_float(rgba[i]))||half_float(rgba[i])<0||half_float(rgba[i])>1))throw std::runtime_error("native_input_not_sdr");
        p.staging.assign(rgba,rgba+elements);
        if(c.strength==0){std::copy(p.staging.begin(),p.staging.end(),out);error="native_strength_zero";return true;}
        if(p.capacity<bytes){p.hip.completed();void* first{},*second{};check(p.hip.allocate(&first,bytes),"native_input_alloc_failed");
            if(p.hip.allocate(&second,bytes)){(void)p.hip.release(first);throw std::runtime_error("native_output_alloc_failed");}
            if(p.input)check(p.hip.release(p.input),"native_input_free_failed");if(p.output)check(p.hip.release(p.output),"native_output_free_failed");
            p.input=first;p.output=second;p.capacity=bytes;}
        put<float>(p.base,0x71d088,float(c.style)/128.f);
        for(unsigned pass=0;pass<c.passes;++pass){
            auto context=p.context(pass,c);check(p.hip.copy(p.input,p.staging.data(),bytes,1),"native_input_copy_failed");

            std::vector<std::uint16_t> poison(elements,0x7e00);check(p.hip.copy(p.output,poison.data(),bytes,1),"native_output_prime_failed");
            reinterpret_cast<Eval>(p.base+0x2a080)(context,p.input,int(w*8),0,int(h),int(w),p.output,int(w*8),0,0,nullptr);
            p.hip.completed();check(p.hip.copy(p.staging.data(),p.output,bytes,2),"native_output_copy_failed");
            for(size_t i=0;i<elements;++i)p.staging[i]=i%4==3?rgba[i]:checked_half(p.staging[i]);
        }
        std::copy(p.staging.begin(),p.staging.end(),out);error="native_completed_spatial_full_color";return true;
    }catch(const std::exception& e){p.poisoned=true;error=e.what();return false;}catch(...){p.poisoned=true;error="native_exception";return false;}
}
}
