#pragma once
#include "nr_publico.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace nr1080 {
struct Dim {unsigned x,y,z;};
namespace contract {

inline constexpr std::size_t cauda_bytes=19008;
inline constexpr unsigned char cauda_sha256[32]={0x21,0xad,0x99,0x1c,0x12,0x21,0xf8,0xb1,0x90,0xbe,0x82,0xec,0xb0,0xaa,0x80,0x6a,0xae,0xb4,0xdc,0x66,0xc4,0x85,0x9b,0xf9,0xd6,0x05,0x54,0x3e,0x72,0xae,0xfa,0x9c};
inline constexpr const unsigned char* cauda_sha256_alt=nullptr;
inline constexpr const char* final_head="k_final_head_fast";
inline constexpr const char* export_r0="nr_tone_export_r0";

inline constexpr std::size_t rw1d_bytes=39256;
inline constexpr unsigned char rw1d_sha256[32]={0x1f,0x59,0xb4,0xf1,0x29,0x70,0xbe,0x10,0x16,0x25,0x25,0xfa,0xc2,0xfb,0x3b,0x1e,0x51,0x5f,0x5d,0x39,0x74,0x1d,0x39,0x8e,0x15,0x95,0xc3,0xe8,0x06,0xec,0x99,0x03};
inline constexpr const unsigned char* rw1d_sha256_alt=nullptr;
enum Rw1d : unsigned {rw_conv_m3n2,rw_conv_m4n2,rw_conv_m1n1,rw_expand,rw_attn,rw_count};
inline constexpr const char* rw1d[rw_count]={"k1d_conv_m3n2_g1x4_p4","k1d_conv_m4n2_g1x4_p4","k1d_conv_m1n1_g1x4_p8","k1d_expand_m4n2_g1x2_p4_x1","k1d_attn_w4q1_l80"};

inline constexpr std::size_t r55_1d_bytes=63888;
inline constexpr unsigned char r55_1d_sha256[32]={0x28,0x28,0x83,0x80,0x0c,0x86,0xb2,0xb1,0x4d,0x23,0xeb,0xdf,0xb8,0x8d,0xc1,0x58,0x76,0x25,0xda,0x5f,0x38,0x10,0x5b,0x4c,0x57,0xb6,0xf1,0x2f,0xe2,0x6f,0x7d,0xf0};
enum R55Qkv : unsigned {r55_qkv_m3,r55_qkv_m4,r55_qkv_count};
inline constexpr const char* r55_qkv[r55_qkv_count]={"k1d_qkv_m3_g1x4_p8","k1d_qkv_m4_g1x4_p4"};
inline constexpr unsigned r55_qkv_mw[r55_qkv_count]={3,4};

inline constexpr std::size_t vitx_bytes=26024;
inline constexpr unsigned char vitx_sha256[32]={0x1c,0xcd,0x54,0x87,0x72,0x40,0xe1,0x1a,0xfa,0x26,0x33,0xca,0x90,0xc4,0x8f,0x0b,0xee,0x26,0xc0,0x3e,0xb6,0xac,0x50,0x66,0x37,0xc3,0xb7,0x99,0x07,0xd6,0x39,0x56};
inline constexpr const unsigned char* vitx_sha256_alt=nullptr;
inline constexpr const char* vitx_conv="k_vitx_conv";
inline constexpr const char* vitx_ffwd="k_vitx_ffwd";

inline constexpr std::size_t vitx_attn_bytes=15128;
inline constexpr unsigned char vitx_attn_sha256[32]={0x61,0xc4,0x15,0x28,0x1c,0x75,0x0d,0xf0,0x50,0x9e,0x71,0x34,0x37,0x0a,0x9d,0x84,0x4f,0xde,0xd5,0xd3,0xeb,0x54,0x02,0x56,0x41,0x0e,0xb6,0x6d,0xe0,0xe0,0xe4,0xeb};
inline constexpr const char* vitx_attn="k_vitx_attn2";

inline constexpr std::size_t vitx_attn_nopulo_bytes=14960;
inline constexpr unsigned char vitx_attn_nopulo_sha256[32]={0x7c,0xc6,0xc1,0x97,0x42,0xb9,0xf2,0xe0,0xc4,0x57,0xa4,0x31,0x70,0x51,0xc4,0x71,0x6f,0x42,0xd5,0xfc,0x15,0x91,0x1a,0x54,0xb1,0x86,0x4a,0xd7,0xa7,0xf0,0x43,0xfd};

inline constexpr std::size_t fast43_mhpeep_bytes=2719832;
inline constexpr unsigned char fast43_mhpeep_sha256[32]={0xb2,0x76,0xc6,0xef,0x4f,0x2f,0x89,0x55,0xff,0x9a,0xec,0x2e,0x6e,0x28,0x59,0x45,0x92,0x97,0xe6,0x93,0x3c,0x6c,0x0f,0xf2,0xbe,0xa1,0x73,0xd9,0xd4,0x04,0xed,0x38};

inline constexpr unsigned char fast43_mhovfl_sha256[32]={0x28,0x42,0xb0,0x12,0x69,0xc9,0xcf,0x2f,0xd3,0x48,0xb5,0x90,0x7e,0xf9,0x97,0xa7,0xbc,0xcb,0x30,0x0f,0xab,0x27,0xde,0xb0,0xef,0x0b,0xef,0xeb,0x26,0xe0,0x9d,0xa4};
inline constexpr const char* decup="_Z11k_reg_decupILb1EEv14DecUpRegParams";

inline constexpr std::size_t sw32x_a_bytes=220856;
inline constexpr unsigned char sw32x_a_sha256[32]={0x98,0x1b,0x87,0x3c,0x80,0xa0,0xf5,0x39,0x25,0xda,0xe8,0x59,0x2c,0x89,0x23,0x57,0x1e,0xfe,0x61,0x7b,0xe8,0x5c,0xf8,0x39,0x3b,0xab,0xcb,0x28,0xa4,0x8d,0xb3,0xd2};
inline constexpr std::size_t sw32x_e_bytes=202456;
inline constexpr unsigned char sw32x_e_sha256[32]={0xb1,0x12,0x2b,0x4a,0x27,0x75,0xdb,0x17,0xdf,0xe6,0x5f,0x3b,0xfb,0xb8,0x14,0x9d,0x4c,0xde,0x54,0x62,0x4d,0x25,0xa7,0xd1,0x2d,0xa7,0xe3,0xfd,0x77,0x33,0xf9,0xff};

inline constexpr std::size_t sw32x_ao_bytes=201336;
inline constexpr unsigned char sw32x_ao_sha256[32]={0xec,0xb0,0x0d,0x8a,0x6d,0x0c,0x32,0xc0,0x79,0xc9,0x82,0x4c,0xdf,0x4a,0x17,0x8c,0x73,0x54,0xa7,0x1d,0xb8,0xc7,0x71,0x15,0x66,0x0d,0xbd,0x93,0x42,0xc5,0x3f,0xfd};

inline constexpr std::size_t r55_mh64_bytes=242544;
inline constexpr unsigned char r55_mh64_sha256[32]={0x07,0xb5,0xe3,0x53,0x97,0x19,0xef,0xee,0x88,0xa1,0x85,0xfd,0x11,0x3e,0xc1,0x29,0xdd,0x17,0x3b,0xa0,0xc4,0x1f,0x4b,0x6a,0x0f,0x0b,0x65,0x37,0x9c,0xfc,0xc1,0xe1};
inline constexpr std::size_t r55_mh128_bytes=275456;
inline constexpr unsigned char r55_mh128_sha256[32]={0x72,0x64,0xbe,0xb6,0x6a,0x94,0xf7,0x37,0x23,0x5f,0x97,0x78,0x4e,0xdb,0xfc,0x3f,0xfd,0xa5,0x4d,0x49,0xbb,0x4e,0xb5,0xa0,0xa1,0xe2,0xcb,0xb2,0x72,0x3d,0x78,0x7d};
inline constexpr std::size_t r55_mh256_bytes=266752;
inline constexpr unsigned char r55_mh256_sha256[32]={0xcc,0xe3,0xdc,0xa5,0x28,0xd3,0x68,0x09,0x04,0xbd,0x15,0x7d,0x89,0x79,0x5e,0xea,0x63,0x01,0x80,0x86,0xc8,0x84,0x74,0x0c,0x9b,0x11,0x67,0x46,0xa8,0x3a,0x55,0xb6};

inline constexpr std::size_t r55_mh64b_bytes=344432;
inline constexpr unsigned char r55_mh64b_sha256[32]={0xe0,0x6f,0x2f,0xee,0x6f,0x41,0x04,0xbb,0x58,0xa5,0x55,0x2e,0x23,0xee,0x45,0x3b,0xf1,0xa5,0xe5,0x0b,0x6f,0x8a,0x76,0x9d,0x25,0xfe,0x91,0x04,0xfa,0x9a,0x6c,0x97};
inline constexpr std::size_t r55_mh128b_bytes=391680;
inline constexpr unsigned char r55_mh128b_sha256[32]={0xbd,0x4d,0x1e,0xa5,0x1e,0xef,0x6e,0x68,0x5e,0x73,0x75,0xdb,0x02,0x1e,0x54,0x7d,0xa4,0x46,0xaf,0xc6,0xf2,0x3b,0x33,0x2d,0x4f,0xe7,0x11,0x37,0x17,0xc0,0x1e,0x5b};
inline constexpr std::size_t r55_mh256b_bytes=382976;
inline constexpr unsigned char r55_mh256b_sha256[32]={0x5d,0x6b,0xea,0x7c,0x23,0x60,0x94,0xdf,0x5b,0x73,0x61,0x7a,0xa5,0xa8,0xe0,0x7a,0x8f,0x47,0x06,0x96,0x6d,0x43,0xe8,0x9a,0x7c,0x5e,0x6f,0x9a,0x44,0xaf,0x29,0xdf};

inline constexpr std::size_t r55_mhp_bytes=1068824;
inline constexpr unsigned char r55_mhp_sha256[32]={0xea,0xd7,0x6f,0xef,0x8b,0x3e,0x91,0xb3,0xc8,0x9f,0x5d,0x81,0x94,0xca,0x95,0x56,0xb6,0xcb,0xd3,0x74,0x16,0x67,0x0c,0x88,0x38,0xa5,0xdd,0x23,0xc1,0xd1,0x2e,0x43};

inline constexpr std::size_t r55_sw32_bytes=188536;
inline constexpr unsigned char r55_sw32_sha256[32]={0x45,0x0b,0x04,0x76,0x63,0x3b,0x10,0xaf,0xb9,0xa6,0x32,0x38,0x4f,0xd5,0x58,0x79,0x33,0x1f,0xce,0xbc,0xbf,0xeb,0xaf,0xbc,0x38,0xa4,0x76,0x88,0xc7,0x65,0xa3,0x48};

inline constexpr const char* sw32x_swin32[8]={
 "_Z12k_reg_swin32ILi0ELb1EEv9VarParams","_Z12k_reg_swin32ILi1ELb1EEv9VarParams","_Z12k_reg_swin32ILi2ELb1EEv9VarParams",
 "_Z12k_reg_swin32ILi4ELb1EEv9VarParams",nullptr,"_Z12k_reg_swin32ILi8ELb1EEv9VarParams",
 "_Z12k_reg_swin32ILi20ELb1EEv9VarParams","_Z12k_reg_swin32ILi32ELb1EEv9VarParams"};
}

