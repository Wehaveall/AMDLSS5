#pragma once
#include "../../../runtime/nr_publico.h"
#include <atomic>
#include <filesystem>
#include <mutex>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace nrpresentmode {

inline std::atomic<unsigned> startup{0}, requested{0}, status{0};
inline std::mutex initialization_mutex;
inline bool active() noexcept { return startup.load(std::memory_order_acquire)==2; }
inline std::filesystem::path configuration_path(const std::filesystem::path& path) {

    if(path.parent_path().filename()==L"_storage_"){
        const auto original=path.parent_path().parent_path()/path.filename();
        if(std::filesystem::is_regular_file(original))return original;
    }
    return path;
}
inline bool initialize(const std::filesystem::path& path) noexcept {
    try {
        std::lock_guard<std::mutex> lock(initialization_mutex);
        if(startup.load(std::memory_order_acquire))return status.load()!=4;
        if(!path.is_absolute() || !path.has_filename()) {
            status.store(4);startup.store(1,std::memory_order_release);return false;
        }
        const DWORD attributes=GetFileAttributesW(path.c_str());
        if(attributes==INVALID_FILE_ATTRIBUTES && GetLastError()!=ERROR_FILE_NOT_FOUND && GetLastError()!=ERROR_PATH_NOT_FOUND) {
            status.store(4);startup.store(1,std::memory_order_release);return false;
        }
        if(attributes!=INVALID_FILE_ATTRIBUTES && (attributes&FILE_ATTRIBUTE_DIRECTORY)) {
            status.store(4);startup.store(1,std::memory_order_release);return false;
        }
        wchar_t text[16]{};
        const auto selected_path=configuration_path(path);
        const DWORD count=NRCFG_STR(L"DlssNrOnAmd",L"PresentMode",L"0",text,16,selected_path.c_str());
        if(count!=1 || (text[0]!=L'0' && text[0]!=L'1')) {
            status.store(4);startup.store(1,std::memory_order_release);return false;
        }
        const unsigned selected=text[0]==L'1' ? 1u : 0u;
        requested.store(selected,std::memory_order_relaxed);
        status.store(selected ? 0u : 2u,std::memory_order_relaxed);
        startup.store(selected ? 2u : 1u,std::memory_order_release);
        return true;
    } catch(...) {status.store(4);startup.store(1,std::memory_order_release);return false;}
}
}
