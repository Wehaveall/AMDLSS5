#pragma once

#include "gpu_backend.h"
#include <memory>

namespace nrscale {
std::unique_ptr<GpuBackend> make_d3d12_backend();

std::unique_ptr<InputAdapter> make_d3d12_input_adapter();
}
