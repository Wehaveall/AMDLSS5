#pragma once
#include "../../../runtime/nr_publico.h"

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <cstdio>
#include <string>
#include <vector>
#include <atomic>
namespace nrtela {
using Microsoft::WRL::ComPtr;
struct Item{ComPtr<ID3D12Resource> buf;ComPtr<ID3D12CommandAllocator> al;ComPtr<ID3D12GraphicsCommandList> l;D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT h{};unsigned k{};UINT64 valor{};long long qpc{};};
struct Estado{bool lido=false,falhou=false;unsigned n=0,em=900;unsigned long long presents=0;unsigned feitos=0,serie=0;std::atomic<bool> disparo{false};ComPtr<ID3D12Fence> f;UINT64 valor=0;std::vector<Item> pend;};
inline Estado& st(){static Estado e;return e;}
inline void disparar() noexcept {st().disparo.store(true);}
inline void gravar(Estado& e,bool tudo){
(void)e;(void)tudo;
}
inline void capturar(void* swapchain,void* fila,const std::wstring& ini) noexcept {
(void)swapchain;(void)fila;(void)ini;
}
}
