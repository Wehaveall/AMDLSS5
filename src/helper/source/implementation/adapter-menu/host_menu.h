#pragma once
#include "menu_bridge.h"
#include <filesystem>
#include "../../../runtime/tone_control.h"
#include "../compose-controls/control_state.h"
#include "../async-reprojection/reproject_controls.h"

namespace nradapter {
struct NativeVector {
    nrmenu::NativeRow* begin;
    nrmenu::NativeRow* end;
    nrmenu::NativeRow* capacity;
};
using Grow = void* (*)(NativeVector*, nrmenu::NativeRow*, nrmenu::NativeRow*);
struct NativeHost {
    void* base = nullptr;
    NativeVector* rows = nullptr;
    bool ready = false;
    std::filesystem::path selected_ini;
    Grow grow = nullptr;
    bool hide_scale = false;
    unsigned* auto_mask = nullptr;
    unsigned* style_requested = nullptr;
    unsigned* style_effective = nullptr;
    unsigned* multipass_requested = nullptr;
    unsigned* multipass_effective = nullptr;
    nrtone::State* tone_state = nullptr;
    nrcompose::State* compose_controls = nullptr;

    nrreproject::Controls* reproject_controls = nullptr;

    bool full_menu = false;
};
enum class Access { Read, Write, Execute };
bool accessible(const void* address, std::size_t length, Access access) noexcept;
bool valid_vector(const NativeVector* rows, std::size_t& count) noexcept;
bool valid_path(const std::filesystem::path& path) noexcept;
bool resolve_host(void* base, NativeHost& host) noexcept;
}
