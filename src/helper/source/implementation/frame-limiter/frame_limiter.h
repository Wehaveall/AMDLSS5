#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
namespace nrlimit {

inline constexpr unsigned opcoes[]={0,60,90,100,110,120,130,144,165};
inline constexpr unsigned opcoes_total=unsigned(sizeof(opcoes)/sizeof(*opcoes));
inline constexpr unsigned minimo=30,maximo=500;

inline std::atomic<unsigned> pedido{0};
inline std::atomic<bool> definido{false};

constexpr unsigned aceitar(long long valor) noexcept {
    return valor>=minimo&&valor<=maximo?unsigned(valor):0u;
}

constexpr unsigned proximo(unsigned atual,int delta) noexcept {
    if(delta<0){

        if(atual==0)return opcoes[opcoes_total-1];
        unsigned escolhido=0;
        for(unsigned i=0;i<opcoes_total;++i)if(opcoes[i]<atual)escolhido=opcoes[i];
        return escolhido;
    }
    for(unsigned i=0;i<opcoes_total;++i)if(opcoes[i]>atual)return opcoes[i];
    return opcoes[0];
}

struct Agenda {
    static constexpr long long pausa_minima_ns=100'000'000;
    long long prazo=0,retorno=0;
    unsigned fps=0;
    bool armado=false,rearmou=false;
    static constexpr long long periodo_ns(unsigned f) noexcept { return f?1'000'000'000LL/f:0; }

    long long planejar(long long agora,unsigned pedido_fps) noexcept {
        rearmou=false;
        if(!pedido_fps){armado=false;fps=0;return agora;}
        const long long periodo=periodo_ns(pedido_fps);
        if(!armado||pedido_fps!=fps||agora-retorno>(std::max)(pausa_minima_ns,4*periodo)){
            armado=true;rearmou=true;fps=pedido_fps;prazo=agora;retorno=agora;return agora;
        }
        prazo=(std::max)(prazo+periodo,agora-periodo);
        retorno=(std::max)(agora,prazo);
        return prazo;
    }
};

struct Folga {
    static constexpr unsigned capacidade=64,minimo_amostras=8;
    long long amostras[capacidade]{};
    unsigned n=0,i=0;
    void registrar(long long sobra) noexcept { amostras[i]=sobra<0?0:sobra; i=(i+1)%capacidade; if(n<capacidade)++n; }
    void limpar() noexcept { n=i=0; }
    long long valor(long long periodo) const noexcept {
        if(n<minimo_amostras)return 0;
        long long copia[capacidade]{};
        for(unsigned k=0;k<n;++k)copia[k]=amostras[k];
        const unsigned alvo=(n*9)/10;
        std::nth_element(copia,copia+alvo,copia+n);
        return (std::min)(copia[alvo],periodo/4);
    }
};

inline std::atomic<unsigned> modo{0};

void ritmar(void* base) noexcept;
void depois() noexcept;

long long agora_ns() noexcept;
void esperar_ate(long long prazo_ns) noexcept;
struct Contagem {unsigned long long quadros,atrasados,esperados,pulados,alinhados;double espera_ms,espera_depois_ms,folga_ms;unsigned modo;bool alta_resolucao;};
Contagem contagem() noexcept;
}
