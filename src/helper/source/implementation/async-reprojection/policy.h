#pragma once
#include <array>
#include <cstdint>
namespace nrreproject {

struct Policy {

    static constexpr unsigned history_count=16;
    struct Job { int id{}; std::uint64_t frame{}; };
    std::uint64_t frame{};
    std::array<std::uint64_t,history_count> frames{};
    std::array<Job,history_count*2> jobs{};
    bool captured{};
    void invalidate() noexcept { frames={};jobs={};captured=false; }
    unsigned begin() noexcept { captured=true;++frame;frames[frame%history_count]=frame;return unsigned(frame%history_count); }
    void accepted(int job) noexcept { if(captured&&job>0){auto& slot=jobs[unsigned(job)%jobs.size()];if(slot.id!=job)slot={job,frame};} }
    int age(int job) const noexcept {
        if(!captured||job<=0)return -1;
        const auto& j=jobs[unsigned(job)%jobs.size()];
        if(j.id!=job||!j.frame||j.frame>frame||frame-j.frame>=history_count)return -1;
        for(auto f=j.frame;f<=frame;++f)if(frames[f%history_count]!=f)return -1;
        return int(frame-j.frame);
    }

    int long_age(int job) const noexcept {
        if(!captured||job<=0)return -1;
        const auto& j=jobs[unsigned(job)%jobs.size()];
        if(j.id!=job||!j.frame||j.frame>frame)return -1;
        return int(frame-j.frame);
    }
    unsigned mask() const noexcept {unsigned m=0;for(unsigned i=0;i<history_count;++i)if(frames[i]&&frames[i]<=frame&&frame-frames[i]<history_count)m|=1u<<i;return m;}
};
}
