#pragma once
#include <atomic>
namespace nrmode {

inline std::atomic<int> async_completed_job{0};
inline std::atomic<int> async_completion_status{0};
}
