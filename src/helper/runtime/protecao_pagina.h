#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <cstddef>
#include <cstdint>
namespace nrpagina {
inline bool executavel(DWORD protecao) noexcept {
 const DWORD b=protecao&0xFFu;
 return b==PAGE_EXECUTE||b==PAGE_EXECUTE_READ||b==PAGE_EXECUTE_READWRITE||b==PAGE_EXECUTE_WRITECOPY;
}
inline DWORD gravavel(DWORD atual) noexcept {return executavel(atual)?PAGE_EXECUTE_READWRITE:PAGE_READWRITE;}

inline DWORD gravavel_faixa(const void* p,std::size_t n) noexcept {
 if(!p)return PAGE_READWRITE;
 MEMORY_BASIC_INFORMATION m{};bool exec=false;
 if(VirtualQuery(p,&m,sizeof m))exec=executavel(m.Protect);
 if(!exec&&n>1){const auto* fim=static_cast<const unsigned char*>(p)+(n-1);MEMORY_BASIC_INFORMATION u{};
  if(VirtualQuery(fim,&u,sizeof u))exec=executavel(u.Protect);}
 return exec?PAGE_EXECUTE_READWRITE:PAGE_READWRITE;
}
}
