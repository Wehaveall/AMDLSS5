#include "../../../runtime/nr_publico.h"
#include "compute_state.h"
#include "observer_platform.h"
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <map>
#include <mutex>
#include <vector>
#include <cstdio>
#include <atomic>
#include <algorithm>
namespace nrstate { namespace {
using Microsoft::WRL::ComPtr;
using List=ID3D12GraphicsCommandList;
struct Argument {
    enum Kind {None,Table,Constants,CBV,SRV,UAV} kind=None;
    UINT64 address{},mask{};std::array<UINT,64> values{};
};
struct State {
    bool primed{},compute_known=true,graphics_known=true,ray{};

    bool graphics_indirect_unknown{},graphics_bundle_unknown{};
    bool render_pass_active{},render_targets_known=true;
    UINT heaps_count{};std::array<ComPtr<ID3D12DescriptorHeap>,2> heaps;
    ComPtr<ID3D12RootSignature> compute,graphics;
    ComPtr<ID3D12PipelineState> pipeline;ComPtr<ID3D12StateObject> ray_pipeline;
    std::array<Argument,64> args,graphics_args;
    std::array<UINT64,64> unknown_compute{};
    UINT viewport_count{},scissor_count{},target_count{};BOOL targets_contiguous{};bool has_depth{};
    std::array<D3D12_VIEWPORT,16> viewports{};std::array<D3D12_RECT,16> scissors{};
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE,8> targets{};D3D12_CPU_DESCRIPTOR_HANDLE depth{};
    D3D12_PRIMITIVE_TOPOLOGY topology=D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ComPtr<ID3D12Resource> predicate;UINT64 predicate_offset{};
    D3D12_PREDICATION_OP predicate_op=D3D12_PREDICATION_OP_EQUAL_ZERO;
};
void refresh_compute(State& s){s.compute_known=true;for(auto mask:s.unknown_compute)if(mask){s.compute_known=false;break;}}
struct Entry {ComPtr<List> list;State state;UINT64 epoch{};void** table{};};
std::recursive_mutex mutex;
std::map<void*,std::unique_ptr<Entry>> entries;
void** table{};std::array<void*,77> originals{};bool advanced{};

bool first_private{};std::vector<void**> private_tables;

std::uintptr_t private_offset{};
#if NR_SPIN_DRAW
std::atomic<DispatchInterceptor> dispatch_interceptor{};
thread_local bool in_dispatch_interceptor{};
#endif
template<class F>F original(unsigned slot){return reinterpret_cast<F>(originals[slot]);}
template<class F>void update(List* list,F f){std::lock_guard<std::recursive_mutex> lock(mutex);auto e=entries.find(list);if(e!=entries.end())f(e->second->state);}
void clear(List* l,ID3D12PipelineState* p){original<void(*)(List*,ID3D12PipelineState*)>(11)(l,p);reset(l,p);}
void pipeline(List* l,ID3D12PipelineState* p){update(l,[&](State& s){s.pipeline=p;s.ray_pipeline.Reset();s.ray=false;});original<void(*)(List*,ID3D12PipelineState*)>(25)(l,p);}
void predication(List* l,ID3D12Resource* p,UINT64 offset,D3D12_PREDICATION_OP op){
 update(l,[&](State& s){s.predicate=p;s.predicate_offset=offset;s.predicate_op=op;});
 original<void(*)(List*,ID3D12Resource*,UINT64,D3D12_PREDICATION_OP)>(55)(l,p,offset,op);
}
void log_predicate(List* l,const State& s)noexcept{
(void)l;(void)s;
}
void heaps(List* l,UINT n,ID3D12DescriptorHeap* const* h){
 update(l,[&](State& s){if(n>2||(n&&!h)){s.primed=false;return;}bool changed=n!=s.heaps_count;
  for(UINT i=0;i<n;++i)changed|=s.heaps[i].Get()!=h[i];
  if(changed){for(auto* args:{&s.args,&s.graphics_args})for(auto& a:*args)if(a.kind==Argument::Table)a={};}
  s.heaps_count=n;for(UINT i=0;i<2;++i)s.heaps[i]=i<n?h[i]:nullptr;
 });original<void(*)(List*,UINT,ID3D12DescriptorHeap* const*)>(28)(l,n,h);
}
template<bool graphics>void root(List* l,ID3D12RootSignature* p){
 update(l,[&](State& s){auto& r=graphics?s.graphics:s.compute;auto& args=graphics?s.graphics_args:s.args;
  if(r.Get()!=p){args={};(graphics?s.graphics_known:s.compute_known)=true;if(!graphics)s.unknown_compute={};else s.graphics_indirect_unknown=false;}r=p;
 });original<void(*)(List*,ID3D12RootSignature*)>(graphics?30:29)(l,p);
}
template<bool graphics>void descriptor(List* l,UINT i,D3D12_GPU_DESCRIPTOR_HANDLE h){
 update(l,[&](State& s){if(i>=64){s.primed=false;return;}auto& a=(graphics?s.graphics_args:s.args)[i];a={};a.kind=Argument::Table;a.address=h.ptr;});
 original<void(*)(List*,UINT,D3D12_GPU_DESCRIPTOR_HANDLE)>(graphics?32:31)(l,i,h);
}
void constant(List* l,UINT i,UINT value,UINT offset){
 update(l,[&](State& s){if(i>=64||offset>=64){s.primed=false;return;}auto& a=s.args[i];if(a.kind!=Argument::Constants)a={};a.kind=Argument::Constants;a.values[offset]=value;a.mask|=UINT64{1}<<offset;s.unknown_compute[i]&=~(UINT64{1}<<offset);refresh_compute(s);});
 original<void(*)(List*,UINT,UINT,UINT)>(33)(l,i,value,offset);
}
void constants(List* l,UINT i,UINT n,const void* values,UINT offset){
 update(l,[&](State& s){if(i>=64||offset>64||n>64-offset||(n&&!values)){s.primed=false;return;}auto& a=s.args[i];if(a.kind!=Argument::Constants)a={};a.kind=Argument::Constants;
  for(UINT j=0;j<n;++j){a.values[offset+j]=static_cast<const UINT*>(values)[j];a.mask|=UINT64{1}<<(offset+j);s.unknown_compute[i]&=~(UINT64{1}<<(offset+j));}refresh_compute(s);});
 original<void(*)(List*,UINT,UINT,const void*,UINT)>(35)(l,i,n,values,offset);
}
template<unsigned slot,Argument::Kind kind>void address(List* l,UINT i,D3D12_GPU_VIRTUAL_ADDRESS value){
 update(l,[&](State& s){if(i>=64){s.primed=false;return;}auto& a=s.args[i];a={};a.kind=kind;a.address=value;s.unknown_compute[i]=0;refresh_compute(s);});
 original<void(*)(List*,UINT,D3D12_GPU_VIRTUAL_ADDRESS)>(slot)(l,i,value);
}
#if NR_SPIN_DRAW
void graphics_constant(List* l,UINT i,UINT value,UINT offset){
 update(l,[&](State& s){if(i>=64||offset>=64){s.graphics_known=false;return;}auto& a=s.graphics_args[i];if(a.kind!=Argument::Constants)a={};a.kind=Argument::Constants;a.values[offset]=value;a.mask|=UINT64{1}<<offset;});
 original<void(*)(List*,UINT,UINT,UINT)>(34)(l,i,value,offset);
}
void graphics_constants(List* l,UINT i,UINT n,const void* values,UINT offset){
 update(l,[&](State& s){if(i>=64||offset>64||n>64-offset||(n&&!values)){s.graphics_known=false;return;}auto& a=s.graphics_args[i];if(a.kind!=Argument::Constants)a={};a.kind=Argument::Constants;
  for(UINT j=0;j<n;++j){a.values[offset+j]=static_cast<const UINT*>(values)[j];a.mask|=UINT64{1}<<(offset+j);}});
 original<void(*)(List*,UINT,UINT,const void*,UINT)>(36)(l,i,n,values,offset);
}
template<unsigned slot,Argument::Kind kind>void graphics_address(List* l,UINT i,D3D12_GPU_VIRTUAL_ADDRESS value){
 update(l,[&](State& s){if(i>=64){s.graphics_known=false;return;}auto& a=s.graphics_args[i];a={};a.kind=kind;a.address=value;});
 original<void(*)(List*,UINT,D3D12_GPU_VIRTUAL_ADDRESS)>(slot)(l,i,value);
}
void topology(List* l,D3D12_PRIMITIVE_TOPOLOGY value){update(l,[&](State& s){s.topology=value;});original<void(*)(List*,D3D12_PRIMITIVE_TOPOLOGY)>(20)(l,value);}
void viewports(List* l,UINT n,const D3D12_VIEWPORT* values){
 update(l,[&](State& s){if(n>s.viewports.size()||(n&&!values)){s.graphics_bundle_unknown=true;return;}s.viewport_count=n;s.viewports={};for(UINT i=0;i<n;++i)s.viewports[i]=values[i];});
 original<void(*)(List*,UINT,const D3D12_VIEWPORT*)>(21)(l,n,values);
}
void scissors(List* l,UINT n,const D3D12_RECT* values){
 update(l,[&](State& s){if(n>s.scissors.size()||(n&&!values)){s.graphics_bundle_unknown=true;return;}s.scissor_count=n;s.scissors={};for(UINT i=0;i<n;++i)s.scissors[i]=values[i];});
 original<void(*)(List*,UINT,const D3D12_RECT*)>(22)(l,n,values);
}
void targets(List* l,UINT n,const D3D12_CPU_DESCRIPTOR_HANDLE* values,BOOL contiguous,const D3D12_CPU_DESCRIPTOR_HANDLE* depth){
 update(l,[&](State& s){if(n>s.targets.size()||(n&&!values)){s.graphics_bundle_unknown=true;return;}s.render_targets_known=true;s.target_count=n;s.targets_contiguous=contiguous;s.targets={};s.has_depth=depth!=nullptr;s.depth=depth?*depth:D3D12_CPU_DESCRIPTOR_HANDLE{};
  for(UINT i=0;i<(contiguous&&n?1:n);++i)s.targets[i]=values[i];});
 original<void(*)(List*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,BOOL,const D3D12_CPU_DESCRIPTOR_HANDLE*)>(46)(l,n,values,contiguous,depth);
}
void bundle(List* l,List* other){
 update(l,[](State& s){s.graphics_bundle_unknown=true;});
 original<void(*)(List*,List*)>(27)(l,other);
}
void begin_render_pass(List* l,UINT n,const D3D12_RENDER_PASS_RENDER_TARGET_DESC* targets,const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* depth,D3D12_RENDER_PASS_FLAGS flags){
 update(l,[](State& s){s.render_pass_active=true;s.render_targets_known=false;});
 original<void(*)(List*,UINT,const D3D12_RENDER_PASS_RENDER_TARGET_DESC*,const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC*,D3D12_RENDER_PASS_FLAGS)>(68)(l,n,targets,depth,flags);
}
void end_render_pass(List* l){update(l,[](State& s){s.render_pass_active=false;});original<void(*)(List*)>(69)(l);}
void dispatch(List* l,UINT x,UINT y,UINT z){
 const auto callback=dispatch_interceptor.load(std::memory_order_acquire);
 if(callback&&!in_dispatch_interceptor){
  DispatchState value{};bool known=false;
  {std::lock_guard<std::recursive_mutex> lock(mutex);auto e=entries.find(l);
   if(e!=entries.end()){const auto& s=e->second->state;value.root=s.compute.Get();value.pipeline=s.pipeline.Get();
    value.uav0=s.args[0].address;value.uav2=s.args[2].address;for(unsigned i=0;i<4;++i)value.constants[i]=s.args[1].values[i];
    value.complete=s.primed&&s.compute_known&&!s.ray&&value.root&&value.pipeline&&s.args[0].kind==Argument::UAV&&s.args[2].kind==Argument::UAV&&s.args[1].kind==Argument::Constants&&(s.args[1].mask&15u)==15u;
    known=true;
   }
  }
  if(known){struct Guard{Guard(){in_dispatch_interceptor=true;}~Guard(){in_dispatch_interceptor=false;}}guard;if(callback(l,value,x,y,z))return;}
 }
 original<void(*)(List*,UINT,UINT,UINT)>(14)(l,x,y,z);
}
#endif
void indirect(List* l,ID3D12CommandSignature* signature,UINT count,ID3D12Resource* buffer,UINT64 offset,ID3D12Resource* count_buffer,UINT64 count_offset){

 update(l,[](State& s){s.graphics_indirect_unknown=true;for(unsigned i=0;i<64;++i){const auto& a=s.args[i];
   if(a.kind==Argument::Constants)s.unknown_compute[i]|=a.mask;
   else if(a.kind==Argument::CBV||a.kind==Argument::SRV||a.kind==Argument::UAV)s.unknown_compute[i]=1;
  }refresh_compute(s);});
 original<void(*)(List*,ID3D12CommandSignature*,UINT,ID3D12Resource*,UINT64,ID3D12Resource*,UINT64)>(59)(l,signature,count,buffer,offset,count_buffer,count_offset);
}
void ray(List* l,ID3D12StateObject* p){update(l,[&](State& s){s.ray=true;s.ray_pipeline=p;s.pipeline.Reset();});original<void(*)(List*,ID3D12StateObject*)>(75)(l,p);}
struct Hook {unsigned slot;void* address;};
std::vector<Hook> hooks(){std::vector<Hook> h{{11,reinterpret_cast<void*>(&clear)},{25,reinterpret_cast<void*>(&pipeline)},{28,reinterpret_cast<void*>(&heaps)},
 {29,reinterpret_cast<void*>(&root<false>)},{30,reinterpret_cast<void*>(&root<true>)},{31,reinterpret_cast<void*>(&descriptor<false>)},{32,reinterpret_cast<void*>(&descriptor<true>)},
 {33,reinterpret_cast<void*>(&constant)},{35,reinterpret_cast<void*>(&constants)},{37,reinterpret_cast<void*>(&address<37,Argument::CBV>)},
 {39,reinterpret_cast<void*>(&address<39,Argument::SRV>)},{41,reinterpret_cast<void*>(&address<41,Argument::UAV>)},{55,reinterpret_cast<void*>(&predication)},{59,reinterpret_cast<void*>(&indirect)}};
#if NR_SPIN_DRAW
 const Hook graphics_hooks[]{{14,reinterpret_cast<void*>(&dispatch)},{20,reinterpret_cast<void*>(&topology)},{21,reinterpret_cast<void*>(&viewports)},
  {22,reinterpret_cast<void*>(&scissors)},{27,reinterpret_cast<void*>(&bundle)},{34,reinterpret_cast<void*>(&graphics_constant)},
  {36,reinterpret_cast<void*>(&graphics_constants)},{38,reinterpret_cast<void*>(&graphics_address<38,Argument::CBV>)},
  {40,reinterpret_cast<void*>(&graphics_address<40,Argument::SRV>)},{42,reinterpret_cast<void*>(&graphics_address<42,Argument::UAV>)},{46,reinterpret_cast<void*>(&targets)}};
 for(const auto& hook:graphics_hooks)h.push_back(hook);
 if(advanced){h.push_back({68,reinterpret_cast<void*>(&begin_render_pass)});h.push_back({69,reinterpret_cast<void*>(&end_render_pass)});}
#endif
 if(advanced)h.push_back({75,reinterpret_cast<void*>(&ray)});return h;}
bool copy(const void* p,void* out,SIZE_T n)noexcept{SIZE_T done=0;return p&&ReadProcessMemory(GetCurrentProcess(),p,out,n,&done)&&done==n;}
SIZE_T table_bytes(){return (advanced?76u:60u)*sizeof(void*);}

bool private_slots_ok(void** t){
 std::array<void*,77> slots{};if(!copy(t,slots.data(),table_bytes()))return false;
 for(const auto& h:hooks())if(slots[h.slot]!=h.address)return false;return true;
}
bool intact(){if(!table)return false;if(first_private)return private_slots_ok(table);for(const auto& h:hooks())if(table[h.slot]!=h.address)return false;return true;}

bool intact_for(const Entry& e){
 if(!intact())return false;
 if(e.table==table&&!first_private)return true;
 void** current{};if(!copy(e.list.Get(),&current,sizeof(current))||current!=e.table)return false;
 return e.table==table||private_slots_ok(e.table);
}
thread_local char private_refusal[96]{};

bool private_region(void** t,bool slots_in_d3d12,DWORD& protect){
 MEMORY_BASIC_INFORMATION info{};if(!VirtualQuery(t,&info,sizeof(info)))return false;
 const auto offset=reinterpret_cast<std::uintptr_t>(t)-reinterpret_cast<std::uintptr_t>(info.BaseAddress);
 if(offset>info.RegionSize||table_bytes()>info.RegionSize-offset)return false;
 protect=info.Protect;
 return nrrecord::detail::classify_table_region(info.State,info.Type,info.Protect,slots_in_d3d12)==nrrecord::detail::TableRegion::Private;
}

bool exchange_private(void** t,DWORD protect,bool install){
 const auto h=hooks();DWORD previous{};const bool flip=protect!=PAGE_READWRITE;
 if(flip&&!VirtualProtect(t,table_bytes(),PAGE_READWRITE,&previous))return false;
 bool changed=true;unsigned installed=0;
 if(install){
  for(const auto& x:h){if(InterlockedCompareExchangePointer(t+x.slot,x.address,originals[x.slot])!=originals[x.slot]){changed=false;break;}++installed;}
  if(!changed){for(unsigned i=0;i<installed;++i)InterlockedCompareExchangePointer(t+h[i].slot,originals[h[i].slot],h[i].address);}
 }else for(const auto& x:h)InterlockedCompareExchangePointer(t+x.slot,originals[x.slot],x.address);
 DWORD ignored{};const bool protected_again=!flip||VirtualProtect(t,table_bytes(),previous,&ignored)!=FALSE;
 return changed&&protected_again;
}
bool patch_private(void** t,DWORD protect){if(exchange_private(t,protect,true))return true;exchange_private(t,protect,false);return false;}
void unpatch_private(void** t,DWORD protect){exchange_private(t,protect,false);}
class Snapshot final:public Saved {
 Entry& entry_;State state_;UINT64 epoch_;bool restored_{};
public:
 explicit Snapshot(Entry& e):entry_(e),state_(e.state),epoch_(e.epoch){}
 bool suspend_predication()noexcept override{try{
  std::lock_guard<std::recursive_mutex> lock(mutex);
  if(restored_||entry_.epoch!=epoch_||!intact_for(entry_))return false;
  auto* l=entry_.list.Get();
  predication(l,nullptr,0,D3D12_PREDICATION_OP_EQUAL_ZERO);
  log_predicate(l,state_);return true;
 }catch(...){return false;}}
 bool restore()noexcept override{try{
  {std::lock_guard<std::recursive_mutex> lock(mutex);
   if(restored_||entry_.epoch!=epoch_||!intact_for(entry_))return false;}
  auto* l=entry_.list.Get();auto& s=state_;
  ID3D12DescriptorHeap* hs[]{s.heaps[0].Get(),s.heaps[1].Get()};
  original<void(*)(List*,UINT,ID3D12DescriptorHeap* const*)>(28)(l,s.heaps_count,hs);
  original<void(*)(List*,ID3D12RootSignature*)>(29)(l,s.compute.Get());
  if(s.ray)original<void(*)(List*,ID3D12StateObject*)>(75)(l,s.ray_pipeline.Get());
  else original<void(*)(List*,ID3D12PipelineState*)>(25)(l,s.pipeline.Get());
  for(UINT i=0;i<64;++i){const auto& a=s.args[i];switch(a.kind){
   case Argument::Table: original<void(*)(List*,UINT,D3D12_GPU_DESCRIPTOR_HANDLE)>(31)(l,i,{a.address});break;
   case Argument::Constants:for(UINT j=0;j<64;++j)if(a.mask&(UINT64{1}<<j))original<void(*)(List*,UINT,UINT,UINT)>(33)(l,i,a.values[j],j);break;
   case Argument::CBV:original<void(*)(List*,UINT,D3D12_GPU_VIRTUAL_ADDRESS)>(37)(l,i,a.address);break;
   case Argument::SRV:original<void(*)(List*,UINT,D3D12_GPU_VIRTUAL_ADDRESS)>(39)(l,i,a.address);break;
   case Argument::UAV:original<void(*)(List*,UINT,D3D12_GPU_VIRTUAL_ADDRESS)>(41)(l,i,a.address);break;
   default:break;
  }
  const auto& ga=s.graphics_args[i];if(ga.kind==Argument::Table)original<void(*)(List*,UINT,D3D12_GPU_DESCRIPTOR_HANDLE)>(32)(l,i,{ga.address});
  }
  original<void(*)(List*,ID3D12Resource*,UINT64,D3D12_PREDICATION_OP)>(55)(l,s.predicate.Get(),s.predicate_offset,s.predicate_op);
  {std::lock_guard<std::recursive_mutex> lock(mutex);
   if(entry_.epoch!=epoch_)return false;
   entry_.state=state_;restored_=true;}return true;
 }catch(...){return false;}}
};
#if NR_SPIN_DRAW
class GraphicsSnapshot final:public Saved {
 Entry& entry_;UINT64 epoch_;bool restored_{};
 ComPtr<ID3D12RootSignature> root_;ComPtr<ID3D12PipelineState> pipeline_;
 std::array<Argument,64> args_;
 UINT viewport_count_,scissor_count_,target_count_;BOOL contiguous_;bool has_depth_;
 std::array<D3D12_VIEWPORT,16> viewports_;std::array<D3D12_RECT,16> scissors_;
 std::array<D3D12_CPU_DESCRIPTOR_HANDLE,8> targets_;D3D12_CPU_DESCRIPTOR_HANDLE depth_;
 D3D12_PRIMITIVE_TOPOLOGY topology_;
public:
 explicit GraphicsSnapshot(Entry& e):entry_(e),epoch_(e.epoch),root_(e.state.graphics),pipeline_(e.state.pipeline),args_(e.state.graphics_args),
  viewport_count_(e.state.viewport_count),scissor_count_(e.state.scissor_count),target_count_(e.state.target_count),contiguous_(e.state.targets_contiguous),has_depth_(e.state.has_depth),
  viewports_(e.state.viewports),scissors_(e.state.scissors),targets_(e.state.targets),depth_(e.state.depth),topology_(e.state.topology){}
 bool suspend_predication()noexcept override{return false;}
 bool restore()noexcept override{try{
  std::lock_guard<std::recursive_mutex> lock(mutex);
  if(restored_||entry_.epoch!=epoch_||!intact_for(entry_))return false;
  auto* l=entry_.list.Get();

  original<void(*)(List*,ID3D12RootSignature*)>(30)(l,nullptr);
  if(root_)original<void(*)(List*,ID3D12RootSignature*)>(30)(l,root_.Get());
  original<void(*)(List*,ID3D12PipelineState*)>(25)(l,pipeline_.Get());
  for(UINT i=0;i<args_.size();++i){const auto& a=args_[i];switch(a.kind){
   case Argument::Table:original<void(*)(List*,UINT,D3D12_GPU_DESCRIPTOR_HANDLE)>(32)(l,i,{a.address});break;
   case Argument::Constants:for(UINT j=0;j<64;++j)if(a.mask&(UINT64{1}<<j))original<void(*)(List*,UINT,UINT,UINT)>(34)(l,i,a.values[j],j);break;
   case Argument::CBV:original<void(*)(List*,UINT,D3D12_GPU_VIRTUAL_ADDRESS)>(38)(l,i,a.address);break;
   case Argument::SRV:original<void(*)(List*,UINT,D3D12_GPU_VIRTUAL_ADDRESS)>(40)(l,i,a.address);break;
   case Argument::UAV:original<void(*)(List*,UINT,D3D12_GPU_VIRTUAL_ADDRESS)>(42)(l,i,a.address);break;
   default:break;
  }}
  original<void(*)(List*,UINT,const D3D12_VIEWPORT*)>(21)(l,viewport_count_,viewport_count_?viewports_.data():nullptr);
  original<void(*)(List*,UINT,const D3D12_RECT*)>(22)(l,scissor_count_,scissor_count_?scissors_.data():nullptr);
  original<void(*)(List*,D3D12_PRIMITIVE_TOPOLOGY)>(20)(l,topology_);
  original<void(*)(List*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,BOOL,const D3D12_CPU_DESCRIPTOR_HANDLE*)>(46)(l,target_count_,target_count_?targets_.data():nullptr,contiguous_,has_depth_?&depth_:nullptr);
  auto& s=entry_.state;s.graphics=root_;s.pipeline=pipeline_;s.ray=false;s.ray_pipeline.Reset();s.graphics_args=args_;
  s.viewport_count=viewport_count_;s.scissor_count=scissor_count_;s.target_count=target_count_;s.targets_contiguous=contiguous_;s.has_depth=has_depth_;
  s.viewports=viewports_;s.scissors=scissors_;s.targets=targets_;s.depth=depth_;s.topology=topology_;
  s.graphics_known=true;s.graphics_indirect_unknown=false;s.graphics_bundle_unknown=false;
  restored_=true;return true;
 }catch(...){return false;}}
};
#endif
}
namespace {thread_local const char* refusal{};bool refuse(const char* why)noexcept{refusal=why;return false;}}
const char* observe_refusal()noexcept{return refusal;}
void* observed_table()noexcept{std::lock_guard<std::recursive_mutex> lock(mutex);return table;}
std::size_t private_table_count()noexcept{std::lock_guard<std::recursive_mutex> lock(mutex);return private_tables.size()+(first_private?1u:0u);}
bool observe(void* command)noexcept{refusal=nullptr;try{
 std::lock_guard<std::recursive_mutex> lock(mutex);if(const auto known=entries.find(command);known!=entries.end())return intact_for(*known->second)||refuse("observe_known_list_hooks_not_intact");
 if(!command)return refuse("observe_null_command");if(entries.size()>=64)return refuse("observe_entries_cap_64");
 auto* list=static_cast<List*>(command);void** current=*reinterpret_cast<void***>(list);
 const auto object=reinterpret_cast<std::uintptr_t>(command),at=reinterpret_cast<std::uintptr_t>(current);
 if(table&&current!=table&&std::find(private_tables.begin(),private_tables.end(),current)==private_tables.end()){

  if(!first_private||!nrrecord::detail::same_private_offset(object,at,private_offset))
   return refuse(first_private&&nrrecord::detail::table_inside_object(object,at)?"observe_private_offset_differs":"observe_vtable_differs_from_first");
  std::array<void*,77> slots{};DWORD protect{};
  if(!copy(current,slots.data(),table_bytes()))return refuse("observe_vtable_differs_from_first");
  bool same=true;for(const auto& x:hooks())same=same&&slots[x.slot]==originals[x.slot];
  if(!private_region(current,same,protect))return refuse(same?"observe_private_vtable_region_refused":"observe_private_vtable_originals_differ");
  auto e=std::make_unique<Entry>();e->list=list;e->table=current;
  if(!patch_private(current,protect))return refuse("observe_private_patch_failed");
  private_tables.push_back(current);
  if(!intact_for(*e)){private_tables.pop_back();unpatch_private(current,protect);return refuse("observe_hooks_not_intact");}
  entries.emplace(command,std::move(e));return true;
 }
 if(!table){
  ComPtr<ID3D12GraphicsCommandList4> v4;advanced=SUCCEEDED(list->QueryInterface(IID_PPV_ARGS(&v4)))&&v4.Get()==list;
  HMODULE pinned{};if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&observe),&pinned))return refuse("observe_self_pin_failed");
  MEMORY_BASIC_INFORMATION kind{};
  if(VirtualQuery(current,&kind,sizeof(kind))&&kind.Type==MEM_PRIVATE){

   if(!nrrecord::detail::table_inside_object(object,at))return refuse("observe_private_vtable_outside_object");
   std::array<void*,77> slots{};DWORD protect{};
   if(!copy(current,slots.data(),table_bytes()))return refuse("observe_private_vtable_unreadable");
   bool d3d12=true;unsigned bad=0;
   for(const auto& x:hooks())if(!nrrecord::detail::d3d12_runtime_or_driver_code(slots[x.slot])){d3d12=false;bad=x.slot;break;}
   if(!private_region(current,d3d12,protect)){
    if(d3d12)return refuse("observe_private_vtable_region_refused");
    std::snprintf(private_refusal,sizeof(private_refusal),"observe_private_vtable_slot_%u_not_d3d12",bad);return refuse(private_refusal);
   }
   auto e=std::make_unique<Entry>();e->list=list;e->table=current;
   for(const auto& x:hooks())originals[x.slot]=slots[x.slot];
   table=current;first_private=true;private_offset=at-object;
   const auto rollback=[&]{table=nullptr;first_private=false;private_offset=0;};
   if(!patch_private(current,protect)){rollback();return refuse("observe_private_patch_failed");}
   if(!intact_for(*e)){unpatch_private(current,protect);rollback();return refuse("observe_hooks_not_intact");}
   entries.emplace(command,std::move(e));return true;
  }else{
  table=current;const auto h=hooks();for(const auto& x:h)originals[x.slot]=table[x.slot];
  DWORD previous{};const SIZE_T size=(advanced?76u:60u)*sizeof(void*);
  if(!VirtualProtect(table,size,PAGE_EXECUTE_READWRITE,&previous)){table=nullptr;return refuse("observe_virtualprotect_failed");}
  bool changed=true;unsigned installed=0;
  for(const auto& x:h){if(InterlockedCompareExchangePointer(table+x.slot,x.address,originals[x.slot])!=originals[x.slot]){changed=false;break;}++installed;}
  if(!changed){for(unsigned i=0;i<installed;++i)InterlockedCompareExchangePointer(table+h[i].slot,originals[h[i].slot],h[i].address);}
  DWORD ignored{};const bool protected_again=VirtualProtect(table,size,previous,&ignored)!=FALSE;
  if(!changed)return refuse("observe_slot_exchange_race");
  if(!protected_again)return refuse("observe_reprotect_failed");
  }
 }
 auto e=std::make_unique<Entry>();e->list=list;e->table=current;
 if(!intact_for(*e))return refuse("observe_hooks_not_intact");
 entries.emplace(command,std::move(e));return true;
 }catch(...){return refuse("observe_exception");}}
