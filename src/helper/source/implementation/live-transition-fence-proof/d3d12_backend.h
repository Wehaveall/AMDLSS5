#pragma once
#include "fence_proof.h"
struct ID3D12Device;
struct ID3D12CommandQueue;
namespace nrfence {

std::unique_ptr<Backend> make_d3d12_backend(ID3D12Device* device, ID3D12CommandQueue* queue);
}
