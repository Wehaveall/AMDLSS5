#pragma once
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
#include "reg_swin32.h"

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
 FILE* f=nullptr;if(_wfopen_s(&f,(folder/L"nr_measure_live.log").c_str(),L"a")==0&&f){std::fprintf(f,"pid=%lu %s\n",GetCurrentProcessId(),value);std::fclose(f);}
}
inline void flush() noexcept;
inline void abort_measurement(const char* why) noexcept {done=true;if(state){state->active=false;if(state->saved)flush();}message(why);}
inline void flush() noexcept {
 FILE* f=nullptr;if(_wfopen_s(&f,(folder/L"nr_measure_live.log").c_str(),L"a")!=0||!f){
done=true;return;
}
 for(unsigned i=0;i<state->saved;++i){const auto& r=state->rows[i];std::fprintf(f,"pid=%lu block=%u arm=%s sample=%u counter=%u width=%d height=%d metadata=%d eval_cpu_ms=%.6f period_ms=%.6f gpu_ms=%.6f group_ms=%.6f ordinal=%u\n",GetCurrentProcessId(),r.block,names[r.block],r.sample,r.counter,r.width,r.height,r.metadata?1:0,r.cpu,r.period,r.gpu,r.group,r.target);}
 std::fclose(f);state->saved=0;
}
inline void stamp(unsigned slot,void* stream) noexcept {
 auto& s=*state;unsigned index=(s.row.block*400+s.row.sample)*4+slot;void* args[]={&s.clock_device,&index};
 if(marker_launch(s.clock_kernel,1,1,1,1,1,1,0,stream,args,nullptr))s.valid=false;
}
inline bool initialize_clock() noexcept {
 auto hip=GetModuleHandleW(L"amdhip64_7.dll");auto load=reinterpret_cast<int(*)(void**,const char*)>(GetProcAddress(hip,"hipModuleLoad"));
 auto function=reinterpret_cast<int(*)(void**,void*,const char*)>(GetProcAddress(hip,"hipModuleGetFunction"));
 auto host=reinterpret_cast<int(*)(void**,size_t,unsigned)>(GetProcAddress(hip,"hipHostMalloc"));
 auto mapped=reinterpret_cast<int(*)(void**,void*,unsigned)>(GetProcAddress(hip,"hipHostGetDevicePointer"));
 auto attribute=reinterpret_cast<int(*)(int*,int,int)>(GetProcAddress(hip,"hipDeviceGetAttribute"));
 marker_launch=reinterpret_cast<ModuleLaunch>(GetProcAddress(hip,"hipModuleLaunchKernel"));
 if(!load||!function||!host||!mapped||!attribute||!marker_launch)return false;
 auto& s=*state;const auto file=(folder/L"nr_measure_stamp.hsaco").string();
 if(load(&s.clock_module,file.c_str())||function(&s.clock_kernel,s.clock_module,"nr_measure_stamp"))return false;
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
 try {
  if(enabled)return base==image;
  wchar_t path[32768]{};if(!GetModuleFileNameW(reinterpret_cast<HMODULE>(image),path,32768))return false;
  folder=std::filesystem::path(path).parent_path();const auto ini=folder/L"dlssnr_on_amd.ini";
  if(NRCFG_INT(L"DlssNrOnAmd",L"NeuralMeasurement",0,ini.c_str())!=1)return true;
  clock_enabled=NRCFG_INT(L"DlssNrOnAmd",L"NeuralMeasurementClock",0,ini.c_str())==1;
  if(NRCFG_INT(L"DlssNrOnAmd",L"NeuralMeasurementFamily",0,ini.c_str())==1){
   if(!clock_enabled){message("REJECTED family profiling requires hardware clock");return false;}
   for(unsigned i=2;i<8;++i)names[i]="family";
  }
  for(const auto key:{L"NeuralStyleDiagnostic",L"NeuralKernelTrace",L"NeuralContractTrace",L"NeuralConv2LaunchTrace"})
   if(NRCFG_INT(L"DlssNrOnAmd",key,0,ini.c_str())){message("REJECTED conflicting diagnostics");return false;}
  auto hip=GetModuleHandleW(L"amdhip64_7.dll");if(!hip)return false;
  original=reinterpret_cast<Launch>(GetProcAddress(hip,"hipLaunchKernel"));
  api={reinterpret_cast<decltype(api.record)>(GetProcAddress(hip,"hipEventRecord")),reinterpret_cast<decltype(api.query)>(GetProcAddress(hip,"hipEventQuery")),reinterpret_cast<decltype(api.elapsed)>(GetProcAddress(hip,"hipEventElapsedTime"))};
  create_event=reinterpret_cast<decltype(create_event)>(GetProcAddress(hip,"hipEventCreate"));
  auto slot=reinterpret_cast<void**>(image+0x86bd8);
  if(!original||!api.record||!api.query||!api.elapsed||!create_event||!nrregswin::known_launch(*slot,reinterpret_cast<void*>(original)))return false;
  original=reinterpret_cast<Launch>(*slot);
  DWORD old{},unused{};if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old))return false;
  base=image;enabled=true;
  InterlockedExchangePointer(reinterpret_cast<void* volatile*>(slot),reinterpret_cast<void*>(&launch));
  VirtualProtect(slot,sizeof(void*),old,&unused);nrregswin::register_hook(reinterpret_cast<void*>(&launch));message("INSTALLED waiting for nr_measure_start.flag");return true;
 }catch(...){done=true;return false;}
}
inline void begin(char* image,void* context,int width,int height,void* metadata) noexcept {
 if(!enabled||done||base!=image)return;
 try {
  DWORD unowned=0;owner.compare_exchange_strong(unowned,GetCurrentThreadId());
  if(GetCurrentThreadId()!=owner.load())return;
  if(!state){state=new State;if(clock_enabled){if(!initialize_clock()){abort_measurement("ABORT clock initialization");return;}}
   else for(auto& set:state->event_pool)for(auto& e:set)if(create_event(&e)){abort_measurement("ABORT event allocation");return;}}
  auto& s=*state;const auto now=Clock::now();
  if(!s.started){
   if(now-s.poll<std::chrono::seconds(1))return;s.poll=now;
   if(!std::filesystem::exists(folder/L"nr_measure_start.flag"))return;
   s.started=true;s.width=width;s.height=height;s.metadata=metadata!=nullptr;message(!std::strcmp(names[2],"family")?"START family profile: off,total,6x400 sampled ordinals,total,off; warmup60 each":"START ten alternating blocks, warmup60 samples400 each");
  }
  if(s.width!=width||s.height!=height||s.metadata!=(metadata!=nullptr)){abort_measurement("ABORT native input shape or metadata contract changed");return;}
  if(s.pending){
   const unsigned previous=s.row.block;const auto mode=names[previous];
   if(std::strcmp(mode,"off")){
    nrmeasure::Result r;
    if(clock_enabled){const volatile auto* ticks=s.clock_host+(s.row.block*400+s.row.sample)*4;
     const auto start=ticks[0],end=ticks[1];r.phase="clock_value";
     if(start&&end>start){r.total_ms=static_cast<float>(double(end-start)/s.clock_rate);
      if(s.mode==nrmeasure::Mode::Group){const auto first=ticks[2],last=ticks[3];if(first&&last>first)r.group_ms=static_cast<float>(double(last-first)/s.clock_rate);}
      if(r.total_ms>0&&r.total_ms>s.row.cpu*.1&&(s.mode!=nrmeasure::Mode::Group||(r.group_ms>0&&r.group_ms<=r.total_ms+.001f)))r.status=nrmeasure::Status::Ready;}
    }else r=s.meter.read();
    if(r.status!=nrmeasure::Status::Ready){char detail[256]{};std::snprintf(detail,sizeof(detail),"ABORT HIP timing phase=%s status=%d gpu_ms=%.9f group_ms=%.9f; no wait added",r.phase,r.error,r.total_ms,r.group_ms);abort_measurement(detail);return;}
    s.row.gpu=r.total_ms;s.row.group=r.group_ms;
   }
   s.row.period=std::chrono::duration<double,std::milli>(now-s.start).count();s.rows[s.saved++]=s.row;s.pending=false;
   if(s.saved==400)flush();
  }
  if(s.step>=4600){done=true;message("COMPLETE samples=4000; no image readback performed");return;}
  const unsigned block=s.step/460,within=s.step%460;
  const char* arm=names[block];nrmeasure::Mode mode=nrmeasure::Mode::Off;unsigned first=s.pre,last=s.pre;
  if(s.calibrated&&within>=60){if(!std::strcmp(arm,"total"))mode=nrmeasure::Mode::Total;
   else if(std::strcmp(arm,"off")){mode=nrmeasure::Mode::Group;
    if(!std::strcmp(arm,"family")){
     const unsigned count=s.post-s.pre-1,position=(block-2)*400+within-60;
     unsigned stride=53;while(std::gcd(stride,count)!=1)++stride;
     const unsigned cycle=position/count,offset=position%count;
     first=last=s.pre+1+((cycle%2?count-1-offset:offset)*stride)%count;
    }else if(!std::strcmp(arm,"post"))first=last=s.post;else if(!std::strcmp(arm,"body")){first=s.pre+1;last=s.post-1;}}}
  if(mode!=nrmeasure::Mode::Off)s.meter.events=s.event_pool[block*400+within-60];
  s.mode=mode;s.first=first;s.last=last;
  unsigned counter{};std::memcpy(&counter,static_cast<char*>(context)+0x38,sizeof(counter));
  if(s.step&&counter!=s.counter+1){abort_measurement("ABORT native counter reset or job skipped");return;}s.counter=counter;
  s.row={block,within>=60?within-60:0,counter,width,height,metadata!=nullptr,0,0,0,0,mode==nrmeasure::Mode::Group?first:0};
  s.ordinal=0;s.valid=true;s.meter.begin(mode,first,last,s.calibrated?s.expected:512);s.active=true;s.start=Clock::now();
 }catch(...){abort_measurement("ABORT exception at begin");}
}
inline void end() noexcept {
 if(!state||!state->active||GetCurrentThreadId()!=owner)return;
 auto& s=*state;s.active=false;
 s.row.cpu=std::chrono::duration<double,std::milli>(Clock::now()-s.start).count();
 if(!s.calibrated){
  s.expected=s.ordinal;unsigned matches=0;
  FILE* f=nullptr;const bool opened=_wfopen_s(&f,(folder/L"nr_measure_live.log").c_str(),L"a")==0&&f;
  const auto hip=GetModuleHandleW(L"amdhip64_7.dll");
  const auto function=hip?reinterpret_cast<int(*)(void**,const void*)>(GetProcAddress(hip,"hipGetFuncBySymbol")):nullptr;
  const auto kernel_name=hip?reinterpret_cast<const char*(*)(void*)>(GetProcAddress(hip,"hipKernelNameRef")):nullptr;
  for(unsigned i=0;i<(std::min)(s.ordinal,512u);++i){const auto& item=s.signature[i];
   if(i>=1&&item.stream!=s.signature[1].stream)s.valid=false;
   if(item.f==base+0x68088){if(!matches)s.pre=i;s.post=i;++matches;}
   if(opened)std::fprintf(f,"pid=%lu CALIBRATE ordinal=%u token_rva=%llx grid=%u,%u,%u block=%u,%u,%u\n",GetCurrentProcessId(),i,static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(item.f)-reinterpret_cast<std::uintptr_t>(base)),item.grid.x,item.grid.y,item.grid.z,item.block.x,item.block.y,item.block.z);
   if(opened&&!std::strcmp(names[2],"family")){
    void* native_function{};const char* name=nullptr;
    if(function&&kernel_name&&function(&native_function,item.f)==0)name=kernel_name(native_function);
    std::fprintf(f,"pid=%lu KERNEL_NAME ordinal=%u name=%s\n",GetCurrentProcessId(),i,name?name:"unresolved");
   }
  }
  if(opened){std::fprintf(f,"pid=%lu CALIBRATION launches=%u swin32_calls=%u pre=%u post=%u\n",GetCurrentProcessId(),s.expected,matches,s.pre,s.post);std::fclose(f);}
  if(!s.valid||s.expected<3||s.expected>512||matches!=3||s.pre<1||s.post<=s.pre+1){abort_measurement("ABORT unrecognized native graph; calibration saved");return;}
  s.calibrated=true;return;
 }
 if(!s.valid||(clock_enabled?s.ordinal!=s.expected:!s.meter.finish(s.ordinal))){abort_measurement("ABORT native launch sequence changed");return;}
 s.pending=s.step%460>=60;++s.step;
}
inline void abandon() noexcept {
if(state&&state->active)abort_measurement("ABORT native evaluation did not return");
}
}
