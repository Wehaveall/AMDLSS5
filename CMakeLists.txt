# AMDLSS5: helper nr_native_adapter.dll.
#   cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DNR_SCALE_DXC=<dxc.exe> -DNR_HIP_INCLUDE_DIR=<HIP include folder>
#         -DNR_FFX_SDK_DIR=<FidelityFX SDK 2.3.0 Kits/FidelityFX folder>
#   cmake --build build --config Release --target nr_live_adapter
# Or: powershell -NoProfile -File Build-Helper.ps1 (README.md).
cmake_minimum_required(VERSION 3.24)
project(AMDLSS5 LANGUAGES CXX)
# Release flavor: no log files and no recordings/dumps to disk.
set(NR_SEM_LOG ON CACHE BOOL "" FORCE)
set(NR_SEM_GRAVACAO ON CACHE BOOL "" FORCE)
set(NR_GAME_PRE_ONLY_MENU ON CACHE BOOL "" FORCE)
if(NOT EXISTS "${NR_FFX_SDK_DIR}/api/include/ffx_api.h")
  message(FATAL_ERROR "set NR_FFX_SDK_DIR to the Kits/FidelityFX folder of the AMD FidelityFX SDK 2.3.0 (api/include/ffx_api.h)")
endif()
add_subdirectory(src/helper/source/implementation/live-transition-helper helper)
