#pragma once

#include <cstdint>
#include <string>
#include <vector>
namespace nraviso {

struct Ret {int x,y,w,h;};
void texto(const char* t,int x0,int y0,int s,std::vector<Ret>& out);
int largura(const char* t,int s) noexcept;

int estado(std::uint64_t agora_ms,char* linha,unsigned cap) noexcept;

void desenhar(void* swapchain,void* queue) noexcept;
}
