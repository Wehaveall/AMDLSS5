#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstddef>
#include <mutex>
namespace nrhip {
struct ApiRosto {
    HMODULE modulo{};
    int (*import_external_memory)(void**,const void*){};
    int (*external_memory_mapped_buffer)(void**,void*,const void*){};
    int (*memcpy_2d)(void*,std::size_t,const void*,std::size_t,std::size_t,std::size_t,int){};
    int (*memcpy_2d_async)(void*,std::size_t,const void*,std::size_t,std::size_t,std::size_t,int,void*){};
    int (*memcpy_async)(void*,const void*,std::size_t,int,void*){};
    int (*set_device)(int){};
    int (*get_device)(int*){};
    int (*stream_create_with_flags)(void**,unsigned){};
    int (*stream_synchronize)(void*){};
    int (*malloc_)(void**,std::size_t){};
    int (*free_)(void*){};
    int (*mem_get_info)(std::size_t*,std::size_t*){};
    int (*host_malloc)(void**,std::size_t,unsigned){};
    int (*event_create_with_flags)(void**,unsigned){};
    int (*event_record)(void*,void*){};
    int (*event_query)(void*){};
};
namespace detalhe_rosto {
template<class T> inline void pegar(HMODULE h,const char* nome,T& destino) noexcept {destino=reinterpret_cast<T>(GetProcAddress(h,nome));}
inline ApiRosto resolver_rosto(HMODULE h) noexcept {
    ApiRosto a{};a.modulo=h;if(!h)return a;
    pegar(h,"hipImportExternalMemory",a.import_external_memory);
    pegar(h,"hipExternalMemoryGetMappedBuffer",a.external_memory_mapped_buffer);
    pegar(h,"hipMemcpy2D",a.memcpy_2d);
    pegar(h,"hipMemcpy2DAsync",a.memcpy_2d_async);
    pegar(h,"hipMemcpyAsync",a.memcpy_async);
    pegar(h,"hipSetDevice",a.set_device);
    pegar(h,"hipGetDevice",a.get_device);
    pegar(h,"hipStreamCreateWithFlags",a.stream_create_with_flags);
    pegar(h,"hipStreamSynchronize",a.stream_synchronize);
    pegar(h,"hipMalloc",a.malloc_);
    pegar(h,"hipFree",a.free_);
    pegar(h,"hipMemGetInfo",a.mem_get_info);
    pegar(h,"hipHostMalloc",a.host_malloc);
    if(!a.host_malloc)pegar(h,"hipHostAlloc",a.host_malloc);
    pegar(h,"hipEventCreateWithFlags",a.event_create_with_flags);
    pegar(h,"hipEventRecord",a.event_record);
    pegar(h,"hipEventQuery",a.event_query);
    return a;
}
}
inline ApiRosto api_rosto() noexcept {
    static std::mutex m;static ApiRosto guardada{};
    std::lock_guard<std::mutex> trava(m);
    if(guardada.modulo)return guardada;
    const HMODULE h=GetModuleHandleW(L"amdhip64_7.dll");
    if(!h)return ApiRosto{};
    guardada=detalhe_rosto::resolver_rosto(h);
    return guardada;
}
}
