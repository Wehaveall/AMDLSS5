#pragma once
#include <mutex>

namespace nrini {

bool install(void* base) noexcept;

std::recursive_mutex& mutex() noexcept;

bool reler_agora(void* base,unsigned inline_ini) noexcept;
}
