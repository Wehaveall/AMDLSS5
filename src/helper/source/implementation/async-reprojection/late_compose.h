#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
namespace nrreproject {
struct LateState {
    static constexpr unsigned ring=16;
    static constexpr unsigned snap_stride=256;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> reduzir_heap;

    Microsoft::WRL::ComPtr<ID3D12Resource> tex;
    Microsoft::WRL::ComPtr<ID3D12Resource> snapshot;
    Microsoft::WRL::ComPtr<ID3D12Resource> readback;
    const volatile std::uint64_t* readback_map{};
    unsigned w{},h{};DXGI_FORMAT format{DXGI_FORMAT_UNKNOWN};bool falhou{};

    struct Fonte {ID3D12Resource* color{};DXGI_FORMAT color_format{};ID3D12Resource* motion{};DXGI_FORMAT motion_format{};
                  ID3D12Resource* depth{};ID3D12Resource* exposure{};
                  DXGI_FORMAT depth_format{DXGI_FORMAT_R32_FLOAT};} fonte;
    int armed[ring]{};

    struct Pendente {bool ativo{};std::uint64_t quadro{};int job_prev{},job_late{},age_prev{},age_late{};long long cap_prev{},cap_late{};bool b_ok{true};};
    Pendente pend[ring];
    std::uint64_t gravados{};
    std::uint64_t armed_n{},recorded{},resolved{},executed_late{},refused{},b_failed{},inconsistent{};
    double gain_frames{},gain_ms{};
    std::uint64_t refused_reason[8]{};
};
}
