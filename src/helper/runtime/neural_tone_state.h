#pragma once

#include <atomic>
namespace nrneuraltone {
inline std::atomic<bool> requested{false};

inline std::atomic<bool> multipass_ok{false};

inline bool identidade(const wchar_t* curva,float lift) noexcept {
 if(!curva)return false;const wchar_t* r=L"reinhard";unsigned i=0;
 for(;curva[i]&&r[i];++i){wchar_t c=curva[i];if(c>=L'A'&&c<=L'Z')c=wchar_t(c-L'A'+L'a');if(c!=r[i])return false;}
 return !curva[i]&&!r[i]&&lift==0.f;
}
}
