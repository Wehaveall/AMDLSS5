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
std::string trim(std::string value) {
    auto first=value.find_first_not_of(" \t\r");
    if (first==std::string::npos) return {};
    return value.substr(first,value.find_last_not_of(" \t\r")-first+1);
}
std::string lower(std::string value) {
    for(char& c:value) if(c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');
    return value;
}
struct Line { std::string body, ending; };
struct Document { std::string bom; std::vector<Line> lines; };
bool load(const std::filesystem::path& path, Document& doc, std::string& error, bool allow_missing) {
    std::error_code ec;
    bool exists=std::filesystem::exists(path,ec);
    if(ec) { error="Cannot inspect selected INI: " + ec.message(); return false; }
    if(!exists) {
        if(allow_missing) return true;
        error="Selected INI does not exist"; return false;
    }
    if(!std::filesystem::is_regular_file(path,ec) || ec) { error="Selected INI is not a regular file"; return false; }
    std::ifstream file(path,std::ios::binary);
    if(!file) { error="Cannot read selected INI"; return false; }
    std::string bytes{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    if(file.bad()) { error="Cannot finish reading selected INI"; return false; }
    if(bytes.find('\0') != std::string::npos || bytes.compare(0,2,"\xFF\xFE")==0 || bytes.compare(0,2,"\xFE\xFF")==0) {
        error="Unsupported INI encoding (UTF-16/UTF-32 or embedded NUL); file unchanged"; return false;
    }
    std::size_t pos=0;
    if(bytes.compare(0,3,"\xEF\xBB\xBF")==0) { doc.bom=bytes.substr(0,3); pos=3; }
    while(pos<bytes.size()) {
        auto end=bytes.find('\n',pos);
        if(end==std::string::npos) { doc.lines.push_back({bytes.substr(pos),""}); break; }
        bool cr=end>pos && bytes[end-1]=='\r';
        doc.lines.push_back({bytes.substr(pos,end-pos-(cr?1:0)),cr?"\r\n":"\n"});
        pos=end+1;
    }
    return true;
}
bool header(const Line& line,std::string& section) {
    auto body=trim(line.body);
    if(!body.empty() && body.front()=='[') {
        section=body.size()>=2 && body.back()==']' ? lower(trim(body.substr(1,body.size()-2))) : "";
        return true;
    }
    return false;
}
std::pair<std::string,std::string> key_value(const Line& line) {
    auto body=trim(line.body);
    if(body.empty() || body.front()==';' || body.front()=='#') return {};
    auto equal=body.find('=');
    if(equal==std::string::npos) return {};
    return {lower(trim(body.substr(0,equal))),trim(body.substr(equal+1))};
}
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
bool parse(const Document& doc,Settings& requested,std::string& error) {
    Settings candidate{};
    std::string section; bool seen_pre=false,seen_scale=false;
    for(const auto& line:doc.lines) {
        if(header(line,section) || section!=section_name) continue;
        auto kv=key_value(line);
        if(kv.first=="preupscalenr") {
            if(seen_pre || (kv.second!="0" && kv.second!="1")) { error="Invalid or duplicate PreUpscaleNR (expected 0 or 1)"; return false; }
            seen_pre=true; candidate.pre_upscale=kv.second=="1";
        } else if(kv.first=="neuralresolutionscale") {
            if(seen_scale || !parse_percent(kv.second,candidate.percent)) { error="Invalid or duplicate NeuralResolutionScale (expected supported finite scale)"; return false; }
            seen_scale=true;
        }
    }
    requested=candidate; return true;
}
std::string serialize(Document doc,Settings value) {
    std::string newline="\r\n";
    for(const auto& line:doc.lines) if(!line.ending.empty()) { newline=line.ending; break; }
    const auto index=static_cast<std::size_t>(std::find(percentages.begin(),percentages.end(),value.percent)-percentages.begin());
    const std::array<std::string,2> replacements{std::string("PreUpscaleNR=")+(value.pre_upscale?"1":"0"),std::string("NeuralResolutionScale=")+texts[index]};
    std::string section; bool found_section=false,pre=false,scale=false;
    std::size_t insert=doc.lines.size();
    for(std::size_t i=0;i<doc.lines.size();++i) {
        if(header(doc.lines[i],section)) { if(section==section_name) { found_section=true; insert=i+1; } continue; }
        if(section!=section_name) continue;
        insert=i+1;
        auto key=key_value(doc.lines[i]).first;
        if(key=="preupscalenr") { doc.lines[i].body=replacements[0]; pre=true; }
        if(key=="neuralresolutionscale") { doc.lines[i].body=replacements[1]; scale=true; }
    }

    bool emitted_pre=false,emitted_scale=false;
    std::vector<Line> output;
    auto append=[&](const std::string& body) {
        if(!output.empty() && output.back().ending.empty()) output.back().ending=newline;
        output.push_back({body,newline});
    };
    section.clear();
    for(std::size_t i=0;i<=doc.lines.size();++i) {
        if(i==insert) {
            if(!found_section) append("[DlssNrOnAmd]");
            if(!pre) append(replacements[0]);
            if(!scale) append(replacements[1]);
        }
        if(i==doc.lines.size()) break;
        auto line=doc.lines[i];
        if(!header(line,section) && section==section_name) {
            auto key=key_value(line).first;
            if(key=="preupscalenr") { if(emitted_pre) continue; emitted_pre=true; }
            if(key=="neuralresolutionscale") { if(emitted_scale) continue; emitted_scale=true; }
        }
        output.push_back(std::move(line));
    }
    std::string bytes=doc.bom;
    for(const auto& line:output) { bytes+=line.body; bytes+=line.ending; }
    return bytes;
}
bool persist(const std::filesystem::path& selected,const std::string& bytes,std::string& error) {
    static std::atomic<unsigned long long> counter{0};
    std::filesystem::path temp;
    HANDLE file=INVALID_HANDLE_VALUE;
    for(unsigned attempt=0;attempt<100;++attempt) {
        temp=selected;
        temp+=L".nr-tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(++counter);
        file=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file!=INVALID_HANDLE_VALUE) break;
        if(GetLastError()!=ERROR_FILE_EXISTS && GetLastError()!=ERROR_ALREADY_EXISTS) break;
    }
    if(file==INVALID_HANDLE_VALUE) { error="INI not persisted: cannot create adjacent temporary file (Windows " + std::to_string(GetLastError())+")"; return false; }
    bool okay=true; std::size_t offset=0; DWORD failure=ERROR_SUCCESS;
    while(offset<bytes.size()) {
        DWORD count=static_cast<DWORD>(std::min<std::size_t>(bytes.size()-offset,1024*1024));
        DWORD written=0;
        if(!WriteFile(file,bytes.data()+offset,count,&written,nullptr) || written!=count) { failure=GetLastError(); okay=false; break; }
        offset+=written;
    }
    if(okay && !FlushFileBuffers(file)) { failure=GetLastError(); okay=false; }
    if(!CloseHandle(file) && okay) { failure=GetLastError(); okay=false; }
    if(okay && !MoveFileExW(temp.c_str(),selected.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) { failure=GetLastError(); okay=false; }
    if(!okay) {
        DeleteFileW(temp.c_str());
        error="INI not persisted: atomic replacement failed (Windows " + std::to_string(failure)+")";
    }
    return okay;
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
    Document doc; std::string error;
    if(!load(selected_ini_,doc,error,true) || !persist(selected_ini_,serialize(std::move(doc),value),error)) {
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
    Document doc; Settings value; std::string error;
    if(!load(selected_ini_,doc,error,false) || !parse(doc,value,error)) {
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
