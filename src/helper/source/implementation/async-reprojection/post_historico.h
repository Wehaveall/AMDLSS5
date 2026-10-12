#pragma once

#include "../../../runtime/pre_scale_state.h"
#include <cmath>
#include <cstdint>
namespace nrreproject {
struct Extensao {
    unsigned w{},h{};
    bool operator==(const Extensao& o) const { return w==o.w&&h==o.h; }
};

inline Extensao historico_post(unsigned saida_w,unsigned saida_h,bool post,unsigned chave,std::uint64_t por_pixel,std::uint64_t orcamento,double teto=100.){
    Extensao e{saida_w,saida_h};
    if(!post||!saida_w||!saida_h)return e;

    const double p=nrprescale::post_saida_percent(chave,teto,int(saida_h));

    if(p>0&&p<100){const unsigned a=nrprescale::post_alinhamento(chave);e.w=nrprescale::extensao_rede(int(saida_w),p,a);e.h=nrprescale::extensao_rede(int(saida_h),p,a);
        const int c=nrprescale::campo128.load(std::memory_order_relaxed);e.w=nrprescale::campo_eixo(e.w,c,saida_w);e.h=nrprescale::campo_eixo(e.h,c,saida_h);}
    for(int i=0;i<64&&std::uint64_t(e.w)*e.h*por_pixel>orcamento&&e.w>64&&e.h>64;++i){
        const double s=std::sqrt(double(orcamento)/(double(e.w)*double(e.h)*double(por_pixel)));
        unsigned w=unsigned(double(e.w)*s)&~1u,h=unsigned(double(e.h)*s)&~1u;
        if(w>=e.w)w=e.w-2;
        if(h>=e.h)h=e.h-2;
        e.w=w;e.h=h;
    }
    return e;
}

inline bool vram_insuficiente(bool medido,std::uint64_t budget,std::uint64_t uso){
    return medido&&budget&&uso>=budget;
}

inline std::uint64_t orcamento_dinamico(std::uint64_t teto,std::uint64_t piso,std::uint64_t reserva,std::uint64_t alocado,
                                         bool medido,std::uint64_t budget,std::uint64_t uso){
    if(!medido||!budget)return teto;
    const std::uint64_t livre=budget>uso?budget-uso:0,disponivel=alocado+livre;
    std::uint64_t e=disponivel>reserva?disponivel-reserva:0;
    if(e>teto)e=teto;
    const std::uint64_t piso_efetivo=vram_insuficiente(medido,budget,uso)?piso:(alocado>piso?alocado:piso);
    if(e<piso_efetivo)e=piso_efetivo;
    return e;
}

inline double percent_raio(double efetivo,unsigned saida_w,unsigned historico_w){
    return historico_w&&saida_w!=historico_w?efetivo*double(saida_w)/double(historico_w):efetivo;
}
}
