#pragma once
#include "pasta_helper.h"
#include "nr_publico.h"
#include "../../nr-style-performance/measurement-ab/meter.h"
#include <windows.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <numeric>
#include <vector>
#include "sha256.h"
#include "reg_swin32.h"
#include "hip_api.h"
#include "modulos_pin.h"

namespace nrmeasurelive {
struct Dim {unsigned x,y,z;};
using Launch=int(*)(const void*,Dim,Dim,void**,size_t,void*);
using Clock=std::chrono::steady_clock;
inline Launch original{};
inline nrmeasure::Api api{};
inline int(*create_event)(void**){};
using ModuleLaunch=int(*)(void*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,void*,void**,void**);
inline ModuleLaunch marker_launch{};
inline bool clock_enabled=false;
inline char* base{};
inline std::atomic<bool> enabled{false},done{false};
inline std::filesystem::path folder;
inline std::atomic<DWORD> owner{};
struct Signature {const void* f{};Dim grid{},block{};size_t shared{};void* stream{};};
struct Row {unsigned block{},sample{},counter{};int width{},height{};bool metadata{};double cpu{},period{};float gpu{},group{};unsigned target{};};
struct State {
 nrmeasure::Meter meter{api};

 std::array<std::array<void*,4>,4000> event_pool{};
 std::array<Signature,512> signature{};
 std::array<Row,400> rows{};
 unsigned step{},ordinal{},saved{},counter{},expected{},pre{},post{};int width{},height{};bool metadata{},active{},pending{},valid=true,calibrated{},started{};
 Clock::time_point start{},poll{};
 Row row{};
 void* clock_module{};void* clock_kernel{};void* clock_device{};unsigned long long* clock_host{};int clock_rate{};
 nrmeasure::Mode mode=nrmeasure::Mode::Off;unsigned first{},last{};
};
inline State* state{};
inline const char* names[]={"off","total","pre","post","body","body","post","pre","total","off"};
inline void message(const char* value) noexcept {
(void)value;
}
inline void flush() noexcept;
inline void abort_measurement(const char* why) noexcept {done=true;if(state){state->active=false;if(state->saved)flush();}message(why);}

inline void flush() noexcept {
 if(state)state->saved=0;
}
inline void stamp(unsigned slot,void* stream) noexcept {
 auto& s=*state;unsigned index=(s.row.block*400+s.row.sample)*4+slot;void* args[]={&s.clock_device,&index};
 if(marker_launch(s.clock_kernel,1,1,1,1,1,1,0,stream,args,nullptr))s.valid=false;
}

inline void sha256_hex(const std::vector<unsigned char>& b,char (&out)[65]) noexcept {nrsha::hex(b.data(),b.size(),out);}
inline bool initialize_clock() noexcept {

 auto load=nrhip::api().module_load_data;
 auto function=nrhip::api().module_get_function;
 auto host=reinterpret_cast<int(*)(void**,size_t,unsigned)>(nrhip::api().host_malloc);
 auto mapped=reinterpret_cast<int(*)(void**,void*,unsigned)>(nrhip::api().host_get_device_pointer);
 auto attribute=reinterpret_cast<int(*)(int*,int,int)>(nrhip::api().device_get_attribute);
 marker_launch=nrhip::api().module_launch_kernel;
 if(!load||!function||!host||!mapped||!attribute||!marker_launch)return false;
 auto& s=*state;
 static std::vector<unsigned char> imagem;
 if(nrmodpin::ler(folder/L"nr_measure_stamp.hsaco",nullptr,nullptr,imagem)!=nrmodpin::Leitura::ok){message("CLOCK_BACKEND REJECTED nr_measure_stamp.hsaco ausente");return false;}
 {char sha[65];sha256_hex(imagem,sha);char line[160];std::snprintf(line,sizeof line,"CLOCK_MODULE nr_measure_stamp.hsaco bytes=%zu sha256=%s (sem pin: nao vai em pacote)",imagem.size(),sha);message(line);}
 if(load(&s.clock_module,imagem.data())||function(&s.clock_kernel,s.clock_module,"nr_measure_stamp"))return false;
 constexpr size_t bytes=4000*4*sizeof(unsigned long long);

 if(host(reinterpret_cast<void**>(&s.clock_host),bytes,0x40000002u)||mapped(&s.clock_device,s.clock_host,0))return false;
 std::memset(s.clock_host,0,bytes);

 if(attribute(&s.clock_rate,10017,0)||s.clock_rate<=0)return false;
 message("CLOCK_BACKEND coherent timestamps, GET_REALTIME, no HIP events or per-frame readbacks");return true;
}
inline int launch(const void* f,Dim g,Dim b,void** args,size_t shared,void* stream) {
 if(GetCurrentThreadId()!=owner.load())return original(f,g,b,args,shared,stream);
 auto* s=state;
 if(!s||!s->active)return original(f,g,b,args,shared,stream);
 const unsigned index=s->ordinal++;
 if(index>=s->signature.size()){s->valid=false;return original(f,g,b,args,shared,stream);}
 const Signature item{f,g,b,shared,stream};
 if(!s->calibrated)s->signature[index]=item;
 else {const auto& old=s->signature[index];if(old.f!=f||std::memcmp(&old.grid,&g,sizeof(g))||std::memcmp(&old.block,&b,sizeof(b))||old.shared!=shared||old.stream!=stream)s->valid=false;}
 if(clock_enabled){if(s->mode!=nrmeasure::Mode::Off&&index==1)stamp(0,stream);if(s->mode==nrmeasure::Mode::Group&&index==s->first)stamp(2,stream);}
 else s->meter.before(index,stream);
 const int result=original(f,g,b,args,shared,stream);
 if(clock_enabled){if(s->mode==nrmeasure::Mode::Group&&index==s->last)stamp(3,stream);if(s->mode!=nrmeasure::Mode::Off&&index+1==s->expected)stamp(1,stream);}
 else s->meter.after(index,stream);
 if(result)s->valid=false;
 return result;
}
inline bool install(char* image) noexcept {
(void)image;return true;
}
inline void begin(char* image,void* context,int width,int height,void* metadata) noexcept {
(void)image;(void)context;(void)width;(void)height;(void)metadata;
}
inline void end() noexcept {
}
inline void abandon() noexcept {
}
}
