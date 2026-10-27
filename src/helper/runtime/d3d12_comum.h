#pragma once

#include <utility>
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <climits>
#include <cstdint>
#include <cwchar>
#include "estado_ffx.h"
namespace nrd3d {

constexpr auto estado_recusado=static_cast<D3D12_RESOURCE_STATES>(UINT_MAX);
inline D3D12_RESOURCE_STATES estado_ffx(std::uint32_t valor) noexcept {
    const auto t=nrffx_estado::traduzir(valor);
    return t.d3d12==nrffx_estado::recusa?estado_recusado:static_cast<D3D12_RESOURCE_STATES>(t.d3d12);
}
inline void barreira_uav(ID3D12GraphicsCommandList* lista,ID3D12Resource* recurso) noexcept {
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=recurso;lista->ResourceBarrier(1,&b);
}
inline bool mesmo_dispositivo(ID3D12Device* esperado,ID3D12DeviceChild* filho) noexcept {
    if(!esperado||!filho)return false;Microsoft::WRL::ComPtr<ID3D12Device> real;
    return SUCCEEDED(filho->GetDevice(IID_PPV_ARGS(&real)))&&real.Get()==esperado;
}
inline void nomear(ID3D12Object* objeto,const wchar_t* nome) noexcept {if(objeto&&nome)(void)objeto->SetName(nome);}
inline void marcador(void* lista,const wchar_t* nome) noexcept {
    if(!lista||!nome)return;
    static_cast<ID3D12GraphicsCommandList*>(lista)->SetMarker(0,nome,UINT((std::wcslen(nome)+1)*sizeof(wchar_t)));
}
inline void transicao(ID3D12GraphicsCommandList* lista,ID3D12Resource* recurso,D3D12_RESOURCE_STATES antes,D3D12_RESOURCE_STATES depois) noexcept {
    if(antes==depois)return;
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition={recurso,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,antes,depois};
    lista->ResourceBarrier(1,&b);
}
inline bool raiz_filtro(ID3D12Device* d,unsigned constantes,unsigned srvs,unsigned uavs,Microsoft::WRL::ComPtr<ID3D12RootSignature>& raiz) noexcept {
    D3D12_DESCRIPTOR_RANGE r[2]{};r[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,srvs,0,0,0};r[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,uavs,0,0,srvs};
    D3D12_ROOT_PARAMETER p[2]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[0].Constants={0,0,constantes};
    p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[1].DescriptorTable={2,r};
    D3D12_STATIC_SAMPLER_DESC ss{};ss.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;ss.AddressU=ss.AddressV=ss.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    ss.MaxLOD=D3D12_FLOAT32_MAX;ss.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
    D3D12_ROOT_SIGNATURE_DESC rd{2,p,1,&ss,D3D12_ROOT_SIGNATURE_FLAG_NONE};Microsoft::WRL::ComPtr<ID3DBlob> b,e;
    return SUCCEEDED(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&e))&&
           SUCCEEDED(d->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&raiz)));
}

template<unsigned N,class P=Microsoft::WRL::ComPtr<ID3D12Resource>> struct Aposentados {
    struct Item{P r;unsigned em=0;};
    Item itens[N];unsigned vazados=0;
    bool aposentar(P& r,unsigned agora){
        if(!r)return false;
        for(auto& a:itens)if(!a.r){a.r=std::move(r);a.em=agora;r.Reset();return false;}
        (void)r.Detach();return ++vazados==1;}
    void soltar(unsigned agora,unsigned margem=8u){for(auto& a:itens)if(a.r&&agora-a.em>=margem)a.r.Reset();}
};

template<unsigned N,class P> unsigned livres(const Aposentados<N,P>& a) noexcept {unsigned n=0;for(const auto& i:a.itens)if(!i.r)++n;return n;}
struct Trocas {
    unsigned ultima=0,seguidas=0,total=0;
    bool pode(unsigned agora,unsigned vagas,unsigned precisa,unsigned margem=8u) const noexcept {
        return vagas>=precisa&&(total==0u||agora-ultima>=margem||seguidas<3u);}
    void feita(unsigned agora,unsigned margem=8u) noexcept {seguidas=total&&agora-ultima<margem?seguidas+1u:1u;ultima=agora;++total;}
};
}
