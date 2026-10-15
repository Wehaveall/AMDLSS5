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
inline bool requested=false;
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
 (void)f;(void)g;(void)b;(void)args;(void)shared;(void)stream;(void)forward;(void)result;return false;
}
}
