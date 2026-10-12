#pragma once
#include "active_scheduling.h"
#include "ffx_descriptor.h"
#include <cstddef>

namespace nractive {

const char* resource_refusal(const nrffx::Descriptor& d,const NrResourceDesc (&x)[5],char* campo,std::size_t n) noexcept;

const char* environment_refusal(const NrActiveEnvironmentV1& e) noexcept;

const char* ultimo_veneno() noexcept;
}
