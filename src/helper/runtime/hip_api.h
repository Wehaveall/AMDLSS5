#pragma once

#include <windows.h>
#include <hip/hip_runtime_api.h>
#include <cstddef>
#include <cstring>
#include <mutex>
namespace nrhip {
using ModuleLaunch=int(*)(void*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,void*,void**,void**);
struct Api {
    HMODULE modulo{};
    int (*get_device_properties)(hipDeviceProp_t*,int){};
    int (*get_device)(int*){};
    int (*module_load_data)(void**,const void*){};
    int (*module_get_function)(void**,void*,const char*){};
    int (*get_func_by_symbol)(void**,const void*){};
    const char* (*kernel_name_ref)(void*){};
    ModuleLaunch module_launch_kernel{};
    int (*malloc_)(void**,std::size_t){};
    int (*free_)(void*){};

    int (*event_create)(void**){};
    int (*event_create_with_flags)(void**,unsigned){};
    int (*event_record)(void*,void*){};
    int (*event_query)(void*){};
    int (*event_synchronize)(void*){};
    int (*event_elapsed_time)(float*,void*,void*){};
    int (*event_destroy)(void*){};
    int (*host_malloc)(void**,std::size_t,unsigned){};
    int (*host_get_device_pointer)(void**,void*,unsigned){};
    int (*device_get_attribute)(int*,int,int){};
    int (*memcpy_)(void*,const void*,std::size_t,int){};
    int (*memcpy_async)(void*,const void*,std::size_t,int,void*){};
    int (*mem_get_address_range)(void**,std::size_t*,void*){};
    int (*module_get_global)(void**,std::size_t*,void*,const char*){};
    int (*device_synchronize)(){};
    int (*stream_synchronize)(void*){};
    int (*mem_ptr_get_info)(void*,std::size_t*){};
    int (*memset_async)(void*,int,std::size_t,void*){};
};
namespace detalhe {
template<class T> inline void pegar(HMODULE h,const char* nome,T& destino) noexcept {destino=reinterpret_cast<T>(GetProcAddress(h,nome));}
inline Api resolver(HMODULE h) noexcept {
    Api a{};a.modulo=h;if(!h)return a;
    pegar(h,"hipGetDevicePropertiesR0600",a.get_device_properties);
    pegar(h,"hipGetDevice",a.get_device);
    pegar(h,"hipModuleLoadData",a.module_load_data);
    pegar(h,"hipModuleGetFunction",a.module_get_function);
    pegar(h,"hipGetFuncBySymbol",a.get_func_by_symbol);
    pegar(h,"hipKernelNameRef",a.kernel_name_ref);
    pegar(h,"hipModuleLaunchKernel",a.module_launch_kernel);
    pegar(h,"hipMalloc",a.malloc_);
    pegar(h,"hipFree",a.free_);
    pegar(h,"hipEventCreate",a.event_create);pegar(h,"hipEventCreateWithFlags",a.event_create_with_flags);
    pegar(h,"hipEventRecord",a.event_record);pegar(h,"hipEventQuery",a.event_query);pegar(h,"hipEventSynchronize",a.event_synchronize);
    pegar(h,"hipEventElapsedTime",a.event_elapsed_time);pegar(h,"hipEventDestroy",a.event_destroy);
    pegar(h,"hipHostMalloc",a.host_malloc);pegar(h,"hipHostGetDevicePointer",a.host_get_device_pointer);
    pegar(h,"hipDeviceGetAttribute",a.device_get_attribute);pegar(h,"hipMemcpy",a.memcpy_);pegar(h,"hipMemcpyAsync",a.memcpy_async);
    pegar(h,"hipMemGetAddressRange",a.mem_get_address_range);pegar(h,"hipModuleGetGlobal",a.module_get_global);
    pegar(h,"hipDeviceSynchronize",a.device_synchronize);pegar(h,"hipStreamSynchronize",a.stream_synchronize);
    pegar(h,"hipMemPtrGetInfo",a.mem_ptr_get_info);
    pegar(h,"hipMemsetAsync",a.memset_async);
    return a;
}
}

inline bool dispositivo_ok(int(*get_device)(int*),int(*props)(hipDeviceProp_t*,int),int& device) noexcept {
    if(!get_device||!props)return false;
    hipDeviceProp_t p{};
    if(get_device(&device)||props(&p,device))return false;
    return !std::strncmp(p.gcnArchName,"gfx1201",7)&&(!p.gcnArchName[7]||p.gcnArchName[7]==':')&&p.warpSize==32;
}

inline Api api() noexcept {
    static std::mutex m;static Api guardada{};
    std::lock_guard<std::mutex> trava(m);
    if(guardada.modulo)return guardada;
    const HMODULE h=GetModuleHandleW(L"amdhip64_7.dll");
    if(!h)return Api{};
    guardada=detalhe::resolver(h);
    return guardada;
}
}
