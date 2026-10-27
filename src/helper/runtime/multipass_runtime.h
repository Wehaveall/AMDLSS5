#pragma once
extern "C" bool NRMultipassInstall_v1(void* proxy_base) noexcept;

extern "C" __declspec(dllexport) void NRMultipassEval_v1(void* context,void* source,int input_pitch,int input_format,
    int height,int width,void* destination,int output_pitch,int output_format,int tone,void* metadata) noexcept;
