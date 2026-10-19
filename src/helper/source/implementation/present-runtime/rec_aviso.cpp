#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../../../runtime/nr_publico.h"
#include "rec_aviso.h"
#include "../async-reprojection/reproject_controls.h"
#include "../../../runtime/overlay_amd.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace nraviso {
namespace {
using Microsoft::WRL::ComPtr;

struct Glifo {char c;unsigned char l[7];};
constexpr Glifo fonte[]={
 {'A',{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},{'B',{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},{'C',{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
 {'D',{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}},{'E',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},{'F',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
 {'G',{0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}},{'H',{0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},{'I',{0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}},
 {'J',{0x07,0x02,0x02,0x02,0x02,0x12,0x0C}},{'K',{0x11,0x12,0x14,0x18,0x14,0x12,0x11}},{'L',{0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
 {'M',{0x11,0x1B,0x15,0x15,0x11,0x11,0x11}},{'N',{0x11,0x11,0x19,0x15,0x13,0x11,0x11}},{'O',{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
 {'P',{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},{'Q',{0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}},{'R',{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
 {'S',{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},{'T',{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},{'U',{0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
 {'V',{0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},{'W',{0x11,0x11,0x11,0x15,0x15,0x15,0x0A}},{'X',{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}},
 {'Y',{0x11,0x11,0x11,0x0A,0x04,0x04,0x04}},{'Z',{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},
 {'0',{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},{'1',{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},{'2',{0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
 {'3',{0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}},{'4',{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},{'5',{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}},
 {'6',{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}},{'7',{0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},{'8',{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},
 {'9',{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}},{'/',{0x01,0x01,0x02,0x04,0x08,0x10,0x10}},{'(',{0x02,0x04,0x08,0x08,0x08,0x04,0x02}},
 {')',{0x08,0x04,0x02,0x02,0x02,0x04,0x08}},{'.',{0,0,0,0,0,0x0C,0x0C}},{'-',{0,0,0,0x1F,0,0,0}},{':',{0,0x0C,0x0C,0,0x0C,0x0C,0}},{'+',{0,0x04,0x04,0x1F,0x04,0x04,0}},
};
const unsigned char* glifo(char c) noexcept {
    if(c>='a'&&c<='z')c=char(c-'a'+'A');
    for(const auto& g:fonte)if(g.c==c)return g.l;
    return nullptr;
}

struct Desenho {
    ComPtr<ID3D12Device> device;ComPtr<ID3D12DescriptorHeap> heap;ComPtr<ID3D12Fence> fence;
    ComPtr<ID3D12CommandAllocator> alloc[3];ComPtr<ID3D12GraphicsCommandList> list[3];UINT64 valor[3]{};UINT64 proximo{1};
    D3D12_COMMAND_LIST_TYPE tipo{};unsigned n{};bool falhou{};
};
Desenho& d(){static auto* x=new Desenho;return *x;}
void log_uma(const char* t) noexcept {
(void)t;
}
}
void texto(const char* t,int x0,int y0,int s,std::vector<Ret>& out){
    int x=x0;
    for(const char* c=t;*c;++c,x+=6*s){
        const unsigned char* g=glifo(*c);if(!g)continue;
        for(int r=0;r<7;++r){int a=-1;
            for(int k=0;k<=5;++k){const bool on=k<5&&((g[r]>>(4-k))&1u);
                if(on&&a<0)a=k;
                if(!on&&a>=0){out.push_back({x+a*s,y0+r*s,(k-a)*s,s});a=-1;}}}
    }
}
int largura(const char* t,int s) noexcept {const int n=int(std::strlen(t));return n>0?n*6*s-s:0;}
int estado(std::uint64_t agora,char* linha,unsigned cap) noexcept {
    using namespace nrreproject;
    static bool ativo=false;static std::uint64_t gravado_ate=0;static unsigned n_gravado=0;
    static unsigned n0=0;
    if(!gravar_aviso.load(std::memory_order_relaxed))return 0;
    const unsigned falta=despejo_final.load(),pedido=(std::max)(1u,final_pedido.load()),grav=final_gravados.load(),total=final_total.load();
    if(aviso_cheio_ate_ms.load()>agora){NRLOGF(linha,cap,"GRAVACAO CHEIA %u/%u - REABRA O JOGO",total,final_teto_sessao.load());return 3;}
    if(falta>0){if(!ativo)n0=0;ativo=true;if(falta>n0)n0=falta;const unsigned N=(std::max)(pedido,n0);const unsigned k=N-falta;NRLOGF(linha,cap,"GRAVANDO %u/%u  (%u/%u)",k,N,total,final_teto_sessao.load());return 1;}
    if(ativo){
        if(grav>=(std::max)(pedido,n0)||final_cheio.load()){ativo=false;gravado_ate=agora+2000;n_gravado=grav;}
        else{NRLOGF(linha,cap,"SALVANDO %u/%u",grav,(std::max)(pedido,n0));return 1;}
    }
    if(gravado_ate>agora){NRLOGF(linha,cap,"GRAVADO %u QUADROS (%u/%u)",n_gravado,total,final_teto_sessao.load());return 2;}
    return 0;
}
void desenhar(void* opaque_swap,void* opaque_queue) noexcept {

    char linha[96]{};int tipo=nroverlayamd::linha_aviso(GetTickCount64(),linha,sizeof linha);
    if(!tipo){if(!nrreproject::gravar_aviso.load(std::memory_order_relaxed))return;tipo=estado(GetTickCount64(),linha,sizeof linha);}
    if(!tipo||!opaque_swap||!opaque_queue)return;
    auto& D=d();if(D.falhou)return;
    try{
        ComPtr<IDXGISwapChain3> swap;ComPtr<ID3D12CommandQueue> queue;
        if(FAILED(static_cast<IUnknown*>(opaque_swap)->QueryInterface(IID_PPV_ARGS(&swap)))||FAILED(static_cast<IUnknown*>(opaque_queue)->QueryInterface(IID_PPV_ARGS(&queue))))return;
        ComPtr<ID3D12Resource> bb;if(FAILED(swap->GetBuffer(swap->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&bb)))||!bb)return;
        ComPtr<ID3D12Device> dev;if(FAILED(bb->GetDevice(IID_PPV_ARGS(&dev))))return;
        const auto tq=queue->GetDesc().Type;
        if(tq!=D3D12_COMMAND_LIST_TYPE_DIRECT)return;
        if(D.device.Get()!=dev.Get()||D.tipo!=tq){
            Desenho novo{};novo.device=dev;novo.tipo=tq;
            D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=3;
            if(FAILED(dev->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&novo.heap)))||FAILED(dev->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&novo.fence)))){D.falhou=true;log_uma("aviso falhou: heap/cerca");return;}
            for(unsigned i=0;i<3;++i)if(FAILED(dev->CreateCommandAllocator(tq,IID_PPV_ARGS(&novo.alloc[i])))||FAILED(dev->CreateCommandList(0,tq,novo.alloc[i].Get(),nullptr,IID_PPV_ARGS(&novo.list[i])))||FAILED(novo.list[i]->Close())){D.falhou=true;log_uma("aviso falhou: listas");return;}
            if(D.device&&D.fence){
                for(auto& a:D.alloc)(void)a.Detach();for(auto& l:D.list)(void)l.Detach();(void)D.fence.Detach();}
            D=std::move(novo);log_uma("aviso pronto");
        }
        const unsigned k=D.n%3u;
        if(D.fence->GetCompletedValue()<D.valor[k])return;
        const auto bd=bb->GetDesc();
        if(bd.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||bd.SampleDesc.Count!=1||!(bd.Flags&D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET))return;
        const UINT inc=D.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtv=D.heap->GetCPUDescriptorHandleForHeapStart();rtv.ptr+=SIZE_T(k)*inc;
        D.device->CreateRenderTargetView(bb.Get(),nullptr,rtv);
        const int H=int(bd.Height),W=int(bd.Width);const int s=(std::max)(2,H/300);const int m=(std::max)(8,H/40);
        std::vector<Ret> rs;const int tx=m+(tipo==1?10*s:0);texto(linha,tx,m,s,rs);
        const int larg=largura(linha,s)+(tipo==1?10*s:0);
        D3D12_RECT fundo{m-2*s,m-2*s,(std::min)(W,m+larg+2*s),(std::min)(H,m+9*s)};
        std::vector<D3D12_RECT> rr;rr.reserve(rs.size()+8);
        for(const auto& r:rs){D3D12_RECT q{r.x,r.y,(std::min)(W,r.x+r.w),(std::min)(H,r.y+r.h)};if(q.right>q.left&&q.bottom>q.top)rr.push_back(q);}
        if(tipo==1){const int cx=m+3*s,cy=m+3*s,R=3*s;
            for(int y=-R;y<=R;y+=s){int w=0;while((w+s)*(w+s)+y*y<=R*R)w+=s;rr.push_back({cx-w,cy+y,cx+w+s,cy+y+s});}}
        auto& L=D.list[k];
        if(FAILED(D.alloc[k]->Reset())||FAILED(L->Reset(D.alloc[k].Get(),nullptr)))return;
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={bb.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET};
        L->ResourceBarrier(1,&b);
        const float preto[4]{0.f,0.f,0.f,1.f};
        const float cor[3][4]{{1.f,.12f,.12f,1.f},{.15f,1.f,.25f,1.f},{1.f,.85f,.1f,1.f}};
        if(fundo.right>fundo.left&&fundo.bottom>fundo.top)L->ClearRenderTargetView(rtv,preto,1,&fundo);
        if(!rr.empty())L->ClearRenderTargetView(rtv,cor[tipo-1],UINT(rr.size()),rr.data());
        std::swap(b.Transition.StateBefore,b.Transition.StateAfter);L->ResourceBarrier(1,&b);
        if(FAILED(L->Close()))return;
        ID3D12CommandList* ls[]{L.Get()};queue->ExecuteCommandLists(1,ls);
        const UINT64 v=D.proximo++;if(SUCCEEDED(queue->Signal(D.fence.Get(),v)))D.valor[k]=v;else{D.falhou=true;log_uma("aviso falhou: sinal");}
        ++D.n;
    }catch(...){D.falhou=true;}
}
}
