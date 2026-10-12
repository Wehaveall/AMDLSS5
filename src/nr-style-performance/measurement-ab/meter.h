#pragma once
#include <array>
#include <cmath>
namespace nrmeasure {
enum class Mode {Off,Total,Group};
enum class Status {Invalid,Pending,Ready};
struct Api {int(*record)(void*,void*);int(*query)(void*);int(*elapsed)(float*,void*,void*);};
struct Result {Status status=Status::Invalid;float total_ms=0,group_ms=0;int error=0;const char* phase="inactive";};

class Meter {
 Api api_;Mode mode_=Mode::Off;unsigned first_=0,last_=0,count_=0,marks_=0,seen_=0;
 bool valid_=false,have_stream_=false;void* stream_=nullptr;
 void mark(unsigned index,void* stream){if(!valid_)return;if(api_.record(events[index],stream)){valid_=false;return;}marks_|=1u<<index;}
public:
 std::array<void*,4> events{};
 explicit Meter(Api api):api_(api){}
 void begin(Mode mode,unsigned first,unsigned last,unsigned count){mode_=mode;first_=first;last_=last;count_=count;marks_=0;seen_=0;valid_=count>1&&first<=last&&last<count;have_stream_=false;stream_=nullptr;}
 void before(unsigned index,void* stream){
  if(!valid_||mode_==Mode::Off)return;
  if(!have_stream_){stream_=stream;have_stream_=true;}else if(stream!=stream_)valid_=false;
  if(index!=seen_){valid_=false;return;}
  if(index==1)mark(0,stream);
  if(mode_==Mode::Group&&index==first_)mark(2,stream);
 }
 void after(unsigned index,void* stream){
  if(mode_==Mode::Off)return;
  if(index!=seen_){valid_=false;return;}++seen_;
  if(mode_==Mode::Group&&index==last_)mark(3,stream);
  if(index+1==count_)mark(1,stream);
 }
 bool finish(unsigned count){valid_=valid_&&count==count_&&(mode_==Mode::Off||(seen_==count&&marks_==(mode_==Mode::Total?3u:15u)));return valid_;}
 Result read(){
  if(!valid_||mode_==Mode::Off)return {};
  Result r;const int q=api_.query(events[1]);r.error=q;r.phase="query";if(q==600){r.status=Status::Pending;return r;}if(q)return r;
  r.phase="elapsed_total";r.error=api_.elapsed(&r.total_ms,events[0],events[1]);if(r.error)return r;
  if(mode_==Mode::Group){r.phase="elapsed_group";r.error=api_.elapsed(&r.group_ms,events[2],events[3]);if(r.error)return r;}
  r.phase="elapsed_value";
  if(!std::isfinite(r.total_ms)||r.total_ms<=0||!std::isfinite(r.group_ms)||(mode_==Mode::Group&&(r.group_ms<=0||r.group_ms>r.total_ms+0.001f)))return r;
  r.status=Status::Ready;r.phase="ready";return r;
 }
};
}
