#pragma once

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <deque>

namespace nrrjcam {
struct Reg{std::uint64_t q,epoca;int inst,x0,y0,x1,y1;};

inline constexpr std::size_t fila_max=16384;

inline constexpr float margem_max=16777216.f;
struct Fila{
    std::deque<Reg> v;
    std::uint64_t epoca_vista{},n{},cortes{},podados_teto{};
    void limpar() noexcept {v.clear();}
    void por(const Reg& r){
        if(v.size()>=fila_max){v.pop_front();++podados_teto;}
        v.push_back(r);++n;}

    bool cortar_epoca(std::uint64_t epoca){
        if(epoca==epoca_vista)return false;
        const bool corte=epoca_vista!=0;epoca_vista=epoca;
        v.erase(std::remove_if(v.begin(),v.end(),[&](const Reg& r){return r.epoca!=epoca;}),v.end());
        if(corte)++cortes;
        return corte;}

    void podar(std::uint64_t q,int zera){
        while(!v.empty()&&v.front().q+std::uint64_t(zera)<q)v.pop_front();}
};

inline constexpr int inst_max=16;
inline void regiao(const Fila& F,std::uint64_t q,float margem,int ow,int oh,int b[4],bool limitada=false){
    b[0]=b[1]=INT_MAX;b[2]=b[3]=INT_MIN;
    std::uint64_t novo[inst_max]{};bool tem[inst_max]{};
    if(limitada)for(const auto& r:F.v){if(r.q>q)continue;const int i=(std::clamp)(r.inst,0,inst_max-1);if(!tem[i]||r.q>novo[i]){novo[i]=r.q;tem[i]=true;}}
    for(const auto& r:F.v){
        if(r.q>q)continue;
        int m;
        if(!limitada)m=int((std::min)(64.f+margem*float(q-r.q),margem_max));
        else if(r.q!=novo[(std::clamp)(r.inst,0,inst_max-1)])m=64;
        else{const float teto=float((std::max)(2*(std::max)(r.x1-r.x0,r.y1-r.y0),256));
            m=int((std::min)(64.f+margem*float(q-r.q),(std::max)(teto,64.f)));}
        b[0]=(std::min)(b[0],r.x0-m);b[1]=(std::min)(b[1],r.y0-m);b[2]=(std::max)(b[2],r.x1+m);b[3]=(std::max)(b[3],r.y1+m);}
    b[0]=(std::max)(b[0],0);b[1]=(std::max)(b[1],0);b[2]=(std::min)(b[2],ow);b[3]=(std::min)(b[3],oh);
    if(b[0]>=b[2]||b[1]>=b[3]){b[0]=b[1]=b[2]=b[3]=0;}
}
}
