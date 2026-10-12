#pragma once
#include <cstdint>
#include <cstring>
#include <initializer_list>

namespace nr1dpolicy {
inline constexpr unsigned args_bytes=72;
inline constexpr unsigned split_max=512;
inline constexpr std::uint64_t workspace_bytes=std::uint64_t(split_max)*8192;
struct Dim {unsigned x,y,z;};
struct Plan {unsigned n{};unsigned splits{};Dim conv{};Dim reduce{};};

inline bool plan(const void* params,Dim grid,Dim block,std::size_t shared,Plan& p) noexcept {
 if(!params||shared||block.x!=256||block.y!=1||block.z!=1||grid.y!=8||grid.z!=1||!grid.x||grid.x*64u>split_max)return false;
 const auto* b=static_cast<const unsigned char*>(params);
 std::uint64_t in{},aux{},out{},weights{};std::memcpy(&in,b,8);std::memcpy(&aux,b+8,8);std::memcpy(&out,b+0x10,8);std::memcpy(&weights,b+0x18,8);
 if(!in||!aux||!out||!weights)return false;
 std::int32_t k{},m{},z{};std::memcpy(&k,b+0x20,4);std::memcpy(&m,b+0x24,4);std::memcpy(&z,b+0x28,4);
 if((k!=1024&&k!=4096)||std::int64_t(m)!=std::int64_t(k)*1024||z!=4)return false;
 p.n=grid.x*64u;p.splits=unsigned(z);
 p.conv={(p.n+63u)/64u,8u,p.splits};
 p.reduce={(p.n*128u+255u)/256u,1u,1u};
 return true;
}
inline void build_args(const void* params,void* workspace,unsigned n,unsigned char out[args_bytes]) noexcept {
 std::memcpy(out,params,args_bytes);
 const std::uint64_t zero=0;const auto ws=reinterpret_cast<std::uint64_t>(workspace);const std::int32_t nn=std::int32_t(n),pad=0;
 std::memcpy(out+0x30,&zero,8);std::memcpy(out+0x38,&ws,8);std::memcpy(out+0x40,&nn,4);std::memcpy(out+0x44,&pad,4);
}
}

namespace nr1d43policy {
struct Dim {unsigned x,y,z;};
enum Fn : unsigned {repack,expand,conv_split_fast,conv_fast,conv2_fast,qkv_fast,attn_fast,conv_split_ref,conv_ref,conv2_ref,qkv_ref,attn_ref,qkv_split_fast,qkv_split_ref,fn_count};
enum class Kind : unsigned {repack,expand,contract,qkv,attention};
inline constexpr unsigned max_n=1024;
inline constexpr std::uint64_t workspace_bytes=std::uint64_t(max_n)*8192*3;
inline constexpr std::uint64_t counter_bytes=std::uint64_t(max_n/64)*80;
struct Buffers {void* workspace{};void* counters{};};
struct Plan {Fn fn{};Dim grid{};Dim block{};unsigned bytes{};unsigned char args[80]{};bool forward{};unsigned n{};};
inline std::uint64_t u64(const void* p,unsigned o) noexcept {std::uint64_t v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+o,8);return v;}
inline std::int32_t i32(const void* p,unsigned o) noexcept {std::int32_t v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+o,4);return v;}
inline void put64(unsigned char* b,unsigned o,std::uint64_t v) noexcept {std::memcpy(b+o,&v,8);}
inline void put32(unsigned char* b,unsigned o,std::int32_t v) noexcept {std::memcpy(b+o,&v,4);}
inline bool block256(Dim b,std::size_t shared) noexcept {return b.x==256&&b.y==1&&b.z==1&&!shared;}
inline bool tokens_ok(std::int64_t n) noexcept {return n>=64&&n<=max_n&&n%64==0;}
inline Fn pick(bool fast,Fn f) noexcept {return fast?f:static_cast<Fn>(f+(conv_split_ref-conv_split_fast));}

