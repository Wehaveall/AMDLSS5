#pragma once
#include "nr_publico.h"

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nrtimeline {

struct Row {std::uint64_t calls{};double ms{};double gap{};};
struct GapRow {std::uint64_t calls{};double ms{};};
struct ReportRow {double calls_per_job{};double ms_per_job{};};
struct Report {unsigned jobs{};double span_ms_per_job{};double kernel_ms_per_job{};std::map<std::string,ReportRow> rows;};
class Aggregate {
 std::map<std::string,Row> rows_;
 std::map<std::pair<std::string,std::string>,GapRow> gaps_;
 unsigned jobs_{};
 double span_{};
 double gap_{};
public:
 void add(const std::string& name,float ms){auto& r=rows_[name];++r.calls;r.ms+=static_cast<double>(ms);}

 void add_gap(const std::string& prev,const std::string& next,double ms){
  if(ms<0)ms=0;
  rows_[next].gap+=ms;
  auto& g=gaps_[{prev,next}];++g.calls;g.ms+=ms;gap_+=ms;
 }
 double gap_total() const {return gap_;}
 const std::map<std::pair<std::string,std::string>,GapRow>& gaps() const {return gaps_;}
 void add_span(double ms){span_+=ms;}
 void end_job(){++jobs_;}
 unsigned jobs() const {return jobs_;}
 double span_total() const {return span_;}
 const std::map<std::string,Row>& totals() const {return rows_;}
 void clear(){rows_.clear();gaps_.clear();jobs_=0;span_=0;gap_=0;}
 Report report() const {
  Report r{};r.jobs=jobs_;
  if(!jobs_)return r;
  for(const auto& [name,row]:rows_){
   const ReportRow x{static_cast<double>(row.calls)/jobs_,row.ms/jobs_};
   r.rows.emplace(name,x);r.kernel_ms_per_job+=x.ms_per_job;
  }
  r.span_ms_per_job=span_/jobs_;
  return r;
 }
};

enum class Close {none,boundary,fallback};
class JobSplitter {
 bool seen_{};
 unsigned since_{};
public:
 static constexpr unsigned fallback_launches=256;
 Close before(bool is_boundary) noexcept {
  if(is_boundary){seen_=true;const bool open=since_>0;since_=1;return open?Close::boundary:Close::none;}
  if(!seen_&&since_>=fallback_launches){since_=1;return Close::fallback;}
  ++since_;return Close::none;
 }
 bool seen() const noexcept {return seen_;}
};

inline constexpr char import_name[]="_Z8k_import12ImportParams";
inline constexpr char flag_wait_name[]="_Z11k_flag_waitPjjj";
class GapTracker {
 std::string prev_;
 bool has_{};
public:
 bool next(const std::string& name,std::string& prev){
  const bool start=name==import_name||name==flag_wait_name;
  const bool counts=has_&&!start;
  prev=prev_;prev_=name;has_=true;
  return counts;
 }
};

inline std::string format_window(const Aggregate& a,unsigned index,bool fallback) {
 std::uint64_t launches=0;double kernel=0;
 for(const auto& [name,row]:a.totals()){launches+=row.calls;kernel+=row.ms;}
 char line[256]{};
 std::snprintf(line,sizeof line,"WINDOW n=%u jobs=%u launches=%llu boundary=%s span_ms=%.6f kernel_ms=%.6f gap_ms=%.6f\n",index,a.jobs(),
  static_cast<unsigned long long>(launches),fallback?"sem_fronteira":"pre_block",a.span_total(),kernel,a.gap_total());
 std::string out=line;
 for(const auto& [name,row]:a.totals()){
  std::snprintf(line,sizeof line,"ROW calls=%llu ms=%.6f gap_ms=%.6f name=",static_cast<unsigned long long>(row.calls),row.ms,row.gap);
  out+=line;out+=name;out+='\n';
 }

 for(const auto& [pair,g]:a.gaps()){
  std::snprintf(line,sizeof line,"GAP calls=%llu ms=%.6f prev=",static_cast<unsigned long long>(g.calls),g.ms);
  out+=line;out+=pair.first;out+=" next=";out+=pair.second;out+='\n';
 }
 out+="END\n";
 return out;
}

inline constexpr char boundary_name[]="_Z21k_pre_block_1h_32_fp89PreParams";
inline constexpr unsigned window_jobs=64;
inline constexpr bool requested=false;

inline thread_local bool replaced=false;

inline thread_local bool flushed=false;

inline thread_local bool inside=false;
struct Mark {void* start{};};
class Recorder {
 using EventCreate=int(*)(void**);
 using EventCreateFlags=int(*)(void**,unsigned);
 using EventRecord=int(*)(void*,void*);
 using EventSync=int(*)(void*);
 using EventElapsed=int(*)(float*,void*,void*);
 using BySymbol=int(*)(void**,const void*);
 using NameRef=const char*(*)(void*);
 struct Pair {void* start;void* end;std::string name;bool nested{};};
 EventCreate create_{};EventCreateFlags create_flags_{};bool no_fence_{};EventRecord record_{};EventSync sync_{};EventElapsed elapsed_{};BySymbol by_symbol_{};NameRef name_ref_{};
 std::mutex mutex_;
 bool failed_{};
 std::vector<void*> pool_;
 std::vector<Pair> pending_;
 std::unordered_map<const void*,std::string> names_;
 Aggregate window_;
 JobSplitter split_;
 GapTracker gap_split_;
 void* last_end_{};
 unsigned windows_{};
 bool window_fallback_{};
 std::filesystem::path log_;
 void write(const std::string& text) noexcept {
  try{FILE* f=nullptr;if(_wfopen_s(&f,log_.c_str(),L"a")==0&&f){std::fputs(text.c_str(),f);std::fclose(f);}}catch(...){}
 }
 void fail(const char* what) noexcept {
  if(failed_)return;
  failed_=true;
  try{write(std::string("DESLIGADO ")+what+" (o lancamento segue normal, sem cronometro)\n");}catch(...){}
  for(auto& p:pending_){pool_.push_back(p.start);if(p.end)pool_.push_back(p.end);}
  pending_.clear();
 }

 void* event() noexcept {
  if(!pool_.empty()){void* e=pool_.back();pool_.pop_back();return e;}
  void* e{};

  if(no_fence_&&create_flags_&&!create_flags_(&e,0x20000000u)&&e)return e;
  if(no_fence_){no_fence_=false;write("AVISO hipEventDisableSystemFence recusado; eventos padrao\n");}
  e=nullptr;if(create_(&e)||!e){fail("hipEventCreate");return nullptr;}
  return e;
 }
 void close_job(bool fallback) noexcept {
  if(failed_||pending_.empty())return;
  try{
   for(auto& p:pending_)if(sync_(p.end)){fail("hipEventSynchronize");return;}
   float span{};
   if(elapsed_(&span,pending_.front().start,pending_.back().end)){fail("hipEventElapsedTime span");return;}

   float nested=0;
   for(auto& p:pending_){
    float ms{};if(elapsed_(&ms,p.start,p.end)){fail("hipEventElapsedTime");return;}
    if(p.nested){window_.add(p.name,ms);nested+=ms;continue;}
    window_.add(p.name,ms-nested);nested=0;
    std::string prev;
    if(gap_split_.next(p.name,prev)&&last_end_){
     float gap{};if(elapsed_(&gap,last_end_,p.start)){fail("hipEventElapsedTime gap");return;}
     window_.add_gap(prev,p.name,gap);
    }
    if(last_end_)pool_.push_back(last_end_);
    last_end_=p.end;p.end=nullptr;
   }
   window_.add_span(span);window_.end_job();
   window_fallback_=window_fallback_||fallback;
   for(auto& p:pending_){pool_.push_back(p.start);if(p.end)pool_.push_back(p.end);}
   pending_.clear();
   if(window_.jobs()>=window_jobs){write(format_window(window_,++windows_,window_fallback_));window_.clear();window_fallback_=false;}
  }catch(...){fail("excecao ao fechar o job");}
 }
public:
 bool configure(const std::wstring& ini,const std::filesystem::path& folder) noexcept {
  try{
   if(NRCFG_INT(L"DlssNrOnAmd",L"NeuralKernelTimeline",0,ini.c_str())!=1)return false;
   log_=folder/L"nr_kernel_timeline.log";
   const auto hip=GetModuleHandleW(L"amdhip64_7.dll");
   if(!hip){write("DESLIGADO amdhip64_7.dll ausente\n");return false;}
   create_=reinterpret_cast<EventCreate>(GetProcAddress(hip,"hipEventCreate"));
   create_flags_=reinterpret_cast<EventCreateFlags>(GetProcAddress(hip,"hipEventCreateWithFlags"));

   no_fence_=NRCFG_INT(L"DlssNrOnAmd",L"NeuralKernelTimelineFence",0,ini.c_str())==0;
   record_=reinterpret_cast<EventRecord>(GetProcAddress(hip,"hipEventRecord"));
   sync_=reinterpret_cast<EventSync>(GetProcAddress(hip,"hipEventSynchronize"));
   elapsed_=reinterpret_cast<EventElapsed>(GetProcAddress(hip,"hipEventElapsedTime"));
   by_symbol_=reinterpret_cast<BySymbol>(GetProcAddress(hip,"hipGetFuncBySymbol"));
   name_ref_=reinterpret_cast<NameRef>(GetProcAddress(hip,"hipKernelNameRef"));
   if(!create_||!record_||!sync_||!elapsed_||!by_symbol_||!name_ref_){write("DESLIGADO API HIP ausente\n");return false;}
   write("# nr_kernel_timeline v1 pid="+std::to_string(GetCurrentProcessId())+(no_fence_?" eventos=sem_barreira":" eventos=padrao")+": par de hipEvent em volta de cada hipLaunchKernel do proxy "
         "(gancho inteiro, conversoes incluidas); '->novo' = kernel trocado pelo gancho, '#mN' = modo do VarParams. "
         "job = de _Z21k_pre_block_1h_32_fp89PreParams ao seguinte. Os eventos custam tempo: o total medido fica maior que o real.\n");
   return true;
  }catch(...){return false;}
 }

 std::string name(const void* token) {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto it=names_.find(token);
  if(it!=names_.end())return it->second;
  std::string n;
  void* fn{};const char* text{};
  if(!by_symbol_(&fn,token)&&fn&&(text=name_ref_(fn))!=nullptr)n=text;
  else{char b[40]{};std::snprintf(b,sizeof b,"token@%p",token);n=b;}
  names_.emplace(token,n);
  return n;
 }

 void observe(const std::string& kernel) noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  if(failed_)return;
  const auto c=split_.before(kernel==boundary_name);
  if(c!=Close::none)close_job(c==Close::fallback);
 }
 Mark start(void* stream) noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  if(failed_)return {};
  void* e=event();if(!e)return {};
  if(record_(e,stream)){pool_.push_back(e);fail("hipEventRecord");return {};}
  return {e};
 }

 void nested_stop(Mark m,const char* name,void* stream) noexcept {try{stop(m,name,stream,inside);}catch(...){}}
 void stop(Mark m,std::string name,void* stream,bool nested=false) noexcept {
  if(!m.start)return;
  std::lock_guard<std::mutex> lock(mutex_);
  if(failed_){pool_.push_back(m.start);return;}
  void* e=event();if(!e){pool_.push_back(m.start);return;}
  if(record_(e,stream)){pool_.push_back(m.start);pool_.push_back(e);fail("hipEventRecord");return;}
  try{pending_.push_back({m.start,e,std::move(name),nested});}catch(...){pool_.push_back(m.start);pool_.push_back(e);fail("memoria");}
 }
};
inline Recorder recorder;
inline void configure(const std::wstring&,const std::filesystem::path&) noexcept {}
}
