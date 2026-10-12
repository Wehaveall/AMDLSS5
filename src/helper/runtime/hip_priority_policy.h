#pragma once

namespace nrhipprio::policy {

inline constexpr unsigned handshake_count=3;
inline constexpr unsigned handshake_offsets[handshake_count]{0x62260u,0x62268u,0x62270u};
inline constexpr const char* handshake_names[handshake_count]{"_Z11k_flag_waitPjjj","_Z13k_align_probePh","_Z10k_flag_setPjjS_"};

inline constexpr unsigned slot_launch=0x86bd8u,slot_copy_async=0x86bf0u,slot_set_async=0x86c08u,slot_event_record=0x86b90u;

inline constexpr int default_enabled=0;

inline constexpr int absent=-12345;
inline bool enabled(int neural_hip_priority,int legacy_async_hip_priority) noexcept {
 if(neural_hip_priority!=absent)return neural_hip_priority==1;
 if(legacy_async_hip_priority!=absent)return legacy_async_hip_priority==1;
 return default_enabled==1;
}
inline int handshake_index(const void* f,const char* image) noexcept {
 if(!f||!image)return -1;
 for(unsigned i=0;i<handshake_count;++i)if(f==image+handshake_offsets[i])return static_cast<int>(i);
 return -1;
}
enum class Route {keep,priority};

enum class Call {kernel,copy_memset_event};
inline Route route(Call call,bool in_eval,bool stream_ready,const void* requested,bool handshake) noexcept {
 if(!stream_ready||requested||handshake)return Route::keep;
 if(call==Call::kernel||in_eval)return Route::priority;
 return Route::keep;
}

inline bool wants_stream(Call call,bool in_eval,const void* requested,bool handshake) noexcept {
 return route(call,in_eval,true,requested,handshake)==Route::priority;
}
}
