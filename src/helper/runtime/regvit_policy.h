#pragma once
#include <cstdint>
#include <cstring>

namespace nrvitpolicy {
enum class Kind : unsigned {ffwd=0,conv=1,attn=2,views=3};

enum Fn : unsigned {ffwd0=0,ffwd1=1,conv0=2,conv1=3,conv2=4,conv4=5,attn=6,fn_count=7};
struct Dim {unsigned x,y,z;};
inline constexpr unsigned max_ptrs=3;
struct Plan {
 Fn fn{};Dim grid{};Dim block{};unsigned args_bytes{};unsigned char args[80]{};
 std::uint64_t bytes{};
 void* dense_in[max_ptrs]{};unsigned dense_in_count{};
 void* dense_out{};
 void* legacy_in{};
 void* legacy_out[2]{};unsigned legacy_out_count{};
 std::uint64_t legacy_bytes[2]{};
 bool exato{};
};
inline std::uint64_t u64(const unsigned char* b,unsigned o) noexcept {std::uint64_t v{};std::memcpy(&v,b+o,8);return v;}
inline std::int32_t i32(const unsigned char* b,unsigned o) noexcept {std::int32_t v{};std::memcpy(&v,b+o,4);return v;}
inline bool aligned(std::uint64_t p) noexcept {return p&&!(p&3u);}
inline bool apart(std::uint64_t a,std::uint64_t b,std::uint64_t n) noexcept {return (a>b?a-b:b-a)>=n;}
inline bool disjoint(std::uint64_t a,std::uint64_t na,std::uint64_t b,std::uint64_t nb) noexcept {return a+na<=b||b+nb<=a;}

inline constexpr std::int32_t proven_h=24,proven_w=40,proven_n=60,proven_h2=12,proven_w2=20;

inline bool todas_formas=false;

inline bool views_todas=false;

inline bool views_exato=false;
inline bool shape(std::int32_t h,std::int32_t w,std::int32_t n) noexcept {
 if(todas_formas)return h>0&&w>0&&h<=4096&&w<=4096&&h%4==0&&w%4==0&&n==(h/4)*(w/4);
 return h==proven_h&&w==proven_w&&n==proven_n;
}
inline bool pooled_partial(std::int32_t h2,std::int32_t w2) noexcept {return h2%4!=0||w2%4!=0;}
inline bool pooled_shape(std::int32_t h,std::int32_t w,std::int32_t h2,std::int32_t w2) noexcept {
 if(todas_formas)return h2>0&&w2>0&&h2==h/2&&w2==w/2&&(views_todas||views_exato||!pooled_partial(h2,w2));
 return h2==proven_h2&&w2==proven_w2;
}
inline bool legacy_block(Dim b,std::size_t shared) noexcept {return b.x==256&&b.y==1&&b.z==1&&!shared;}
inline bool shift_ok(std::int32_t sx,std::int32_t sy) noexcept {return (sx==0||sx==-4)&&(sy==0||sy==-4);}

inline constexpr unsigned attn_z(bool attn2) noexcept {return attn2?16u:4u;}
inline constexpr unsigned attn_block(bool attn2) noexcept {return attn2?64u:128u;}

inline bool plan(Kind kind,const void* params,Dim g,Dim b,std::size_t shared,Plan& p,bool attn2=false) noexcept {
 if(!params||!legacy_block(b,shared))return false;
 const auto* a=static_cast<const unsigned char*>(params);
 p=Plan{};
 switch(kind){
 case Kind::ffwd:{
  const auto in0=u64(a,0),in1=u64(a,8),out=u64(a,0x10),w=u64(a,0x18);const auto h=i32(a,0x20),wd=i32(a,0x24),n=i32(a,0x28);
  if(!shape(h,wd,n)||g.x!=unsigned(n+3)/4||g.y!=8||g.z!=1||!aligned(out)||!aligned(w)||(!in0)==(!in1))return false;
  p.bytes=std::uint64_t(n)*8192;
  const auto in=in0?in0:in1;if(!aligned(in)||!apart(in,out,p.bytes))return false;
  p.fn=in1?ffwd1:ffwd0;p.args_bytes=48;std::memcpy(p.args,a,48);
  const std::int32_t zero=0;std::memcpy(p.args+0x2c,&zero,4);
  p.grid={unsigned(n),8,1};p.block={32,1,1};
  if(in0){p.dense_in[p.dense_in_count++]=reinterpret_cast<void*>(in0);}else p.legacy_in=reinterpret_cast<void*>(in1);
  p.dense_out=reinterpret_cast<void*>(out);
  return true;}
 case Kind::conv:{
  const auto in=u64(a,0),res=u64(a,8),view=u64(a,0x10),out=u64(a,0x18),extra=u64(a,0x20),w=u64(a,0x28);
  const auto h=i32(a,0x30),wd=i32(a,0x34),n=i32(a,0x38);
  if(!shape(h,wd,n)||g.x!=unsigned(n+3)/4||g.y!=4||g.z!=1||!aligned(in)||!aligned(out)||!aligned(w)||(!res)==(!view))return false;
  if(view&&extra)return false;
  p.bytes=std::uint64_t(n)*8192;
  const auto r=res?res:view;
  if(!aligned(r)||!apart(in,out,p.bytes)||!apart(r,out,p.bytes)||!apart(in,r,p.bytes))return false;
  if(extra&&(!aligned(extra)||!apart(extra,in,p.bytes)||!apart(extra,r,p.bytes)||!apart(extra,out,p.bytes)))return false;
  p.fn=view?conv1:extra?conv2:conv0;p.args_bytes=80;std::memcpy(p.args,a,64);
  const std::int32_t zero=0;std::memcpy(p.args+0x3c,&zero,4);
  p.grid={unsigned(n),8,1};p.block={32,1,1};
  p.dense_in[p.dense_in_count++]=reinterpret_cast<void*>(in);
  if(res)p.dense_in[p.dense_in_count++]=reinterpret_cast<void*>(res);else p.legacy_in=reinterpret_cast<void*>(view);
  p.dense_out=reinterpret_cast<void*>(out);
  if(extra){p.legacy_out[p.legacy_out_count]=reinterpret_cast<void*>(extra);p.legacy_bytes[p.legacy_out_count++]=p.bytes;}
  return true;}
 case Kind::attn:{
  const auto in=u64(a,0),out=u64(a,8),w=u64(a,0x10);const auto h=i32(a,0x18),wd=i32(a,0x1c),sx=i32(a,0x20),sy=i32(a,0x24);
  if(!shape(h,wd,(h/4)*(wd/4))||!shift_ok(sx,sy)||g.x!=unsigned(wd+7-sx)/8||g.y!=unsigned(h+7-sy)/8||g.z!=16)return false;
  p.bytes=std::uint64_t(h/4)*std::uint64_t(wd/4)*8192;
  if(!aligned(in)||!aligned(out)||!aligned(w)||!apart(in,out,p.bytes))return false;
  p.fn=attn;p.args_bytes=40;std::memcpy(p.args,a,40);p.grid={g.x,g.y,attn_z(attn2)};p.block={attn_block(attn2),1,1};
  p.dense_in[p.dense_in_count++]=reinterpret_cast<void*>(in);p.dense_out=reinterpret_cast<void*>(out);
  return true;}
 case Kind::views:{
  const auto in=u64(a,0),view=u64(a,8),res=u64(a,0x10),out=u64(a,0x18),extra=u64(a,0x20),w=u64(a,0x28),pooled=u64(a,0x38);
  const auto h=i32(a,0x30),wd=i32(a,0x34),h2=i32(a,0x40),w2=i32(a,0x44);const std::int32_t n=(h/4)*(wd/4);
  if(!shape(h,wd,n)||g.x!=unsigned(n)||g.y!=1||g.z!=1||view||extra||!aligned(in)||!aligned(res)||!aligned(out)||!aligned(w)||!aligned(pooled))return false;
  if(!pooled_shape(h,wd,h2,w2))return false;
  p.bytes=std::uint64_t(n)*8192;const std::uint64_t pb=std::uint64_t((h2+3)/4)*std::uint64_t((w2+3)/4)*8192;

  if(!apart(in,out,p.bytes)||!apart(res,out,p.bytes)||!apart(in,res,p.bytes)||!disjoint(pooled,pb,in,p.bytes)||!disjoint(pooled,pb,res,p.bytes)||!disjoint(pooled,pb,out,p.bytes))return false;
  p.fn=conv4;p.args_bytes=80;p.exato=views_exato&&pooled_partial(h2,w2);
  std::memcpy(p.args,a,8);std::memcpy(p.args+8,a+0x10,8);
  std::memset(p.args+0x10,0,8);std::memcpy(p.args+0x18,a+0x18,0x18);
  std::memcpy(p.args+0x30,a+0x30,8);std::memcpy(p.args+0x38,&n,4);std::memset(p.args+0x3c,0,4);
  std::memcpy(p.args+0x40,a+0x38,8);std::memcpy(p.args+0x48,a+0x40,8);
  p.grid={unsigned(n),8,1};p.block={32,1,1};
  p.dense_in[p.dense_in_count++]=reinterpret_cast<void*>(in);p.dense_in[p.dense_in_count++]=reinterpret_cast<void*>(res);
  p.dense_out=reinterpret_cast<void*>(out);p.legacy_out[p.legacy_out_count]=reinterpret_cast<void*>(pooled);p.legacy_bytes[p.legacy_out_count++]=pb;
  return true;}
 }
 return false;
}

struct Tensor {void* ptr{};std::uint64_t bytes{};};
struct Tracker {
 static constexpr unsigned cap=8;
 Tensor held[cap]{};unsigned count{};void* stream{};
 unsigned stage{};void* f_out{};void* c1_out{};void* a_out{};

