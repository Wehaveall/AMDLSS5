#pragma once

#include <atomic>
#include <cstdint>
namespace nrredecada {
constexpr unsigned maximo=12;

constexpr unsigned valores[]{1u,2u,3u,4u,6u,8u,10u};
constexpr unsigned valores_n=unsigned(sizeof(valores)/sizeof(*valores));
inline std::atomic<unsigned> pedido{1};
inline std::atomic<bool> definido{false};
inline std::atomic<std::uint64_t> vetos{0};
inline std::atomic<std::uint64_t> jobs{0};
inline unsigned aceitar(long long v) noexcept {return v<=1?1u:v>=maximo?maximo:unsigned(v);}

inline bool vetar(unsigned n,std::uint64_t quadro_agora,std::uint64_t quadro_ultimo_job,bool tem_ultimo) noexcept {
    if(n<=1||!tem_ultimo||quadro_agora<quadro_ultimo_job)return false;
    return quadro_agora-quadro_ultimo_job<n;
}

namespace cam {
inline std::atomic<unsigned> par{0},mov{0},segura{16};
inline std::atomic<float> px{.5f},frac{.5f};
inline std::atomic<int> estado{-1};
inline std::atomic<std::uint64_t> medidos{0},andando{0},trocas{0},q_par{0},q_mov{0};

struct Decisor {
    std::uint64_t ult_mov{};bool viu_mov{false};unsigned atual{0};
    unsigned n(unsigned base,std::uint64_t quadro,int est,unsigned n_par,unsigned n_mov,unsigned seg) noexcept {
        if(n_par<=1)return base;
        const unsigned nm=n_mov>=1?n_mov:base;
        if(est<0&&!viu_mov)return nm;
        if(est>0){ult_mov=quadro;viu_mov=true;}
        const bool anda=viu_mov&&quadro>=ult_mov&&quadro-ult_mov<seg;
        return anda?nm:n_par;
    }
};
inline bool anda(float f,float fr) noexcept {return f>=0.f&&f>fr;}
}

inline unsigned proximo(unsigned atual,int delta) noexcept {
    unsigned i=0;for(unsigned k=0;k<valores_n;++k)if(valores[k]==atual)i=k;
    i=delta<0?(i+valores_n-1)%valores_n:(i+1)%valores_n;
    return valores[i];
}

constexpr long sentinela=0x7FFFFFF0L;
struct Trava {
    volatile long* alvo{};bool armada{};

    bool armar(volatile long* job_pendente,int worker_job,std::uint64_t lista_pendente,unsigned reset_pendente) noexcept;
    void soltar() noexcept;
    ~Trava(){soltar();}
};
}
#if defined(_MSC_VER)
#include <intrin.h>
inline bool nrredecada::Trava::armar(volatile long* job_pendente,int worker_job,std::uint64_t lista_pendente,unsigned reset_pendente) noexcept {
    if(armada||!job_pendente||worker_job>=0||lista_pendente||reset_pendente)return false;
    if(_InterlockedCompareExchange(job_pendente,sentinela,-1L)!=-1L)return false;
    alvo=job_pendente;armada=true;return true;
}
inline void nrredecada::Trava::soltar() noexcept {
    if(!armada)return;armada=false;

    (void)_InterlockedCompareExchange(alvo,-1L,sentinela);alvo=nullptr;
}
#endif