struct Sw32x {const wchar_t* file;std::size_t bytes;const unsigned char* sha256;const char* tag;};
inline bool sw32x_ovfl=false;
inline const Sw32x* sw32x_ao() noexcept {
 static constexpr Sw32x ao{L"nr_reg_swin32x_ao.hsaco",contract::sw32x_ao_bytes,contract::sw32x_ao_sha256,NRD("A+OVFL ECB00D8A (A sem clamps fp8, FP16_OVFL)")};
 return &ao;
}
inline bool r55_sw32=false;
inline const Sw32x* sw32x_r55() noexcept {
 static constexpr Sw32x r{L"nr_r55_sw32.hsaco",contract::r55_sw32_bytes,contract::r55_sw32_sha256,NRD("R55 E+OVFL (rede-55 K1)")};
 return &r;
}
inline const Sw32x* sw32x(int key) noexcept {
 static constexpr Sw32x a{L"nr_reg_swin32x_a.hsaco",contract::sw32x_a_bytes,contract::sw32x_a_sha256,NRD("A 981B873C (bit a bit)")};
 static constexpr Sw32x e{L"nr_reg_swin32x_e.hsaco",contract::sw32x_e_bytes,contract::sw32x_e_sha256,NRD("E B1122B4A (>= 50,8 dB)")};
 return key==1?(sw32x_ovfl?sw32x_ao():&a):key==2?(r55_sw32?sw32x_r55():&e):nullptr;
}

