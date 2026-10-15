#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>

namespace nrpainel {
inline std::atomic<int> jobs{-1};
inline std::atomic<int> mediana_ms{-1};
inline std::atomic<long long> present_ant{0};
inline std::atomic<long long> present_max{0};
inline std::atomic<bool> sonda{false};

struct Janela {
    std::array<std::uint32_t,31> v{};unsigned n{0},pos{0};int ultimo_job{-1};

    int empurra(int job,std::uint32_t ms,unsigned tam) noexcept {
        tam=std::clamp(tam|1u,3u,31u);
        if(job!=ultimo_job){ultimo_job=job;v[pos]=ms;pos=(pos+1)%31u;if(n<31u)++n;}
        const unsigned k=std::min(n,tam);if(!k)return -1;
        std::array<std::uint32_t,31> t{};
        for(unsigned i=0;i<k;++i)t[i]=v[(pos+31u-1u-i)%31u];
        std::nth_element(t.begin(),t.begin()+k/2,t.begin()+k);
        return int(t[k/2]);
    }
};
inline Janela& janela() noexcept {static Janela j;return j;}

inline void no_present(long long agora) noexcept {
    const long long ant=present_ant.exchange(agora,std::memory_order_relaxed);
    if(ant<=0||agora<=ant)return;
    const long long d=agora-ant;long long m=present_max.load(std::memory_order_relaxed);
    while(d>m&&!present_max.compare_exchange_weak(m,d,std::memory_order_relaxed)){}
}
}
