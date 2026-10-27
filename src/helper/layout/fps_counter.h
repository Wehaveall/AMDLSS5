#pragma once
namespace nrmenufps {

struct Display { double display_fps{},input_fps{}; bool display_valid{},input_valid{},read_ok{}; int fg{}; };

Display observe(void* base,void* swapchain,unsigned width,unsigned height) noexcept;
}
