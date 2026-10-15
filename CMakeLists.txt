# AMDLSS5: helper nr_native_adapter.dll.
#   cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DNR_SCALE_DXC=<dxc.exe> -DNR_HIP_INCLUDE_DIR=<HIP include folder>
#   cmake --build build --config Release --target nr_live_adapter
# Or: powershell -NoProfile -File Build-Helper.ps1 (README.md).
cmake_minimum_required(VERSION 3.24)
project(AMDLSS5 LANGUAGES CXX)
set(NR_PUBLICO OFF CACHE BOOL "" FORCE)
set(NR_PRESENT_RUNTIME ON CACHE BOOL "" FORCE)
set(NR_GAME_PRE_ONLY_MENU ON CACHE BOOL "" FORCE)
add_subdirectory(src/helper/source/implementation/live-transition-helper helper)
