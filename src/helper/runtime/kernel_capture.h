#pragma once
#include "nr_publico.h"

#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace nrcapture {
struct Dim3 {unsigned x,y,z;};
using Launch=int(*)(const void*,Dim3,Dim3,void**,std::size_t,void*);
inline constexpr bool requested=false;
inline std::string symbol;
inline unsigned arg_bytes=0,limit=2,skip=0;
inline int mode_filter=-1;
inline std::atomic<unsigned> seen{0},done{0};
inline std::filesystem::path root;
inline std::mutex mutex;
inline const void* token{};inline bool token_checked=false;
inline int(*by_symbol)(void**,const void*){};inline const char*(*name_ref)(void*){};
inline int(*range)(void**,std::size_t*,void*){};inline int(*sync)(void*){};inline int(*copy)(void*,const void*,std::size_t,int){};
inline void configure(const std::wstring& ini,const std::filesystem::path& folder) noexcept {
(void)ini;(void)folder;
}
inline bool matches(const void* f) noexcept {
 if(token)return f==token;
 void* fn{};if(by_symbol(&fn,f)||!fn)return false;
 const char* n=name_ref(fn);
 if(n&&symbol==n){token=f;return true;}
 return false;
}
struct Buffer{std::uint64_t base,bytes;std::string id;};
inline void write(const std::filesystem::path& p,const void* data,std::size_t n){std::ofstream o(p,std::ios::binary);o.write(static_cast<const char*>(data),static_cast<std::streamsize>(n));}

inline bool maybe(const void* f,Dim3 g,Dim3 b,void** args,std::size_t shared,void* stream,Launch forward,int& result) noexcept {
 if(!requested||done.load()>=limit||!args||!args[0])return false;
 try{
  std::lock_guard<std::mutex> lock(mutex);
  if(!matches(f))return false;
  if(mode_filter>=0){if(arg_bytes<44)return false;unsigned m{};std::memcpy(&m,static_cast<const unsigned char*>(args[0])+40,4);if(m!=static_cast<unsigned>(mode_filter))return false;}
  const unsigned ordinal=seen.fetch_add(1);
  if(ordinal<skip||done.load()>=limit)return false;
  const auto* a=static_cast<const unsigned char*>(args[0]);
  std::vector<Buffer> buffers;std::string relocs;std::uint64_t total=0;
  for(unsigned off=0;off+8<=arg_bytes;off+=8){
   std::uint64_t v{};std::memcpy(&v,a+off,8);
   if(v<65536||v>0x0000ffffffffffffull)continue;
   void* base{};std::size_t size{};
   if(range(&base,&size,reinterpret_cast<void*>(v))||!base||!size)continue;
   const auto b0=reinterpret_cast<std::uint64_t>(base);
   std::string id;for(auto& x:buffers)if(x.base==b0&&x.bytes==size)id=x.id;
   if(id.empty()){if(size>512ull<<20||total+size>(1ull<<30))return false;id="b"+std::to_string(buffers.size());buffers.push_back({b0,size,id});total+=size;}
   if(!relocs.empty())relocs+=',';
   relocs+="{\"arg_offset\":"+std::to_string(off)+",\"allocation_id\":\""+id+"\",\"allocation_offset\":"+std::to_string(v-b0)+"}";
  }
  const auto dir=root/(std::to_string(done.load())+"-call-"+std::to_string(ordinal));
  std::filesystem::create_directories(dir);
  write(dir/"args.bin",a,arg_bytes);
  if(sync(stream))return false;
  std::vector<unsigned char> host;
  for(auto& x:buffers){host.resize(x.bytes);if(copy(host.data(),reinterpret_cast<void*>(x.base),x.bytes,2))return false;write(dir/("before-"+x.id+".bin"),host.data(),x.bytes);}
  result=forward(f,g,b,args,shared,stream);
  if(sync(stream))return true;
  std::string bufs;
  for(auto& x:buffers){host.resize(x.bytes);if(copy(host.data(),reinterpret_cast<void*>(x.base),x.bytes,2))return true;write(dir/("after-"+x.id+".bin"),host.data(),x.bytes);
   if(!bufs.empty())bufs+=',';bufs+="{\"id\":\""+x.id+"\",\"bytes\":"+std::to_string(x.bytes)+"}";}
  std::ofstream(dir/"launch.json")<<"{\"schema\":1,\"symbol\":\""<<symbol<<"\",\"ordinal\":"<<ordinal<<",\"arg_bytes\":"<<arg_bytes
   <<",\"grid\":["<<g.x<<","<<g.y<<","<<g.z<<"],\"block\":["<<b.x<<","<<b.y<<","<<b.z<<"],\"shared_bytes\":"<<shared
   <<",\"buffers\":["<<bufs<<"],\"relocations\":["<<relocs<<"],\"result\":"<<result<<"}\n";
  std::ofstream(dir/"COMPLETE")<<"ok\n";
  done.fetch_add(1);
  return true;
 }catch(...){return false;}
}
}
