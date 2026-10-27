cmake_minimum_required(VERSION 3.24)
project(nr_live_helper LANGUAGES CXX)
add_compile_options(/utf-8)

  add_compile_definitions(NR_SEM_LOG=1)
  add_compile_options(/wd4127 /wd4100 /wd4189 /wd4505)

  add_compile_definitions(NR_SEM_GRAVACAO=1)

message(STATUS "NR sabor: NR_SEM_LOG=${NR_SEM_LOG} NR_SEM_GRAVACAO=${NR_SEM_GRAVACAO}")
add_subdirectory(../universal-scale universal-scale)
add_subdirectory(../adapter-menu adapter)

  target_compile_definitions(nr_adapter_core PRIVATE NR_TONE_RUNTIME=1)

add_subdirectory(../live-transition-binding binding)
set(NR_MULTIPASS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../runtime")
set(NR_PRESENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../present-runtime")
add_library(nr_live_adapter SHARED export.cpp present_overlay.cpp menu_input.cpp fsr2_gancho.cpp "${NR_MULTIPASS_DIR}/multipass_runtime.cpp")

find_path(NR_HIP_INCLUDE_DIR hip/hip_runtime_api.h
  HINTS "$ENV{HIP_PATH}/include" REQUIRED)
target_include_directories(nr_live_adapter SYSTEM PRIVATE "${NR_HIP_INCLUDE_DIR}")
target_compile_definitions(nr_live_adapter PRIVATE __HIP_PLATFORM_AMD__)

foreach(t)
  target_compile_features(${t} PRIVATE cxx_std_17)
  target_compile_options(${t} PRIVATE /UNDEBUG /W4 /WX /EHsc)
endforeach()

set(NR_FRAME_LIMITER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../frame-limiter")
target_sources(nr_live_adapter PRIVATE "${NR_FRAME_LIMITER_DIR}/frame_limiter.cpp")

set(NR_MENU_LAYOUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../layout")
target_sources(nr_live_adapter PRIVATE "${NR_MENU_LAYOUT_DIR}/menu_layout.cpp" "${NR_MENU_LAYOUT_DIR}/fps_counter.cpp")
target_include_directories(nr_live_adapter PRIVATE "${NR_MENU_LAYOUT_DIR}")

target_include_directories(nr_live_adapter PRIVATE "${NR_MULTIPASS_DIR}")

set_target_properties(nr_live_adapter PROPERTIES OUTPUT_NAME nr_native_adapter)
target_link_options(nr_live_adapter PRIVATE "/MAP:${CMAKE_CURRENT_BINARY_DIR}/nr_measure.map")
target_link_libraries(nr_live_adapter PRIVATE nr_adapter_core nr_live_binding nr_scale_policy nr_scale_d3d12 d3d12 dxgi bcrypt dxguid)
target_compile_options(nr_live_adapter PRIVATE /W4 /WX /permissive- /EHsc)

add_custom_command(TARGET nr_live_adapter POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NR_SCALE_SHADERS} $<TARGET_FILE_DIR:nr_live_adapter>)

target_sources(nr_live_adapter PRIVATE "${NR_PRESENT_DIR}/print_capture.cpp")

target_sources(nr_live_adapter PRIVATE "${NR_PRESENT_DIR}/rec_aviso.cpp")

target_sources(nr_live_adapter PRIVATE "${NR_MULTIPASS_DIR}/overlay_amd.cpp")

if(NR_FILTROS_CS)
  foreach(alvo nr_live_adapter)
    add_dependencies(${alvo} nr_filtros_cs)
    target_include_directories(${alvo} PRIVATE "${NR_FILTROS_CS_DIR}")
    target_compile_definitions(${alvo} PRIVATE NR_FILTROS_CS=1 NR_FG_NITIDEZ_CS=1)
  endforeach()
endif()
target_link_libraries(nr_live_adapter PRIVATE windowscodecs ole32)

target_sources(nr_live_adapter PRIVATE fsr2_ffx.cpp)
target_link_libraries(nr_live_adapter PRIVATE nr_fsr2_ffx_traducao)