struct R55Mh {const wchar_t* file;std::size_t bytes;const unsigned char* sha256;const wchar_t* key;const char* tag;};
inline const R55Mh* r55_mh(unsigned w_index) noexcept {
 static constexpr R55Mh m64{L"nr_r55_mh64.hsaco",contract::r55_mh64_bytes,contract::r55_mh64_sha256,L"NeuralR55Mh64",NRD("MH64")};
 static constexpr R55Mh m128{L"nr_r55_mh128.hsaco",contract::r55_mh128_bytes,contract::r55_mh128_sha256,L"NeuralR55Mh128",NRD("MH128")};
 static constexpr R55Mh m256{L"nr_r55_mh256.hsaco",contract::r55_mh256_bytes,contract::r55_mh256_sha256,L"NeuralR55Mh256",NRD("MH256")};
 return w_index==0?&m64:w_index==1?&m128:w_index==2?&m256:nullptr;
}

inline const R55Mh* r55_mh_borda(unsigned w_index) noexcept {
 static constexpr R55Mh m64{L"nr_r55_mh64b.hsaco",contract::r55_mh64b_bytes,contract::r55_mh64b_sha256,L"NeuralR55MhBorda",NRD("MH64B")};
 static constexpr R55Mh m128{L"nr_r55_mh128b.hsaco",contract::r55_mh128b_bytes,contract::r55_mh128b_sha256,L"NeuralR55MhBorda",NRD("MH128B")};
 static constexpr R55Mh m256{L"nr_r55_mh256b.hsaco",contract::r55_mh256b_bytes,contract::r55_mh256b_sha256,L"NeuralR55MhBorda",NRD("MH256B")};
 return w_index==0?&m64:w_index==1?&m128:w_index==2?&m256:nullptr;
}

