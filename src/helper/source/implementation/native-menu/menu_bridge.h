#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>

#if !defined(_WIN64) || !defined(_M_X64)
#error This ABI bridge requires Windows x64.
#endif

namespace nrmenu {
struct MenuCallbacks {
    void* context;
    void (*read)(void*, unsigned control, char* output, std::size_t capacity);
    void (*act)(void*, unsigned control, int delta);
};
enum Control : unsigned { BeforeUpscale = 0, NeuralResolution = 1 };
struct NativeString {
    char text[16];
    std::uint64_t size;
    std::uint64_t capacity;
};
struct alignas(8) NativeRow {
    const char* label;
    std::byte getter_storage[56];
    void* getter;
    std::byte action_storage[56];
    void* action;
    const char* help;
};

void initialise_row(NativeRow& row, MenuCallbacks callbacks, unsigned control, const char* label, const char* help) noexcept;
void initialise_rows(NativeRow (&rows)[2], MenuCallbacks callbacks) noexcept;

void replace_mode_control(NativeRow& row, MenuCallbacks callbacks) noexcept;

void move_row(NativeRow& destination, NativeRow& source) noexcept;
void destroy_row(NativeRow& row) noexcept;

static_assert(sizeof(void*) == 8 && sizeof(int) == 4);
static_assert(std::is_standard_layout_v<NativeRow>);
static_assert(sizeof(NativeRow) == 0x90 && alignof(NativeRow) == 8);
static_assert(offsetof(NativeRow, label) == 0);
static_assert(offsetof(NativeRow, getter_storage) == 0x08);
static_assert(offsetof(NativeRow, getter) == 0x40);
static_assert(offsetof(NativeRow, action_storage) == 0x48);
static_assert(offsetof(NativeRow, action) == 0x80);
static_assert(offsetof(NativeRow, help) == 0x88);
static_assert(sizeof(NativeString) == 32);
static_assert(offsetof(NativeString, size) == 16);
static_assert(offsetof(NativeString, capacity) == 24);
}
