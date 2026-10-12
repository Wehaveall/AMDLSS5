#include "thread_proof.h"
namespace nrtransition {
namespace {
bool usable(HANDLE h) noexcept {
    return h != nullptr && h != INVALID_HANDLE_VALUE;
}
DWORD failure() noexcept {
    const DWORD e = GetLastError();
    return e == ERROR_SUCCESS ? ERROR_INVALID_DATA : e;
}
bool shape(const ThreadPairSnapshot& s) noexcept {
    if(!s.generation || !usable(s.worker) || !s.worker_id || s.worker_id==GetCurrentThreadId())return false;
    if(!s.watchdog_required)return s.watchdog==nullptr && s.watchdog_id==0;
    return usable(s.watchdog)
        && s.worker != s.watchdog && s.worker_id != 0 && s.watchdog_id != 0
        && s.worker_id != s.watchdog_id && s.worker_id != GetCurrentThreadId()
        && s.watchdog_id != GetCurrentThreadId();
}
bool same_thread(HANDLE original, HANDLE duplicate, DWORD id) noexcept {
    return GetThreadId(original) == id && GetThreadId(duplicate) == id
        && CompareObjectHandles(original, duplicate) != FALSE;
}
}
ThreadProof::ThreadProof(const ThreadPairSnapshot& s) noexcept {
    if (!shape(s)) return;

    const HANDLE process = GetCurrentProcess();
    constexpr DWORD rights = THREAD_QUERY_LIMITED_INFORMATION | SYNCHRONIZE;
    if (!DuplicateHandle(process, s.worker, process, &worker_, rights, FALSE, 0)) {
        error_ = failure();
        return;
    }
    if (s.watchdog_required && !DuplicateHandle(process, s.watchdog, process, &watchdog_, rights, FALSE, 0)) {
        error_ = failure();
        CloseHandle(worker_);
        worker_ = nullptr;
        return;
    }
    if (!same_thread(s.worker, worker_, s.worker_id)
        || (s.watchdog_required && (!same_thread(s.watchdog, watchdog_, s.watchdog_id)
        || CompareObjectHandles(worker_, watchdog_) != FALSE))) {
        CloseHandle(worker_);
        if(watchdog_)CloseHandle(watchdog_);
        worker_ = watchdog_ = nullptr;
        return;
    }
    worker_id_ = s.worker_id;
    watchdog_id_ = s.watchdog_id;
    generation_ = s.generation;
    watchdog_required_ = s.watchdog_required;
    error_ = ERROR_SUCCESS;
}
ThreadProof::~ThreadProof() noexcept {
    if (worker_) CloseHandle(worker_);
    if (watchdog_) CloseHandle(watchdog_);
}
bool ThreadProof::valid() const noexcept { return error_ == ERROR_SUCCESS; }
DWORD ThreadProof::last_error() const noexcept { return error_; }
bool ThreadProof::matches(const ThreadPairSnapshot& s) const noexcept {
    return valid() && shape(s) && s.generation == generation_ && s.watchdog_required == watchdog_required_
        && s.worker_id == worker_id_ && s.watchdog_id == watchdog_id_
        && same_thread(s.worker, worker_, worker_id_)
        && (!watchdog_required_ || same_thread(s.watchdog, watchdog_, watchdog_id_));
}
ThreadPoll ThreadProof::poll() noexcept {
    if (!valid()) return ThreadPoll::Failed;

    const DWORD worker = WaitForSingleObject(worker_, 0);
    const DWORD worker_error = worker == WAIT_FAILED ? failure() : ERROR_SUCCESS;
    const DWORD watchdog = watchdog_required_ ? WaitForSingleObject(watchdog_, 0) : WAIT_OBJECT_0;
    const DWORD watchdog_error = watchdog == WAIT_FAILED ? failure() : ERROR_SUCCESS;
    if ((worker != WAIT_OBJECT_0 && worker != WAIT_TIMEOUT)
        || (watchdog != WAIT_OBJECT_0 && watchdog != WAIT_TIMEOUT)) {
        error_ = worker_error ? worker_error : watchdog_error;
        if (error_ == ERROR_SUCCESS) error_ = ERROR_INVALID_DATA;
        return ThreadPoll::Failed;
    }
    return worker == WAIT_OBJECT_0 && watchdog == WAIT_OBJECT_0
        ? ThreadPoll::Terminated : ThreadPoll::Pending;
}
}
