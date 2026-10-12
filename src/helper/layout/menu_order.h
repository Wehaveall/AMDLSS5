#pragma once
#include <cstddef>
#include <vector>

namespace nrmenulayout {
struct Order {
    std::vector<unsigned> display_to_native;
    std::vector<int> native_to_display;
};

inline Order make_order(const std::vector<int>& rank,const std::vector<bool>& shown) {
    Order o;const std::size_t n=rank.size();o.native_to_display.assign(n,-1);
    std::vector<unsigned> idx;
    for(unsigned i=0;i<n;++i)if(i>=shown.size()||shown[i])idx.push_back(i);

    for(std::size_t a=1;a<idx.size();++a){
        const unsigned v=idx[a];std::size_t b=a;
        auto key=[&](unsigned i){return rank[i]<0?(long long)(1ll<<40)+i:(long long)rank[i]*4096+i;};
        while(b>0&&key(idx[b-1])>key(v)){idx[b]=idx[b-1];--b;}
        idx[b]=v;
    }
    o.display_to_native=idx;
    for(std::size_t d=0;d<idx.size();++d)o.native_to_display[idx[d]]=int(d);
    return o;
}
class Navigator {
    std::size_t count_=0;const void* identity_=nullptr;int last_=-1;
public:
    void reset() noexcept { count_=0;identity_=nullptr;last_=-1; }

    unsigned update(const Order& o,unsigned native,const void* identity) {
        const std::size_t n=o.native_to_display.size();
        if(n!=count_||identity!=identity_){count_=n;identity_=identity;last_=-1;}
        if(o.display_to_native.empty()||!n){last_=-1;return native;}
        if(native>=n)native=unsigned(n-1);
        const bool visible=o.native_to_display[native]>=0;
        const std::size_t shown=o.display_to_native.size();
        auto settle=[&](unsigned target){last_=int(target);return target;};
        if(last_<0||unsigned(last_)>=n||o.native_to_display[unsigned(last_)]<0)
            return settle(visible?native:o.display_to_native.front());
        const unsigned last=unsigned(last_);
        if(native==last)return settle(visible?native:o.display_to_native.front());
        int step=0;
        if(native==(last+1)%n)step=1;
        else if(native==(last+n-1)%n)step=-1;
        if(!step)return settle(visible?native:o.display_to_native.front());
        const std::size_t p=std::size_t(o.native_to_display[last]);
        const std::size_t q=(p+shown+std::size_t(step>0?1:shown-1))%shown;
        return settle(o.display_to_native[q]);
    }
};
}