 bool drop_final{};
 int find(const void* p) const noexcept {for(unsigned i=0;i<count;++i)if(held[i].ptr==p)return int(i);return -1;}
 void erase(unsigned i) noexcept {held[i]=held[--count];}
 bool add(void* p,std::uint64_t b) noexcept {if(find(p)>=0)return true;if(count==cap)return false;held[count++]={p,b};return true;}
 bool empty() const noexcept {return !count;}

 struct Before {Tensor to_frag[max_ptrs]{};unsigned n_frag{};Tensor to_tin[3]{};unsigned n_tin{};};
 bool before(const Plan& p,void* s,Before& out) noexcept {
  out=Before{};
  if(count&&s!=stream)return false;

  for(unsigned i=0;i<p.dense_in_count;++i){const int k=find(p.dense_in[i]);if(k>=0&&held[k].bytes!=p.bytes)return false;}
  {const int k=find(p.dense_out);if(k>=0&&held[k].bytes!=p.bytes)return false;}
  for(unsigned i=0;i<p.dense_in_count;++i){
   if(find(p.dense_in[i])>=0)continue;
   bool dup=false;for(unsigned j=0;j<out.n_frag;++j)dup|=out.to_frag[j].ptr==p.dense_in[i];
   if(!dup)out.to_frag[out.n_frag++]={p.dense_in[i],p.bytes};
  }
  const void* legacy[3]{p.legacy_in,p.legacy_out[0],p.legacy_out[1]};
  for(const void* l:legacy){const int k=l?find(l):-1;if(k>=0)out.to_tin[out.n_tin++]=held[k];}

  unsigned extra=out.n_frag+(find(p.dense_out)>=0?0u:1u);
  return count-out.n_tin+extra<=cap;
 }
 void after(const Plan& p,void* s,const Before& b) noexcept {
  stream=s;
  for(unsigned i=0;i<b.n_tin;++i){const int k=find(b.to_tin[i].ptr);if(k>=0)erase(unsigned(k));}
  for(unsigned i=0;i<b.n_frag;++i)add(b.to_frag[i].ptr,b.to_frag[i].bytes);
  add(p.dense_out,p.bytes);

  const bool is_conv=p.fn==conv0||p.fn==conv1||p.fn==conv2||p.fn==conv4;
  if(p.fn==ffwd0||p.fn==ffwd1){stage=1;f_out=p.dense_out;}
  else if(is_conv&&stage==1&&p.dense_in[0]==f_out){stage=2;c1_out=p.dense_out;}
  else if(p.fn==attn&&stage==2&&p.dense_in[0]==c1_out){stage=3;a_out=p.dense_out;}
  else if(is_conv&&stage==3&&p.dense_in_count==2&&p.dense_in[0]==a_out&&p.dense_in[1]==c1_out&&
          f_out!=p.dense_out&&c1_out!=p.dense_out&&a_out!=p.dense_out){
   void* const dead[3]{f_out,c1_out,a_out};
   for(void* d:dead){const int k=find(d);if(k>=0)erase(unsigned(k));}
   if(drop_final&&p.fn==conv2){const int k=find(p.dense_out);if(k>=0)erase(unsigned(k));}
   stage=0;f_out=c1_out=a_out=nullptr;
  }
  else {stage=0;f_out=c1_out=a_out=nullptr;}
 }

 unsigned take_all(Tensor out[cap],void*& s) noexcept {
  const unsigned n=count;for(unsigned i=0;i<n;++i)out[i]=held[i];s=stream;count=0;stage=0;f_out=c1_out=a_out=nullptr;return n;
 }
};

inline bool refs_tracked(const unsigned char* a,unsigned bytes,const Tracker& t) noexcept {
 if(!a)return false;
 for(unsigned o=0;o+8<=bytes;o+=8){
  const std::uint64_t v=u64(a,o);
  for(unsigned i=0;i<t.count;++i){const auto b=reinterpret_cast<std::uint64_t>(t.held[i].ptr);if(v>=b&&v-b<t.held[i].bytes)return true;}
 }
 return false;
}
}
