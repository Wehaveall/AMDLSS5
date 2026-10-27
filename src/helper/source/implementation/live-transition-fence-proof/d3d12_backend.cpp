#include "d3d12_backend.h"
#include "../../../runtime/nr_publico.h"
#include "../../../runtime/pasta_helper.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <atomic>
#include <limits>
#include <cstdio>
#include <cwchar>
namespace nrfence {
namespace {
using Microsoft::WRL::ComPtr;
class D3D12Backend final : public Backend {
public:
    D3D12Backend(ID3D12Device* device,ID3D12CommandQueue* queue):device_(device),queue_(queue) {}
    bool initialize() noexcept override {
        if(attempted_) return false;
        attempted_=true;

        if(!device_ || !queue_ || (queue_->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT&&queue_->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_COMPUTE)) return false;
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

    void dred() noexcept override;
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
namespace {

void andar_breadcrumbs(std::FILE* f,const D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1& bc) noexcept {
    unsigned nos=0,pararam=0,nem_comecaram=0;
    for(auto* no=bc.pHeadAutoBreadcrumbNode;no&&nos<4096;no=no->pNext,++nos){
        const UINT feito=no->pLastBreadcrumbValue?*no->pLastBreadcrumbValue:0;
        const DredLista estado=dred_lista(feito,no->BreadcrumbCount);
        if(estado==DredLista::Completa)continue;
        if(estado==DredLista::NaoComecou){++nem_comecaram;continue;}
        ++pararam;
        std::fprintf(f,"PAROU fila=%p '%ls' lista=%p '%ls' feitos=%u de %u ops:",static_cast<void*>(no->pCommandQueue),
            no->pCommandQueueDebugNameW?no->pCommandQueueDebugNameW:L"",static_cast<void*>(no->pCommandList),
            no->pCommandListDebugNameW?no->pCommandListDebugNameW:L"",feito,no->BreadcrumbCount);
        const DredJanela j=dred_janela(feito,no->BreadcrumbCount);
        for(UINT k=j.de;k<j.ate&&no->pCommandHistory;++k)std::fprintf(f," %s%d",k==feito?">":"",static_cast<int>(no->pCommandHistory[k]));
        std::fprintf(f,"\n");

        UINT ult[4]{};unsigned nu=0;
        for(UINT c=0;c<no->BreadcrumbContextsCount&&no->pBreadcrumbContexts&&c<65536u;++c){if(no->pBreadcrumbContexts[c].BreadcrumbIndex>=j.ate)break;ult[nu%4u]=c;++nu;}
        for(unsigned i=nu>4u?nu-4u:0u;i<nu;++i){const auto& x=no->pBreadcrumbContexts[ult[i%4u]];
            std::fprintf(f,"  contexto op=%u%s '%ls'\n",x.BreadcrumbIndex,x.BreadcrumbIndex<=feito?"":" (depois do ponto)",x.pContextString?x.pContextString:L"");}
        if(!nu)std::fprintf(f,"  sem contexto ate a op %u (DRED breadcrumb-contexts desligado ou lista sem marcador)\n",j.ate);
    }
    std::fprintf(f,"breadcrumbs: %u listas, %u pararam no meio, %u nem comecaram\n",nos,pararam,nem_comecaram);
}
void andar_page_fault(std::FILE* f,const D3D12_DRED_PAGE_FAULT_OUTPUT1& pf) noexcept {
    std::fprintf(f,"page_fault_va=0x%llx\n",static_cast<unsigned long long>(pf.PageFaultVA));
    unsigned k=0;for(auto* a=pf.pHeadExistingAllocationNode;a&&k<64;a=a->pNext,++k)std::fprintf(f,"  viva '%ls' tipo=%d\n",a->ObjectNameW?a->ObjectNameW:L"",static_cast<int>(a->AllocationType));
    k=0;for(auto* a=pf.pHeadRecentFreedAllocationNode;a&&k<64;a=a->pNext,++k)std::fprintf(f,"  liberada '%ls' tipo=%d\n",a->ObjectNameW?a->ObjectNameW:L"",static_cast<int>(a->AllocationType));
}
}
bool dred_escrever_breadcrumbs(std::FILE* f,const D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1& bc) noexcept {
    if(!f)return false;
    __try{andar_breadcrumbs(f,bc);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){std::fprintf(f,"\ncaminhada dos breadcrumbs interrompida: excecao 0x%08lX (no invalido do driver)\n",static_cast<unsigned long>(GetExceptionCode()));return false;}
}
bool dred_escrever_page_fault(std::FILE* f,const D3D12_DRED_PAGE_FAULT_OUTPUT1& pf) noexcept {
    if(!f)return false;
    __try{andar_page_fault(f,pf);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){std::fprintf(f,"\ncaminhada do page fault interrompida: excecao 0x%08lX (no invalido do driver)\n",static_cast<unsigned long>(GetExceptionCode()));return false;}
}
namespace {

void D3D12Backend::dred() noexcept {
}
}
}
