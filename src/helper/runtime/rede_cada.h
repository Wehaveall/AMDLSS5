#pragma once

#include <atomic>
#include <cstdint>
namespace nrredecada {

constexpr unsigned maximo=12;
constexpr unsigned padrao=6;

constexpr unsigned valores[]{1u,2u,3u,4u,6u,8u,10u,12u};
constexpr unsigned valores_n=unsigned(sizeof(valores)/sizeof(*valores));
inline std::atomic<unsigned> pedido{1};
inline std::atomic<bool> definido{false};

inline std::atomic<unsigned> efetiva{0};

inline unsigned cadencia_efetiva() noexcept {
    const unsigned e=efetiva.load(std::memory_order_relaxed);if(e)return e;
    return definido.load(std::memory_order_acquire)?pedido.load(std::memory_order_relaxed):padrao;
}
inline std::atomic<std::uint64_t> vetos{0};
inline std::atomic<std::uint64_t> jobs{0};

inline unsigned aceitar(long long v) noexcept {return v<=0?padrao:v==1?1u:v>=maximo?maximo:unsigned(v);}
static_assert(padrao>=1&&padrao<=maximo&&maximo<16,"o ciclo inteiro dentro das 16 fatias do historico");

inline bool vetar(unsigned n,std::uint64_t quadro_agora,std::uint64_t quadro_ultimo_job,bool tem_ultimo) noexcept {
    if(n<=1||!tem_ultimo||quadro_agora<quadro_ultimo_job)return false;
    return quadro_agora-quadro_ultimo_job<n;
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