inline bool r55_borda_pedida(int chave,int mascara,unsigned w_index) noexcept {return chave==1&&w_index<3&&((mascara>>w_index)&1)!=0;}

struct R55Mhp {const wchar_t* file;std::size_t bytes;const unsigned char* sha256;const char* names[3][3];};
inline const R55Mhp* r55_mhp() noexcept {
 static constexpr R55Mhp m{L"nr_r55_mhp.hsaco",contract::r55_mhp_bytes,contract::r55_mhp_sha256,
  {{"k_mhp64","k_mhp64_e","k_mhp64_d"},{"k_mhp128","k_mhp128_e","k_mhp128_d"},{"k_mhp256","k_mhp256_e","k_mhp256_d"}}};
 return &m;
}

inline int mhp_variante(unsigned modos) noexcept {return (modos&~0x13u)==0?1:(modos&~0x105u)==0?2:0;}

inline bool mhp_vale(int w_index,unsigned janelas,int modo) noexcept {
 if(modo==2)return true;
 if(modo!=1)return false;
 return w_index==0?janelas<1000u:w_index==1?janelas<400u:w_index==2?janelas>=100u:false;
}

inline bool mhp_mode(unsigned mh_mode) noexcept {return mh_mode==0||mh_mode==1||mh_mode==2||mh_mode==4||mh_mode==8;}

struct MhpTail {int w;const void* stream;int h,wd;const void* out;unsigned n,items;};
inline bool mhp_join(const MhpTail& t,int w,const void* stream,int h,int wd,const void* in,unsigned mh_mode,unsigned items,unsigned cap) noexcept {
 return t.n>0&&t.n<8&&t.w==w&&t.stream==stream&&t.h==h&&t.wd==wd&&t.out==in&&!(mh_mode&(1u|8u))&&mhp_mode(mh_mode)&&t.items+items<=cap;
}

inline bool mhp_closes(unsigned mh_mode,unsigned n_after) noexcept {return (mh_mode&(2u|4u))!=0||n_after>=8;}

inline bool r55_mh_mode(unsigned mode,bool fast43) noexcept {return fast43&&(mode<=5||mode==8||mode==10);}

inline int precedencia=0;

inline constexpr bool r55_vence_mh_tab[3][8]={
 {true,true,true,true,true,true,true,true},
 {true,true,true,true,true,true,true,true},
 {true,true,true,true,true,true,true,true}};

inline constexpr bool r55_vence_sw32_tab[8]={true,true,true,false,false,true,false,true};
inline bool r55_vence_mh(unsigned w_index,unsigned mode_index) noexcept {
 if(precedencia==1)return true;if(precedencia==2)return false;
 return w_index<3&&mode_index<8&&r55_vence_mh_tab[w_index][mode_index];
}
inline bool r55_vence_sw32(unsigned index) noexcept {
 if(precedencia==1)return true;if(precedencia==2)return false;
 return index<8&&r55_vence_sw32_tab[index];
}