bool tracked(void* command)noexcept{try{std::lock_guard<std::recursive_mutex> lock(mutex);return entries.find(command)!=entries.end();}catch(...){return false;}}
std::size_t count()noexcept{try{std::lock_guard<std::recursive_mutex> lock(mutex);return entries.size();}catch(...){return 0;}}
bool forget(void* command)noexcept{try{
 std::lock_guard<std::recursive_mutex> lock(mutex);auto e=entries.find(command);if(e==entries.end())return false;

 if(first_private&&e->second->table==table)return false;
 if(e->second->table!=table){const auto t=std::find(private_tables.begin(),private_tables.end(),e->second->table);if(t!=private_tables.end())private_tables.erase(t);}
 entries.erase(e);return true;
 }catch(...){return false;}}
void reset(void* command,void* initial)noexcept{try{
 std::lock_guard<std::recursive_mutex> lock(mutex);auto e=entries.find(command);if(e==entries.end())return;
 e->second->state={};e->second->state.primed=true;e->second->state.pipeline=static_cast<ID3D12PipelineState*>(initial);++e->second->epoch;
 }catch(...){}}
std::unique_ptr<Saved> save(void* command)noexcept{try{
 std::lock_guard<std::recursive_mutex> lock(mutex);auto e=entries.find(command);if(e==entries.end()||!intact_for(*e->second))return {};
 const auto& s=e->second->state;if(!s.primed||!s.compute_known||!s.graphics_known||(!s.pipeline&&!s.ray_pipeline))return {};
 return std::make_unique<Snapshot>(*e->second);
 }catch(...){return {};}}
