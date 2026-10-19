#pragma once

#include "nr_publico.h"
#include <windows.h>
#include <share.h>
#include <atomic>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
namespace nrlogfila {

template<class... A> inline void linha(const wchar_t*,const char*,A&&...) noexcept {}
template<class T> inline void escrever(const wchar_t*,T&&) noexcept {}
inline void esvaziar() noexcept {}
}
