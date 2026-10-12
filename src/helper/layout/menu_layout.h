#pragma once
#include "fps_counter.h"
namespace nrmenulayout {
struct Context {void* base{};unsigned width{},height{};nrmenufps::Display counters;};

class Scope {
    void* previous_{};
    Context context_{};
public:
    Scope(void* base,void* swapchain) noexcept;
    ~Scope();
    Scope(const Scope&)=delete;
    Scope& operator=(const Scope&)=delete;
};
}
