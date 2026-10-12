#pragma once
#include <cstdint>

namespace nrretry {
struct Limits {
    unsigned max_attempts{3};
    std::uint64_t wait_ms{0};
    std::uint64_t ceiling_ms{30000};
};
enum class Verdict { Retry, GiveUp };
class Policy {
public:
    constexpr explicit Policy(Limits limits={})noexcept:limits_(limits){}

    Verdict failure(std::uint64_t now)noexcept{
        if(exhausted_)return Verdict::GiveUp;
        if(!attempts_)first_=now;
        last_=now;
        if(attempts_!=UINT32_MAX)++attempts_;
        if((limits_.max_attempts&&attempts_>limits_.max_attempts)||
           (limits_.ceiling_ms&&now>=first_&&now-first_>=limits_.ceiling_ms)){exhausted_=true;return Verdict::GiveUp;}
        return Verdict::Retry;
    }

    bool ready(std::uint64_t now)const noexcept{
        if(exhausted_)return false;
        if(!attempts_)return true;
        return now<last_||now-last_>=limits_.wait_ms;
    }

    void success()noexcept{attempts_=0;first_=last_=0;exhausted_=false;}
    bool pending()const noexcept{return attempts_!=0;}
    bool exhausted()const noexcept{return exhausted_;}
    unsigned attempts()const noexcept{return attempts_;}

    std::uint64_t elapsed(std::uint64_t now)const noexcept{return attempts_&&now>=first_?now-first_:0;}
    const Limits& limits()const noexcept{return limits_;}
private:
    Limits limits_;
    unsigned attempts_{};
    std::uint64_t first_{},last_{};
    bool exhausted_{};
};
}
