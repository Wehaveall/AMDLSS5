#pragma once
#include "nr_publico.h"

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
namespace nrinipath {
inline bool copy_bytes(const void* p,void* out,std::size_t n) noexcept {
 SIZE_T done=0;return p&&ReadProcessMemory(GetCurrentProcess(),p,out,n,&done)&&done==n;
}

inline std::wstring native(const void* proxy_base) noexcept {
 try{
  if(!proxy_base)return {};
  struct NativePath {char text[16];std::uint64_t size,capacity;} path{};
  if(!copy_bytes(static_cast<const char*>(proxy_base)+0x94AC0,&path,sizeof(path)))return {};
  if(!path.size||path.size>32767||path.capacity<path.size||path.capacity>32767)return {};
  const char* data=path.text;if(path.capacity>=16)std::memcpy(&data,path.text,sizeof(data));
  std::string narrow(static_cast<std::size_t>(path.size)+1,'\0');
  if(!copy_bytes(data,narrow.data(),narrow.size())||narrow.back()!='\0'||narrow.find('\0')!=path.size)return {};
  const int count=MultiByteToWideChar(CP_ACP,MB_ERR_INVALID_CHARS,narrow.data(),int(path.size),nullptr,0);
  if(count<=0)return {};
  std::wstring wide(std::size_t(count),L'\0');
  if(MultiByteToWideChar(CP_ACP,MB_ERR_INVALID_CHARS,narrow.data(),int(path.size),wide.data(),count)!=count)return {};
  const std::filesystem::path result(wide);
  if(!result.is_absolute()||GetFileAttributesW(result.c_str())==INVALID_FILE_ATTRIBUTES)return {};
  return result.wstring();
 }catch(...){return {};}
}

inline std::wstring resolve(const void* proxy_base,const void* address) noexcept {
 auto chosen=native(proxy_base);if(!chosen.empty())return chosen;
 try{
  HMODULE self{};wchar_t path[32768]{};
  if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(address),&self)&&
     GetModuleFileNameW(self,path,32768))return (std::filesystem::path(path).parent_path()/L"dlssnr_on_amd.ini").wstring();
 }catch(...){}
 return {};
}
}
