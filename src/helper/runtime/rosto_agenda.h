#pragma once

#include <algorithm>
#include <cstdint>

namespace nrag {
struct Cand {bool quer{};bool forcado{};float cab4k{};float custo_ms{};unsigned jobs_sem{};};
struct Plano {std::uint32_t bits{};float usado_ms{};unsigned querem{},escolhidos{},forcados{},grandes{},adiados{};};

inline float custo_tabela(unsigned n) noexcept {
    if(n<=384u)return 5.5f;if(n<=512u)return 6.f;if(n<=768u)return 8.5f;if(n<=1024u)return 11.f;if(n<=1152u)return 14.f;
    if(n<=1280u)return 18.f;if(n<=1536u)return 20.f;return 25.f;}

inline Plano planejar(const Cand* c,unsigned n,float orcamento_ms,float grande_px) noexcept {
    Plano p{};n=(std::min)(n,32u);
    if(!(orcamento_ms>0.f)){for(unsigned i=0;i<n;++i)if(c[i].quer){p.bits|=1u<<i;++p.querem;++p.escolhidos;p.usado_ms+=(std::max)(c[i].custo_ms,0.f);}return p;}
    unsigned ord[32];unsigned m=0;
    for(unsigned i=0;i<n;++i){if(!c[i].quer)continue;++p.querem;
        const bool grande=grande_px>0.f&&c[i].cab4k>=grande_px;
        if(c[i].forcado||grande){p.bits|=1u<<i;++p.escolhidos;p.usado_ms+=(std::max)(c[i].custo_ms,0.f);if(c[i].forcado)++p.forcados;else ++p.grandes;}
        else ord[m++]=i;}

    auto pri=[&](unsigned i){return (std::max)(c[i].cab4k,1.f)*float(1u+c[i].jobs_sem);};
    std::sort(ord,ord+m,[&](unsigned a,unsigned b){const float x=pri(a),y=pri(b);return x!=y?x>y:a<b;});
    for(unsigned k=0;k<m;++k){const unsigned i=ord[k];const float cu=(std::max)(c[i].custo_ms,0.f);
        if(p.usado_ms+cu<=orcamento_ms){p.bits|=1u<<i;++p.escolhidos;p.usado_ms+=cu;}else ++p.adiados;}
    return p;}

inline bool forcado(bool geracao_nova,bool nunca,unsigned jobs_sem,unsigned kmax) noexcept {
    return geracao_nova||nunca||jobs_sem+1u>=(std::max)(kmax,1u);}

inline float custo_ema(float ant,float ms) noexcept {if(!(ms>0.f))return ant;return ant>0.f?.8f*ant+.2f*ms:ms;}
}
