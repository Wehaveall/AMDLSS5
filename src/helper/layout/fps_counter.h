#pragma once
namespace nrmenufps {

struct Display { char fps[128]{}; char generation[128]{}; double display_fps{},input_fps{}; bool display_valid{},input_valid{},read_ok{}; int fg{}; };

Display observe(void* base,void* swapchain,unsigned width,unsigned height) noexcept;
}