std::uint32_t readiness(void* command)noexcept{try{
 std::lock_guard<std::recursive_mutex> lock(mutex);auto e=entries.find(command);if(e==entries.end())return 0;
 const auto& s=e->second->state;return 1u|(intact_for(*e->second)?2u:0u)|(s.primed?4u:0u)|(s.compute_known?8u:0u)|(s.graphics_known?16u:0u)|((s.pipeline||s.ray_pipeline)?32u:0u);
 }catch(...){return 0;}}
bool set_dispatch_interceptor(DispatchInterceptor callback)noexcept{
#if NR_SPIN_DRAW
 if(!callback)return false;DispatchInterceptor expected=nullptr;
 return dispatch_interceptor.compare_exchange_strong(expected,callback,std::memory_order_release,std::memory_order_relaxed)||expected==callback;
#else
 (void)callback;return false;
#endif
}
std::unique_ptr<Saved> save_graphics(void* command)noexcept{try{
#if NR_SPIN_DRAW
 std::lock_guard<std::recursive_mutex> lock(mutex);auto e=entries.find(command);if(e==entries.end()||!intact_for(*e->second))return {};
 const auto& s=e->second->state;
 if(!s.primed||!s.graphics_known||s.graphics_indirect_unknown||s.graphics_bundle_unknown||s.render_pass_active||!s.render_targets_known||s.ray||!s.pipeline)return {};
 return std::make_unique<GraphicsSnapshot>(*e->second);
#else
 (void)command;return {};
#endif
 }catch(...){return {};}}
}