inline bool plan_repack(const void* params,Dim g,Dim b,std::size_t shared,Plan& p) noexcept {
 if(!params||b.x==0||b.x>1024||b.y!=1||b.z!=1||shared||!g.x||g.y!=1||g.z!=1)return false;
 const std::int32_t n=i32(params,0x18),dir=i32(params,0x1c),a=i32(params,0x10),w=i32(params,0x14);
 if(!u64(params,0)||!u64(params,8)||!tokens_ok(n)||(dir!=0&&dir!=1)||a<=0||w<=0||std::int64_t(a)*w>n)return false;
 p=Plan{};p.fn=repack;p.grid=g;p.block=b;p.bytes=40;std::memcpy(p.args,params,32);put32(p.args,0x20,1);put32(p.args,0x24,0);
 p.forward=dir==1;p.n=unsigned(n);return true;
}
inline bool plan_expand(const void* params,Dim g,Dim b,std::size_t shared,unsigned n,Plan& p) noexcept {
 if(!params||!block256(b,shared)||g.x!=n/64||g.y!=32||g.z!=1||!u64(params,0)||!u64(params,8)||!u64(params,0x10))return false;
 p=Plan{};p.fn=expand;p.grid=g;p.block=b;p.bytes=32;std::memcpy(p.args,params,24);put64(p.args,0x18,0);p.n=n;return true;
}

inline bool plan_contract(const void* params,Dim g,Dim b,std::size_t shared,unsigned n,const Buffers& buf,bool fast,Plan& p,bool allow_split=true,unsigned split_teto=64) noexcept {
 if(!params||!block256(b,shared)||g.x!=n/64||g.y!=8||g.z!=1)return false;
 const std::int32_t k=i32(params,0x20),m=i32(params,0x24),z=i32(params,0x28);
 if((k!=1024&&k!=4096)||std::int64_t(m)!=std::int64_t(k)*1024||z!=4)return false;
 for(unsigned o:{0u,8u,0x10u,0x18u})if(!u64(params,o))return false;
 p=Plan{};p.bytes=80;p.n=n;p.block={256,1,1};
 std::memcpy(p.args,params,0x2c);put32(p.args,0x2c,0);put64(p.args,0x30,0);
 put64(p.args,0x38,reinterpret_cast<std::uint64_t>(buf.workspace));put32(p.args,0x40,std::int32_t(n));put32(p.args,0x44,0);
 put64(p.args,0x48,reinterpret_cast<std::uint64_t>(buf.counters));
 const unsigned t=n/64;
 if(allow_split&&buf.workspace&&buf.counters&&t*8<=split_teto){p.fn=pick(fast,conv_split_fast);p.grid={t,8,unsigned(z)};}
 else if(t*4<=64){p.fn=pick(fast,conv_fast);p.grid={t,8,1};}
 else {p.fn=pick(fast,conv2_fast);p.grid={t,4,1};}
 return true;
}

inline bool plan_qkv(const void* params,Dim g,Dim b,std::size_t shared,unsigned n,const Buffers& buf,bool fast,Plan& p,unsigned split_max=0) noexcept {
 if(!params||!block256(b,shared)||g.x!=n/64||g.y!=32||g.z!=1)return false;
 for(unsigned o:{0u,8u,0x10u,0x18u,0x20u})if(!u64(params,o))return false;
 const bool split=buf.workspace&&buf.counters&&n<=split_max;
 p=Plan{};p.fn=split?(fast?qkv_split_fast:qkv_split_ref):pick(fast,qkv_fast);p.bytes=72;p.n=n;p.grid={n/64,12,split?2u:1u};p.block={256,1,1};
 std::memcpy(p.args,params,40);put32(p.args,0x28,std::int32_t(n));put32(p.args,0x2c,0);put64(p.args,0x30,0);
 put64(p.args,0x38,reinterpret_cast<std::uint64_t>(buf.workspace));
 put64(p.args,0x40,buf.counters?reinterpret_cast<std::uint64_t>(buf.counters)+std::uint64_t(n/64)*8*4:0);
 return true;
}
inline bool plan_attention(const void* params,Dim g,Dim b,std::size_t shared,unsigned n,bool fast,Plan& p) noexcept {
 if(!params||!block256(b,shared)||g.x!=n/64||g.y!=32||g.z!=1)return false;
 for(unsigned o:{0u,8u,0x10u,0x18u})if(!u64(params,o))return false;
 const std::int32_t np=i32(params,0x20),real=i32(params,0x24);
 if(np!=std::int32_t(n)||real<=0||real>np)return false;
 p=Plan{};p.fn=pick(fast,attn_fast);p.bytes=48;p.n=n;p.grid={n/64,32,1};p.block={128,1,1};
 std::memcpy(p.args,params,40);put64(p.args,0x28,0);return true;
}

struct Chain {
 bool active{};unsigned n{};void* stream{};
 bool begin(unsigned tokens,void* s) noexcept {if(active)return false;active=true;n=tokens;stream=s;return true;}
 bool end(void* s) noexcept {if(!active||s!=stream)return false;active=false;n=0;stream=nullptr;return true;}
};
}
