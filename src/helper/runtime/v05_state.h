#pragma once

#include <atomic>
namespace nrv05 {
inline std::atomic<bool> ligado{false};
inline std::atomic<bool> menu_escreveu{false};
inline bool valor_de(const wchar_t* texto) noexcept {return texto&&texto[0]==L'1'&&texto[1]==0;}
}
