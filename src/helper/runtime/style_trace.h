#pragma once
#include "hip_api.h"
#include "pasta_helper.h"
#include "nr_publico.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <array>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include "kernel_trace.h"
#include "reg_swin32.h"
namespace nrstyletrace {
struct Dim {unsigned x,y,z;};
using Launch=int(*)(const void*,Dim,Dim,void**,size_t,void*);
inline Launch original=nullptr;
inline char* base=nullptr;
inline std::filesystem::path folder;
inline std::wstring ini_proxy;
inline bool enabled=false,style_enabled=false,kernel_enabled=false,contract_enabled=false,conv2_launch_enabled=false;
inline std::unique_ptr<nrkerneltrace::Recorder> kernel_recorder;
inline std::mutex kernel_mutex;
inline unsigned kernel_job=0,kernel_job_limit=3;
inline unsigned contract_count=0;

inline unsigned conv2_launch_count=0,conv2_launch_limit=96;
struct ContractPointers {unsigned id{};std::array<std::uintptr_t,4> value{};};
inline std::vector<ContractPointers> contract_pointers;
struct EventApi {using Create=int(*)(void**);using Record=int(*)(void*,void*);using Sync=int(*)(void*);using Elapsed=int(*)(float*,void*,void*);using Destroy=int(*)(void*);Create create{};Record record{};Sync sync{};Elapsed elapsed{};Destroy destroy{};bool ready()const noexcept{return create&&record&&sync&&elapsed&&destroy;}};
struct TimedLaunch {nrkerneltrace::Launch launch{};int job{};void* begin{};void* end{};};
inline EventApi events;
inline thread_local bool armed=false,observed=false,kernel_armed=false;
inline thread_local std::vector<TimedLaunch> timed;
inline thread_local unsigned requested=0,effective=0;
inline unsigned last=~0u,count=0;
template<class T>T get(const void* p,size_t o){T v;std::memcpy(&v,static_cast<const char*>(p)+o,sizeof(v));return v;}
inline std::uint64_t tick() noexcept {LARGE_INTEGER value{};QueryPerformanceCounter(&value);return static_cast<std::uint64_t>(value.QuadPart);}
inline void write_kernel_receipt(std::vector<nrkerneltrace::Record> records) noexcept {
    if(records.empty())return;
    try {FILE* file=nullptr;if(_wfopen_s(&file,(folder/L"nr_kernel_trace.log").c_str(),L"a")!=0||!file)return;
        for(const auto& record:records)std::fprintf(file,"pid=%lu job=%d function=%p grid=%u,%u,%u block=%u,%u,%u shared=%zu cpu_begin=%llu cpu_end=%llu\n",
            GetCurrentProcessId(),record.job,reinterpret_cast<void*>(record.function),record.grid_x,record.grid_y,record.grid_z,record.block_x,record.block_y,record.block_z,record.shared_bytes,
            static_cast<unsigned long long>(record.begin_tick),static_cast<unsigned long long>(record.end_tick));
        std::fclose(file);
    }catch(...){}
}
inline void write_gpu_timing() noexcept {
    if(timed.empty()||!events.ready())return;
    try {FILE* file=nullptr;if(_wfopen_s(&file,(folder/L"nr_kernel_timing.log").c_str(),L"a")!=0||!file)return;
        for(const auto& item:timed){float ms=-1.0f;if(!events.sync(item.end))events.elapsed(&ms,item.begin,item.end);std::fprintf(file,"pid=%lu job=%d function=%p grid=%u,%u,%u block=%u,%u,%u gpu_ms=%.6f\n",GetCurrentProcessId(),item.job,reinterpret_cast<void*>(item.launch.function),item.launch.grid_x,item.launch.grid_y,item.launch.grid_z,item.launch.block_x,item.launch.block_y,item.launch.block_z,ms);events.destroy(item.begin);events.destroy(item.end);}std::fclose(file);
    }catch(...){} timed.clear();
}
inline void capture_contract_args(const void* f,Dim g,Dim b,void** args,size_t shared) noexcept {

    if(!contract_enabled||f!=base+0x621e0||!args||!args[0])return;
    const bool single=NRCFG_INT(L"DlssNrOnAmd",L"NeuralContractTraceSingle",0,ini_proxy.c_str())==1;
    if(contract_count>=(single?1u:16u))return;
    try {std::array<unsigned char,304> bytes{};std::memcpy(bytes.data(),args[0],bytes.size());const unsigned id=contract_count++;const ContractPointers pointers{id,{get<std::uintptr_t>(bytes.data(),0),get<std::uintptr_t>(bytes.data(),8),get<std::uintptr_t>(bytes.data(),16),get<std::uintptr_t>(bytes.data(),24)}};contract_pointers.push_back(pointers);auto info=reinterpret_cast<int(*)(void*,size_t*)>(nrhip::api().mem_ptr_get_info);FILE* meta=nullptr;if(info&&_wfopen_s(&meta,(folder/L"nr_contract2_ranges.log").c_str(),L"a")==0&&meta){std::fprintf(meta,"id=%u",id);for(auto pointer:pointers.value){size_t size=0;std::fprintf(meta," ptr=%p bytes=%zu",reinterpret_cast<void*>(pointer),info(reinterpret_cast<void*>(pointer),&size)?0:size);}std::fputc('\n',meta);std::fclose(meta);}FILE* file=nullptr;if(_wfopen_s(&file,(folder/(L"nr_contract2_"+std::to_wstring(GetCurrentProcessId())+L"_"+std::to_wstring(id)+L".bin")).c_str(),L"wb")!=0||!file)return;std::fwrite(&g,sizeof(g),1,file);std::fwrite(&b,sizeof(b),1,file);std::fwrite(&shared,sizeof(shared),1,file);std::fwrite(bytes.data(),1,bytes.size(),file);std::fclose(file);}catch(...){}
}
inline void capture_contract_buffers() noexcept {

    if(contract_pointers.empty()||!NRCFG_INT(L"DlssNrOnAmd",L"NeuralContractTraceCopy",0,ini_proxy.c_str()))return;
    try {auto sync=reinterpret_cast<int(*)()>(nrhip::api().device_synchronize);auto copy=reinterpret_cast<int(*)(void*,const void*,size_t,int)>(nrhip::api().memcpy_);if(!sync||!copy||sync())return;for(const auto& sample:contract_pointers)for(unsigned i=0;i<4;++i){std::array<unsigned char,256> bytes{};if(!sample.value[i]||copy(bytes.data(),reinterpret_cast<const void*>(sample.value[i]),bytes.size(),2))continue;FILE* file=nullptr;if(_wfopen_s(&file,(folder/(L"nr_contract2_"+std::to_wstring(GetCurrentProcessId())+L"_"+std::to_wstring(sample.id)+L"_p"+std::to_wstring(i)+L".bin")).c_str(),L"wb")!=0||!file)continue;std::fwrite(bytes.data(),1,bytes.size(),file);std::fclose(file);}}catch(...){}
}
inline void capture_contract_buffers_now(const wchar_t* phase) noexcept {
    if(contract_pointers.empty()||!NRCFG_INT(L"DlssNrOnAmd",L"NeuralContractTraceImmediate",0,ini_proxy.c_str()))return;
    try {const bool full=NRCFG_INT(L"DlssNrOnAmd",L"NeuralContractTraceFullTensors",0,ini_proxy.c_str())==1;const bool single=NRCFG_INT(L"DlssNrOnAmd",L"NeuralContractTraceSingle",0,ini_proxy.c_str())==1;const auto& sample=contract_pointers.back();if(single&&sample.id!=0)return;const unsigned byte_setting=NRCFG_INT(L"DlssNrOnAmd",L"NeuralContractTraceBytes",256,ini_proxy.c_str());const size_t byte_count=(byte_setting>0&&byte_setting<=1024*1024)?byte_setting:256;auto sync=reinterpret_cast<int(*)()>(nrhip::api().device_synchronize);auto copy=reinterpret_cast<int(*)(void*,const void*,size_t,int)>(nrhip::api().memcpy_);auto info=reinterpret_cast<int(*)(void*,size_t*)>(nrhip::api().mem_ptr_get_info);if(!sync||!copy||!info||sync())return;for(unsigned i=0;i<4;++i){const size_t requested_bytes=full?(i==0?1024*1024:i<3?262144:0):byte_count;size_t available=0;if(!requested_bytes||!sample.value[i]||info(reinterpret_cast<void*>(sample.value[i]),&available))continue;const size_t selected=(std::min)(requested_bytes,available);std::vector<unsigned char> bytes(selected);if(copy(bytes.data(),reinterpret_cast<const void*>(sample.value[i]),bytes.size(),2))continue;FILE* file=nullptr;if(_wfopen_s(&file,(folder/(L"nr_contract2_"+std::wstring(phase)+L"_"+std::to_wstring(GetCurrentProcessId())+L"_"+std::to_wstring(sample.id)+L"_p"+std::to_wstring(i)+L".bin")).c_str(),L"wb")!=0||!file)continue;std::fwrite(bytes.data(),1,bytes.size(),file);std::fclose(file);}}catch(...){}
}
inline void record_conv2_launch(const void* f,Dim g,Dim b,void** args,size_t shared) noexcept {
    if(!conv2_launch_enabled||!args||!args[0]||b.x!=256||b.y!=1||b.z!=1||conv2_launch_count>=conv2_launch_limit)return;

    try {
        std::array<unsigned char,64> bytes{};std::memcpy(bytes.data(),args[0],bytes.size());
        const auto input=get<std::uintptr_t>(bytes.data(),0);
        const auto residual=get<std::uintptr_t>(bytes.data(),8);
        const auto spatial_input=get<std::uintptr_t>(bytes.data(),16);
        const auto output=get<std::uintptr_t>(bytes.data(),24);
        const auto spatial_output=get<std::uintptr_t>(bytes.data(),32);
        const auto weights=get<std::uintptr_t>(bytes.data(),40);
        const auto spatial_a=get<unsigned>(bytes.data(),48),spatial_b=get<unsigned>(bytes.data(),52),tiles=get<unsigned>(bytes.data(),56);
        FILE* file=nullptr;if(_wfopen_s(&file,(folder/L"nr_conv2_launch_trace.log").c_str(),L"a")!=0||!file)return;
        std::fprintf(file,"pid=%lu id=%u function=%p grid=%u,%u,%u block=%u,%u,%u shared=%zu input=%p residual=%p spatial_input=%p output=%p spatial_output=%p weights=%p spatial_a=%u spatial_b=%u tiles=%u\n",
            GetCurrentProcessId(),conv2_launch_count++,const_cast<void*>(f),g.x,g.y,g.z,b.x,b.y,b.z,shared,reinterpret_cast<void*>(input),reinterpret_cast<void*>(residual),reinterpret_cast<void*>(spatial_input),reinterpret_cast<void*>(output),reinterpret_cast<void*>(spatial_output),reinterpret_cast<void*>(weights),spatial_a,spatial_b,tiles);
        std::fclose(file);
    }catch(...){ }
}
inline int launch(const void* f,Dim g,Dim b,void** args,size_t shared,void* stream) {
    try {if(kernel_armed&&kernel_recorder){std::lock_guard<std::mutex> lock(kernel_mutex);kernel_recorder->observe({reinterpret_cast<std::uintptr_t>(f),g.x,g.y,g.z,b.x,b.y,b.z,shared},tick());}
    if(style_enabled && armed && !observed && (f==base+0x62178 || f==base+0x68088) && args && args[0]) {
        observed=true;
        const bool slow=f==base+0x62178;auto p=args[0];
        FILE* file=nullptr;
        if(_wfopen_s(&file,(folder/L"nr_style_trace.log").c_str(),L"a")==0 && file) {
            std::fprintf(file,"pid=%lu requested=%u effective=%u kernel=%s style=%.9f tone=%.6f structure=%.6f skin=%.6f mask=%.6f\n",
                GetCurrentProcessId(),requested,effective,slow?"62178":"68088",get<float>(p,slow?0x30:0x5c),get<float>(p,slow?0x28:0x54),
                get<float>(p,slow?0x2c:0x58),get<float>(p,slow?0x48:0x60),get<float>(p,slow?0x4c:0x64));std::fclose(file);
        }
    }
    }catch(...){}
    record_conv2_launch(f,g,b,args,shared);
    capture_contract_args(f,g,b,args,shared);if(contract_enabled&&f==base+0x621e0)capture_contract_buffers_now(L"pre");TimedLaunch timing{};
    if(kernel_armed&&events.ready()&&events.create(&timing.begin)==0&&events.create(&timing.end)==0){timing.launch={reinterpret_cast<std::uintptr_t>(f),g.x,g.y,g.z,b.x,b.y,b.z,shared};timing.job=static_cast<int>(kernel_job);events.record(timing.begin,stream);}
    const auto result=original(f,g,b,args,shared,stream);
    if(contract_enabled&&f==base+0x621e0)capture_contract_buffers_now(L"post");
    if(timing.begin&&events.record(timing.end,stream)==0)timed.push_back(timing);else {if(timing.begin)events.destroy(timing.begin);if(timing.end)events.destroy(timing.end);}
    return result;
}
inline bool install(char* image) noexcept {
(void)image;return true;
}
inline void begin(char* image) noexcept {
(void)image;
}
inline void abandon() noexcept {
}
inline void capture(void* context,void* output,int pitch,int format,int width,int height) noexcept {
(void)context;(void)output;(void)pitch;(void)format;(void)width;(void)height;
}
}
