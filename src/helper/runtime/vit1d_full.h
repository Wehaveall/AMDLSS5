#pragma once
#include <cstdint>
#include <cstring>

namespace nrvit1dfull {
enum class Kind : unsigned {repack,expand,contract,qkv,attention};
struct Dim {unsigned x,y,z;};
inline constexpr unsigned bytes_per_token=8192;
inline constexpr unsigned legacy_bytes[5]={32,24,48,40,40};
inline unsigned ru4(unsigned v) noexcept {return (v+3u)&~3u;}
inline unsigned ru64(unsigned v) noexcept {return (v+63u)&~63u;}
inline std::uint64_t u64(const void* p,unsigned o) noexcept {std::uint64_t v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+o,8);return v;}
inline std::int32_t i32(const void* p,unsigned o) noexcept {std::int32_t v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+o,4);return v;}
inline void put64(unsigned char* b,unsigned o,std::uint64_t v) noexcept {std::memcpy(b+o,&v,8);}
inline void put32(unsigned char* b,unsigned o,std::int32_t v) noexcept {std::memcpy(b+o,&v,4);}

struct Geometry {unsigned a{},b{},n{},a2{},b2{},real2{},n2{};};

inline bool plan(const void* repack_params,unsigned max_n,Geometry& g) noexcept {
 if(!repack_params)return false;
 const std::int32_t a=i32(repack_params,0x10),b=i32(repack_params,0x14),n=i32(repack_params,0x18),dir=i32(repack_params,0x1c);
 if(dir!=1||a<=0||b<=0||n<=0||n%64||std::int64_t(a)*b>n)return false;
 if(!u64(repack_params,0)||!u64(repack_params,8))return false;
 g=Geometry{};g.a=unsigned(a);g.b=unsigned(b);g.n=unsigned(n);g.a2=ru4(g.a);g.b2=ru4(g.b);g.real2=g.a2*g.b2;g.n2=ru64(g.real2);
 if(g.a2==g.a&&g.b2==g.b)return false;
 if(g.n2>max_n)return false;
 return true;
}
inline std::uint64_t shadow_bytes(const Geometry& g) noexcept {return std::uint64_t(g.n2)*bytes_per_token;}

struct Slots {unsigned count;unsigned off[4];};
inline Slots token_slots(Kind k,bool forward) noexcept {
 switch(k){
  case Kind::repack: return forward?Slots{1,{8,0,0,0}}:Slots{1,{0,0,0,0}};
  case Kind::expand: return Slots{2,{0,8,0,0}};
  case Kind::contract: return Slots{3,{0,8,0x10,0}};
  case Kind::qkv: return Slots{4,{0,8,0x10,0x18}};
  case Kind::attention: return Slots{4,{0,8,0x10,0x18}};
 }
 return Slots{0,{}};
}

struct Launch {unsigned char args[48]{};unsigned bytes{};Dim grid{};};

template<class Remap>
inline bool transform(Kind k,const void* params,Dim g,const Geometry& geo,Remap&& remap,Launch& out) noexcept {
 if(!params)return false;
 const unsigned idx=static_cast<unsigned>(k);if(idx>4)return false;
 out=Launch{};out.bytes=legacy_bytes[idx];std::memcpy(out.args,params,out.bytes);out.grid=g;
 bool forward=false;
 switch(k){
  case Kind::repack:{
   const std::int32_t a=i32(params,0x10),b=i32(params,0x14),n=i32(params,0x18),dir=i32(params,0x1c);
   if(unsigned(a)!=geo.a||unsigned(b)!=geo.b||unsigned(n)!=geo.n||(dir!=0&&dir!=1))return false;
   forward=dir==1;
   put32(out.args,0x10,std::int32_t(geo.a2));put32(out.args,0x14,std::int32_t(geo.b2));put32(out.args,0x18,std::int32_t(geo.n2));
   break;}
  case Kind::expand: case Kind::contract: case Kind::qkv:
   if(g.x!=geo.n/64)return false;
   out.grid.x=geo.n2/64;break;
  case Kind::attention:
   if(g.x!=geo.n/64||i32(params,0x20)!=std::int32_t(geo.n)||i32(params,0x24)!=std::int32_t(geo.a*geo.b))return false;
   out.grid.x=geo.n2/64;put32(out.args,0x20,std::int32_t(geo.n2));put32(out.args,0x24,std::int32_t(geo.real2));
   break;
 }
 const Slots s=token_slots(k,forward);
 for(unsigned i=0;i<s.count;++i){
  void* p=reinterpret_cast<void*>(u64(params,s.off[i]));if(!p)return false;
  void* m=remap(p);if(!m)return false;
  put64(out.args,s.off[i],reinterpret_cast<std::uint64_t>(m));
 }
 return true;
}

struct Shadows {
 static constexpr unsigned cap=48,retired_cap=64;
 struct Entry {void* proxy{};void* mine{};std::uint64_t bytes{};unsigned n2{};unsigned long long used{};};
 Entry e[cap]{};unsigned count{};void* retired[retired_cap]{};unsigned retired_count{};unsigned long long evictions{};
 template<class Release> bool retire(void* m,Release&& release) noexcept {
  if(retired_count>=retired_cap){release(retired[0]);for(unsigned i=1;i<retired_count;++i)retired[i-1]=retired[i];--retired_count;}
  retired[retired_count++]=m;return true;
 }
 template<class Bytes,class Alloc,class Clear,class Release>
 void* get(void* proxy,unsigned n2,unsigned long long job,Bytes&& bytes,Alloc&& alloc,Clear&& clear,Release&& release) noexcept {
  Entry* x=nullptr;
  for(unsigned i=0;i<count;++i)if(e[i].proxy==proxy){x=&e[i];break;}
  if(x&&x->n2==n2){x->used=job;return x->mine;}
  if(!x){
   if(count<cap)x=&e[count++],*x=Entry{};
   else {
    for(unsigned i=0;i<cap;++i)if(e[i].used+1<job&&(!x||e[i].used<x->used))x=&e[i];
    if(!x)return nullptr;
    ++evictions;x->proxy=nullptr;x->n2=0;
   }
   x->proxy=proxy;
  }
  const std::uint64_t need=bytes();
  if(!x->mine||x->bytes<need){
   void* m=alloc(need);if(!m){if(!x->mine){*x=Entry{};if(x==&e[count-1])--count;}return nullptr;}
   if(x->mine)retire(x->mine,release);
   x->mine=m;x->bytes=need;
  }
  if(!clear(x->mine,x->bytes))return nullptr;
  x->n2=n2;x->used=job;return x->mine;
 }
 std::uint64_t total() const noexcept {std::uint64_t t=0;for(unsigned i=0;i<count;++i)t+=e[i].bytes;return t;}
};

inline std::uint64_t scaled_bytes(std::uint64_t proxy_bytes,const Geometry& g) noexcept {
 if(!proxy_bytes||!g.n)return shadow_bytes(g);
 const std::uint64_t b=(proxy_bytes*g.n2+g.n-1)/g.n;return (b+255u)&~std::uint64_t(255);
}

struct Chain {bool active{};Geometry geo{};void* stream{};};
}
