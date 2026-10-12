#include "settings.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <fstream>
#include <iterator>
#include <limits>
#include <vector>
#ifndef _WIN32
#error This persistence implementation requires Windows.
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../../../runtime/nr_publico.h"

namespace nr {
namespace {
constexpr std::array<unsigned,7> percentages{100,85,70,65,50,40,30};
constexpr std::array<const char*,7> texts{"1.00","0.85","0.70","0.65","0.50","0.40","0.30"};
constexpr const char* section_name = "dlssnronamd";
bool valid(unsigned percent) { return std::find(percentages.begin(),percentages.end(),percent) != percentages.end(); }
bool parse_percent(const std::string& value,unsigned& percent) {
    if(value.empty()) return false;

    auto dot=value.find('.');
    auto whole=value.substr(0,dot);
    auto fraction=dot==std::string::npos ? std::string{} : value.substr(dot+1);
    if(whole.empty() && fraction.empty()) return false;
    auto digits=[](const std::string& text) { return text.find_first_not_of("0123456789")==std::string::npos; };
    if(!digits(whole) || !digits(fraction)) return false;
    auto first=whole.find_first_not_of('0');
    whole=first==std::string::npos ? "0" : whole.substr(first);
    while(!fraction.empty() && fraction.back()=='0') fraction.pop_back();
    if(whole=="1" && fraction.empty()) { percent=100; return true; }
    if(whole!="0" || fraction.empty() || fraction.size()>2) return false;
    unsigned candidate=static_cast<unsigned>(fraction[0]-'0')*10;
    if(fraction.size()==2) candidate+=static_cast<unsigned>(fraction[1]-'0');
    if(!valid(candidate)) return false;
    percent=candidate; return true;
}

std::string valor_publico(const std::filesystem::path& ini,const wchar_t* chave,const wchar_t* padrao) {
    wchar_t t[64]{};NRCFG_STR(L"DlssNrOnAmd",chave,padrao,t,64,ini.c_str());
    std::string s;for(const wchar_t* c=t;*c;++c)s.push_back(*c<128?char(*c):'?');
    while(!s.empty()&&(s.back()==' '||s.back()=='\t'))s.pop_back();
    while(!s.empty()&&(s.front()==' '||s.front()=='\t'))s.erase(s.begin());
    return s;
}
bool ler_publico(const std::filesystem::path& ini,Settings& requested,std::string& error) {
    Settings candidate{};
    const auto pre=valor_publico(ini,L"PreUpscaleNR",L"");
    if(!pre.empty()){if(pre!="0"&&pre!="1"){error="1";return false;}candidate.pre_upscale=pre=="1";}
    const auto escala=valor_publico(ini,L"NeuralResolutionScale",L"");
    if(!escala.empty()&&!parse_percent(escala,candidate.percent)){error="2";return false;}
    requested=candidate;return true;
}
bool gravar_publico(const std::filesystem::path& ini,Settings value,std::string& error) {
    const auto index=static_cast<std::size_t>(std::find(percentages.begin(),percentages.end(),value.percent)-percentages.begin());
    wchar_t escala[8]{};for(std::size_t i=0;texts[index][i]&&i<7;++i)escala[i]=wchar_t(texts[index][i]);
    if(!NRCFG_ESCREVER(L"DlssNrOnAmd",L"PreUpscaleNR",value.pre_upscale?L"1":L"0",ini.c_str())||
       !NRCFG_ESCREVER(L"DlssNrOnAmd",L"NeuralResolutionScale",escala,ini.c_str())){error="3";return false;}
    return true;
}
}

std::optional<Extent> scaled_extent(Extent stage,unsigned percent) {
    if(!valid(percent) || !stage.width || !stage.height || stage.width>16384 || stage.height>16384) return std::nullopt;
    return Extent{static_cast<unsigned>(std::max<std::uint64_t>(1,std::uint64_t(stage.width)*percent/100)),
                  static_cast<unsigned>(std::max<std::uint64_t>(1,std::uint64_t(stage.height)*percent/100))};
}
SettingsStore::SettingsStore(std::filesystem::path selected_ini):selected_ini_(std::move(selected_ini)) {}
Snapshot SettingsStore::snapshot() const { std::lock_guard<std::mutex> lock(mutex_); return state_; }
void SettingsStore::accept_locked(Settings value) {
    if(value!=state_.requested) { state_.requested=value; ++state_.generation; state_.readiness=Readiness::Pending; }
    state_.diagnostic.clear();
    if(state_.readiness==Readiness::Failed) state_.readiness=Readiness::Pending;
}
Operation SettingsStore::request_locked(Settings value) {
    if(!valid(value.percent)) { state_.diagnostic="Unsupported resolution percent"; return {false,false,false,state_.diagnostic}; }
    if(value==state_.requested) return {true,false,false,{}};
    if(state_.generation==std::numeric_limits<std::uint64_t>::max()) { state_.diagnostic="Request generation exhausted"; return {false,false,false,state_.diagnostic}; }
    std::string error;
    if(!gravar_publico(selected_ini_,value,error)) {
        state_.diagnostic=error; return {false,false,false,error};
    }
    accept_locked(value); return {true,true,true,{}};
}
Operation SettingsStore::request(Settings value) { std::lock_guard<std::mutex> lock(mutex_); return request_locked(value); }
Operation SettingsStore::control(Control control,Key key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if((control!=Control::Scale && control!=Control::PreUpscale) || (key!=Key::Left && key!=Key::Right && key!=Key::Enter)) {
        state_.diagnostic="Unsupported menu control/key"; return {false,false,false,state_.diagnostic};
    }
    auto next=state_.requested;
    if(control==Control::PreUpscale) next.pre_upscale=key==Key::Enter ? !next.pre_upscale : key==Key::Right;
    else {
        auto index=static_cast<std::size_t>(std::find(percentages.begin(),percentages.end(),next.percent)-percentages.begin());
        if(key==Key::Left && index+1<percentages.size()) ++index;
        if(key==Key::Right && index>0) --index;
        next.percent=percentages[index];
    }
    return request_locked(next);
}
Operation SettingsStore::reload() {
    std::lock_guard<std::mutex> lock(mutex_);
    Settings value; std::string error;
    if(!ler_publico(selected_ini_,value,error)) {
        state_.diagnostic=error; return {false,false,false,error}; }
    bool changed=value!=state_.requested;
    if(changed && state_.generation==std::numeric_limits<std::uint64_t>::max()) { state_.diagnostic="Request generation exhausted"; return {false,false,false,state_.diagnostic}; }
    accept_locked(value); return {true,false,changed,{}};
}
bool SettingsStore::commit_if_current(std::uint64_t generation) {
    std::lock_guard<std::mutex> lock(mutex_);
    if(generation!=state_.generation) return false;
    state_.effective=state_.requested; state_.effective_generation=generation;
    state_.readiness=Readiness::Ready; return true;
}
bool SettingsStore::fail_if_current(std::uint64_t generation,std::string diagnostic) {
    std::lock_guard<std::mutex> lock(mutex_);
    if(generation!=state_.generation) return false;
    state_.readiness=Readiness::Failed; state_.diagnostic=std::move(diagnostic); return true;
}
}
