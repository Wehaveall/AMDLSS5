# AMDLSS5: libxess.dll for Shadow of the Tomb Raider (the game's XeSS 1.x API -> AMD FidelityFX API: FSR upscaling of the driver
# + frame generation). x64 + MSVC, static runtime.
#   cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DFFX_SDK_DIR=<FidelityFX SDK 2.3.0 Kits/FidelityFX>
#   cmake --build build --config Release
# Or: powershell -NoProfile -File Build.ps1 (downloads the XeSS 1.3.1 headers with Baixar-Headers.ps1).
cmake_minimum_required(VERSION 3.20)
project(xess_fsr LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(FFX_SDK_DIR "" CACHE PATH "Kits/FidelityFX folder of the AMD FidelityFX SDK 2.3.0")
set(XESS_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/xess-1.3.1" CACHE PATH
    "Folder with xess/xess.h (downloaded by Baixar-Headers.ps1; Intel license)")

foreach(arq "${FFX_SDK_DIR}/api/include/ffx_api.h" "${FFX_SDK_DIR}/upscalers/include/ffx_upscale.h"
            "${FFX_SDK_DIR}/framegeneration/include/ffx_framegeneration.h"
            "${XESS_HEADERS_DIR}/xess/xess_d3d12.h")
  if(NOT EXISTS "${arq}")
    message(FATAL_ERROR "missing ${arq} (FFX_SDK_DIR / XESS_HEADERS_DIR)")
  endif()
endforeach()

add_compile_options(/W4 /permissive- /utf-8 /wd4100)
add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
# Release flavor: no log file and no stencil dumps.
add_compile_definitions(NR_SEM_LOG=1)

add_library(xess_traducao STATIC src/traducao.cpp)

add_library(libxess SHARED src/libxess_fsr.cpp src/geracao_quadros.cpp src/registro.cpp src/libxess.def)
set_target_properties(libxess PROPERTIES OUTPUT_NAME libxess PREFIX "")
target_include_directories(libxess PRIVATE "${FFX_SDK_DIR}/api/include" "${FFX_SDK_DIR}/upscalers/include"
                                           "${FFX_SDK_DIR}/framegeneration/include" "${XESS_HEADERS_DIR}")
target_link_libraries(libxess PRIVATE xess_traducao)
