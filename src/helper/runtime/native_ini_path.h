#pragma once
#include "nr_publico.h"
#include "proxy_rva.h"

#include <windows.h>
#include <cstdint>
#include <cstring>

#include "pasta_helper.h"
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>
namespace nrinipath {
inline bool copy_bytes(const void* p,void* out,std::size_t n) noexcept {
 SIZE_T done=0;return p&&ReadProcessMemory(GetCurrentProcess(),p,out,n,&done)&&done==n;
}
namespace detalhe {
struct NativePath {char text[16];std::uint64_t size,capacity;};
struct Visto {std::mutex m;const void* base{};NativePath bruto{};bool valido{};std::wstring caminho;};
inline Visto& visto() noexcept {static Visto* v=new Visto;return *v;}
}

inline std::wstring native(const void* proxy_base) noexcept {
 try{
  if(!proxy_base)return {};
  detalhe::NativePath path{};
  if(!copy_bytes(static_cast<const char*>(proxy_base)+nrproxy::rva_caminho_ini,&path,sizeof(path)))return {};
  auto& visto=detalhe::visto();
  {std::lock_guard<std::mutex> trava(visto.m);
   if(visto.valido&&visto.base==proxy_base&&std::memcmp(&visto.bruto,&path,sizeof(path))==0)return visto.caminho;}
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
  auto wide_result=result.wstring();
  {std::lock_guard<std::mutex> trava(visto.m);visto.base=proxy_base;visto.bruto=path;visto.caminho=wide_result;visto.valido=true;}
  return wide_result;
 }catch(...){return {};}
}

inline std::wstring resolve(const void* proxy_base,const void* address) noexcept {
 (void)address;
 auto chosen=native(proxy_base);if(!chosen.empty())return chosen;
 try{
  const auto& pasta=nrpasta::helper();
  if(!pasta.empty())return (pasta/L"dlssnr_on_amd.ini").wstring();
 }catch(...){}
 return {};
}
}