inline bool sw32x_mode(unsigned index,bool fast43_applied) noexcept {return index<8&&fast43_applied&&contract::sw32x_swin32[index];}
inline std::uint64_t u64(const void* p,unsigned o) noexcept {std::uint64_t v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+o,8);return v;}
inline std::int32_t i32(const void* p,unsigned o) noexcept {std::int32_t v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+o,4);return v;}

inline bool final_head(const void* params,Dim g,Dim b,std::size_t shared,Dim& grid,Dim& block) noexcept {
 if(!params||shared||b.x!=256||b.y!=1||b.z!=1||!g.x||g.y!=1||g.z!=1)return false;
 if(!u64(params,0)||!u64(params,8)||!u64(params,0x10))return false;
 grid={g.x,4,1};block={128,1,1};return true;
}

struct Decup {unsigned char args[48]{};Dim grid{};Dim block{};std::uint64_t dense_bytes{};void* vit512a{};void* out{};};
inline bool decup(const void* params,Dim g,Dim b,std::size_t shared,Decup& d) noexcept {
 if(!params||shared||b.x!=256||b.y!=1||b.z!=1||g.y!=1||g.z!=1)return false;
 const std::int32_t h=i32(params,0x20),w=i32(params,0x24);
 if(h<=0||w<=0||h>4096||w>4096||h%4||w%4)return false;
 const std::int32_t n=(h/4)*(w/4);
 if(g.x!=unsigned(n))return false;
 const auto tok=u64(params,0),vit=u64(params,8),out=u64(params,0x10),wt=u64(params,0x18);
 if(!tok||!vit||!out||!wt||(vit&3)||(out&3))return false;
 d=Decup{};d.dense_bytes=std::uint64_t(n)*8192;
 if((vit>out?vit-out:out-vit)<d.dense_bytes)return false;
 std::memcpy(d.args,params,0x20);
 const std::int32_t f20=((w/2)+3)/4,f24=w/4,f28=h/4;
 std::memcpy(d.args+0x20,&f20,4);std::memcpy(d.args+0x24,&f24,4);std::memcpy(d.args+0x28,&f28,4);std::memcpy(d.args+0x2c,&f24,4);
 d.grid={unsigned(((h/2+3)/4)*((w/2+3)/4)),8,1};d.block={32,1,1};
 d.vit512a=reinterpret_cast<void*>(vit);d.out=reinterpret_cast<void*>(out);
 return true;
}

struct Pick {unsigned fn{};Dim grid{};Dim block{};};
inline bool tokens_ok(unsigned n) noexcept {return n>=64&&n<=1024&&n%64==0;}
inline bool rw_conv(unsigned n,const void* args,Pick& p) noexcept {
 if(!tokens_ok(n)||!args)return false;
 const std::int32_t k=i32(args,0x20),m=i32(args,0x24);
 if((k!=1024&&k!=4096)||std::int64_t(m)!=std::int64_t(k)*1024)return false;
 const unsigned t=n/16;
 if(t<=8)p={contract::rw_conv_m1n1,{t,16,1},{128,1,1}};
 else if(t%3==0&&n>=576)p={contract::rw_conv_m3n2,{n/48,8,1},{128,1,1}};
 else p={contract::rw_conv_m4n2,{n/64,8,1},{128,1,1}};
 return true;
}
inline bool rw_expand(unsigned n,Pick& p) noexcept {
 if(!tokens_ok(n))return false;
 p={contract::rw_expand,{n/64,64,1},{64,1,1}};return true;
}

inline bool r55_qkv(unsigned n,const void* args,Pick& p) noexcept {
 if(!tokens_ok(n)||!args||i32(args,0x28)!=std::int32_t(n))return false;
 if(!u64(args,0)||!u64(args,8)||!u64(args,0x10)||!u64(args,0x18)||!u64(args,0x20))return false;
 const unsigned t=n/16;
 const unsigned v=(t%3==0&&n>=576)?contract::r55_qkv_m3:contract::r55_qkv_m4;
 p={v,{t/contract::r55_qkv_mw[v],24,1},{128,1,1}};return true;
}
inline constexpr unsigned rw_attn_max=640;
inline bool rw_attn(unsigned n,const void* args,Pick& p) noexcept {
 if(!tokens_ok(n)||n>rw_attn_max||!args||i32(args,0x20)!=std::int32_t(n))return false;
 p={contract::rw_attn,{n/64,32,1},{128,1,1}};return true;
}
}
