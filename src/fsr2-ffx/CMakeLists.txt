# AMDLSS5: ffx_fsr2_api_x64.dll for Red Dead Redemption 2 (the game's FSR 2.2 API -> AMD FidelityFX API: FSR upscaling of the
# driver + frame generation). x64 + MSVC, static runtime. Uses the FSR 2 -> FidelityFX API translation of the helper (../helper).
#   cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DFFX_SDK_DIR=<FidelityFX SDK 2.3.0 Kits/FidelityFX> -DFSR2_SDK_DIR=<FSR 2.2 ffx-fsr2-api>
#   cmake --build build --config Release
# Or: powershell -NoProfile -File Build.ps1
cmake_minimum_required(VERSION 3.20)
project(fsr2_ffx LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(FFX_SDK_DIR "" CACHE PATH "Kits/FidelityFX folder of the AMD FidelityFX SDK 2.3.0")
set(FSR2_SDK_DIR "" CACHE PATH "src/ffx-fsr2-api folder of AMD FidelityFX FSR 2.2 (headers)")
set(HELPER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../helper/source/implementation/active-scheduling")
set(HELPER_RUNTIME "${CMAKE_CURRENT_SOURCE_DIR}/../helper/runtime")

foreach(arq "${FFX_SDK_DIR}/api/include/ffx_api.h" "${FFX_SDK_DIR}/upscalers/include/ffx_upscale.h"
            "${FFX_SDK_DIR}/framegeneration/include/ffx_framegeneration.h" "${FSR2_SDK_DIR}/ffx_fsr2.h")
  if(NOT EXISTS "${arq}")
    message(FATAL_ERROR "missing ${arq} (FFX_SDK_DIR / FSR2_SDK_DIR)")
  endif()
endforeach()

add_compile_options(/W4 /permissive- /utf-8 /wd4100 /EHsc)
add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
# Release flavor: no log file.
add_compile_definitions(NR_SEM_LOG=1)

add_library(fsr2_traducao STATIC "${HELPER_SRC}/ffx_descriptor.cpp" "${HELPER_SRC}/fsr2_descriptor.cpp"
                                 "${HELPER_SRC}/fsr2_ffx_traducao.cpp" "${HELPER_SRC}/fsr2_fg_traducao.cpp")
target_include_directories(fsr2_traducao PUBLIC "${HELPER_SRC}" "${FFX_SDK_DIR}/api/include" "${FFX_SDK_DIR}/upscalers/include"
                                                "${FFX_SDK_DIR}/framegeneration/include")

add_library(fsr2_api SHARED src/fsr2_ffx_dll.cpp src/geracao_quadros.cpp src/registro.cpp src/fsr2_ffx.def)
set_target_properties(fsr2_api PROPERTIES OUTPUT_NAME ffx_fsr2_api_x64 PREFIX "")
target_include_directories(fsr2_api PRIVATE "${FSR2_SDK_DIR}" "${HELPER_RUNTIME}")
target_compile_definitions(fsr2_api PRIVATE FFX_GCC)
target_link_libraries(fsr2_api PRIVATE fsr2_traducao)
