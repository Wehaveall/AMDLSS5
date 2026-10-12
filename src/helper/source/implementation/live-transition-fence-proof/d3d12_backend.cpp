#include "d3d12_backend.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <limits>
namespace nrfence {
namespace {
using Microsoft::WRL::ComPtr;
class D3D12Backend final : public Backend {
public:
    D3D12Backend(ID3D12Device* device,ID3D12CommandQueue* queue):device_(device),queue_(queue) {}
    bool initialize() noexcept override {
        if(attempted_) return false;
        attempted_=true;
        if(!device_ || !queue_ || queue_->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT) return false;
        if(FAILED(device_.As(&device_identity_)) || FAILED(queue_.As(&queue_identity_))) return false;
        ComPtr<ID3D12Device> queue_device;
        ComPtr<IUnknown> queue_device_identity;
        if(FAILED(queue_->GetDevice(IID_PPV_ARGS(queue_device.GetAddressOf()))) ||
           FAILED(queue_device.As(&queue_device_identity)) ||
           queue_device_identity.Get()!=device_identity_.Get()) return false;
        if(!healthy()) return false;
        if(FAILED(device_->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(fence_.GetAddressOf())))) return false;
        return healthy();
    }
    Identity identity() const noexcept override {
        return {reinterpret_cast<std::uintptr_t>(device_identity_.Get()),
                reinterpret_cast<std::uintptr_t>(queue_identity_.Get())};
    }
    bool healthy() noexcept override { return device_ && SUCCEEDED(device_->GetDeviceRemovedReason()); }
    long removed_reason() noexcept override { return device_?static_cast<long>(device_->GetDeviceRemovedReason()):static_cast<long>(E_POINTER); }
    bool signal(Value target) noexcept override {
        return queue_ && fence_ && SUCCEEDED(queue_->Signal(fence_.Get(),target));
    }
    Value completed_value() noexcept override {
        return fence_?fence_->GetCompletedValue():std::numeric_limits<Value>::max();
    }
    bool arm_event(Value target, void* event) noexcept override {
        return fence_ && event && SUCCEEDED(fence_->SetEventOnCompletion(target,static_cast<HANDLE>(event)));
    }
private:
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> queue_;
    ComPtr<ID3D12Fence> fence_;
    ComPtr<IUnknown> device_identity_,queue_identity_;
    bool attempted_{};
};
}
std::unique_ptr<Backend> make_d3d12_backend(ID3D12Device* device,ID3D12CommandQueue* queue) {
    return std::make_unique<D3D12Backend>(device,queue);
}
}
