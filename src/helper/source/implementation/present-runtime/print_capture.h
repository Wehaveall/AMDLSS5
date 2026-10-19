#pragma once
#include "print_state.h"
#include <cstdint>
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace nrprint {
enum class Kind : unsigned { Unsupported=0, Rgba8, Bgra8, Rgb10a2, Rgba16f };
Kind kind_of(unsigned dxgi_format) noexcept;
unsigned bytes_per_pixel(Kind kind) noexcept;

void convert_row(Kind kind,const void* source,unsigned width,unsigned char* rgba) noexcept;
unsigned char linear_half_to_srgb8(std::uint16_t half) noexcept;
std::wstring file_name(const SYSTEMTIME& time);
bool encode_png(const std::wstring& path,const unsigned char* rgba,unsigned width,unsigned height,std::string& error) noexcept;

void capture(void* swapchain,void* queue) noexcept;

void capture_to(void* swapchain,void* queue,const std::wstring& folder) noexcept;

void medir_tela(void* swapchain,void* queue) noexcept;

bool wait_idle(unsigned milliseconds) noexcept;
}
