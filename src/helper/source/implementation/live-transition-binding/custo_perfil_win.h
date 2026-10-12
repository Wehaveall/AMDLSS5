#pragma once

#include "../../../runtime/custo_perfil.h"
#include "../../../runtime/log_fila.h"
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
namespace nrcustowin {
using Microsoft::WRL::ComPtr;
struct Perfil {
    static constexpr unsigned Anel = 16, P = nrcusto::Pontos, NCat = 2 * nrcusto::Cats;
    std::mutex m;
    ComPtr<ID3D12Device> dev;ComPtr<ID3D12QueryHeap> heap;ComPtr<ID3D12Resource> rb;volatile std::uint64_t* map{};bool falhou{};
    double ms_por_tique{};
    struct Quadro {bool pendente{};std::uint64_t n{};unsigned mascara{},cat{};bool tardia{};};
    Quadro q[Anel];std::uint64_t contador{};int cur{-1};void* lista{};
    double soma[NCat][P]{};std::uint64_t cnt[NCat][P]{};double mod[NCat]{};std::uint64_t nmod[NCat]{};std::uint64_t quadros[NCat]{};std::uint64_t lidos{};
    bool criar(ID3D12GraphicsCommandList* l){
        if(heap)return true;if(falhou)return false;
        ComPtr<ID3D12Device> d;if(FAILED(l->GetDevice(IID_PPV_ARGS(&d)))){falhou=true;return false;}
        D3D12_QUERY_HEAP_DESC hd{};hd.Type=D3D12_QUERY_HEAP_TYPE_TIMESTAMP;hd.Count=Anel*P;
        D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=UINT64(Anel)*P*8u;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        void* p{};
        if(FAILED(d->CreateQueryHeap(&hd,IID_PPV_ARGS(&heap)))||FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&rb)))||
           FAILED(rb->Map(0,nullptr,&p))||!p){heap.Reset();rb.Reset();falhou=true;return false;}
        map=static_cast<volatile std::uint64_t*>(p);dev=d;
        nrlogfila::linha(L"nr_reproject.log","pid=%lu CUSTO_FIXO ligado (NeuralCustoPerfil=1): %u pontos por quadro, ms_por_tique=%.9f\n",GetCurrentProcessId(),P,ms_por_tique);
        return true;
    }
    void processar(unsigned s){
        auto& f=q[s];f.pendente=false;
        if(!(f.mascara&(1u<<nrcusto::CapInicio)))return;
        const unsigned c=f.cat+(f.tardia?nrcusto::Cats:0u);
        std::uint64_t t[P]{};bool ok[P]{};
        for(unsigned p=0;p<P;++p)if(f.mascara&(1u<<p)){t[p]=map[s*P+p];ok[p]=t[p]!=0;}
        int ant=-1;
        for(unsigned p=0;p<P;++p){if(!ok[p])continue;
            if(ant>=0&&t[p]>=t[ant]){soma[c][p]+=double(t[p]-t[ant])*ms_por_tique;++cnt[c][p];}
            ant=int(p);}
        if(ok[nrcusto::CapInicio]&&ok[nrcusto::DispFim]&&t[nrcusto::DispFim]>=t[nrcusto::CapInicio]){mod[c]+=double(t[nrcusto::DispFim]-t[nrcusto::CapInicio])*ms_por_tique;++nmod[c];}
        ++quadros[c];
        if(++lidos%1024u==0u)escrever();
    }
    void escrever(){
        static const char* nomes[P]{"disp","fsr","begin","nat_pre","reduzir","warp","fill","estab","cadeia","compose","nat_pos","depois"};
        static const char* cats[NCat]{"V","O","J","Vt","Ot","Jt"};
        for(unsigned c=0;c<NCat;++c){if(!quadros[c])continue;
            char linha[900];int u=std::snprintf(linha,sizeof linha,"pid=%lu CUSTO_FIXO cat=%s n=%llu mod=%.4f",GetCurrentProcessId(),cats[c],
                static_cast<unsigned long long>(quadros[c]),nmod[c]?mod[c]/double(nmod[c]):-1.0);
            for(unsigned p=1;p<P&&u>0&&unsigned(u)<sizeof linha;++p){
                const int w=std::snprintf(linha+u,sizeof linha-unsigned(u)," %s=%.4f/%llu",nomes[p],cnt[c][p]?soma[c][p]/double(cnt[c][p]):-1.0,static_cast<unsigned long long>(cnt[c][p]));
                if(w>0)u+=w;}
            nrlogfila::linha(L"nr_reproject.log","%s\n",linha);}
        std::memset(soma,0,sizeof soma);std::memset(cnt,0,sizeof cnt);std::memset(mod,0,sizeof mod);std::memset(nmod,0,sizeof nmod);std::memset(quadros,0,sizeof quadros);
    }
    void marca(void* lista_v,unsigned ponto) noexcept {
        if(!lista_v||ponto>=P)return;
        try{
            std::lock_guard<std::mutex> trava(m);
            auto* l=static_cast<ID3D12GraphicsCommandList*>(lista_v);
            if(ponto==nrcusto::DispInicio){
                if(l->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT&&l->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE){cur=-1;return;}
                if(!criar(l)){cur=-1;return;}
                ++contador;
                for(unsigned s=0;s<Anel;++s)if(q[s].pendente&&q[s].n+8u<=contador)processar(s);
                const unsigned s=unsigned(contador%Anel);
                if(q[s].pendente)q[s].pendente=false;
                q[s]={false,contador,0u,nrcusto::Ocupada,false};cur=int(s);lista=lista_v;
            }else if(cur<0||lista_v!=lista)return;
            auto& f=q[unsigned(cur)];
            if(f.mascara&(1u<<ponto))return;
            const unsigned i=unsigned(cur)*P+ponto;
            map[i]=0;
            l->EndQuery(heap.Get(),D3D12_QUERY_TYPE_TIMESTAMP,i);
            l->ResolveQueryData(heap.Get(),D3D12_QUERY_TYPE_TIMESTAMP,i,1,rb.Get(),UINT64(i)*8u);
            f.mascara|=1u<<ponto;
            if(ponto==nrcusto::DispFim){f.pendente=true;cur=-1;lista=nullptr;}
        }catch(...){}
    }
    void categoria(unsigned c) noexcept {std::lock_guard<std::mutex> trava(m);if(cur>=0&&c<nrcusto::Cats)q[unsigned(cur)].cat=c;}
    void tardia() noexcept {std::lock_guard<std::mutex> trava(m);if(cur>=0)q[unsigned(cur)].tardia=true;}
};
inline Perfil& perfil() noexcept {static Perfil* p=new Perfil();return *p;}
inline void marca_cb(void* l,unsigned p) noexcept {perfil().marca(l,p);}
inline void cat_cb(unsigned c) noexcept {perfil().categoria(c);}
inline void tardia_cb(unsigned) noexcept {perfil().tardia();}

inline void instalar(ID3D12CommandQueue* fila) noexcept {
    static bool feito=false;if(feito||!fila)return;feito=true;
    UINT64 f=0;if(FAILED(fila->GetTimestampFrequency(&f))||!f)return;
    perfil().ms_por_tique=1000.0/double(f);
    nrcusto::marca_fn().store(&marca_cb);nrcusto::cat_fn().store(&cat_cb);nrcusto::tardia_fn().store(&tardia_cb);
}
}
