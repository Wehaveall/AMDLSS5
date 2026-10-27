#pragma once
#include "fence_proof.h"
#include <cstdio>
struct ID3D12Device;
struct ID3D12CommandQueue;
struct D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1;
struct D3D12_DRED_PAGE_FAULT_OUTPUT1;
namespace nrfence {

std::unique_ptr<Backend> make_d3d12_backend(ID3D12Device* device, ID3D12CommandQueue* queue);

bool dred_escrever_breadcrumbs(std::FILE* f, const D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1& bc) noexcept;
bool dred_escrever_page_fault(std::FILE* f, const D3D12_DRED_PAGE_FAULT_OUTPUT1& pf) noexcept;
}
