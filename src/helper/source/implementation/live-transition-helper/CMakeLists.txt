cmake_minimum_required(VERSION 3.24)
project(nr_live_helper LANGUAGES CXX)
add_compile_options(/utf-8)

  add_compile_definitions(NR_SEM_LOG=1)
  add_compile_options(/wd4127 /wd4100 /wd4189 /wd4505)

  add_compile_definitions(NR_SEM_GRAVACAO=1)

add_subdirectory(../universal-scale universal-scale)
add_subdirectory(../adapter-menu adapter)

  target_compile_definitions(nr_adapter_core PRIVATE NR_TONE_RUNTIME=1)

add_subdirectory(../live-transition-binding binding)
set(NR_MULTIPASS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../runtime")
set(NR_PRESENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../present-runtime")
set(NR_PRESENT_TRANSPORT_SOURCE "${NR_PRESENT_DIR}/present_transport.cpp" CACHE FILEPATH "Present transport implementation source")
if(NOT IS_ABSOLUTE "${NR_PRESENT_TRANSPORT_SOURCE}" OR NOT EXISTS "${NR_PRESENT_TRANSPORT_SOURCE}" OR IS_DIRECTORY "${NR_PRESENT_TRANSPORT_SOURCE}" OR NOT NR_PRESENT_TRANSPORT_SOURCE MATCHES "\\.cpp$")
  message(FATAL_ERROR "NR_PRESENT_TRANSPORT_SOURCE must be an existing absolute .cpp file")
endif()
add_library(nr_present_core STATIC "${NR_PRESENT_DIR}/native_present.cpp" "${NR_PRESENT_TRANSPORT_SOURCE}")
target_compile_features(nr_present_core PUBLIC cxx_std_17)
target_compile_options(nr_present_core PRIVATE /W4 /WX /permissive- /EHsc)
target_link_libraries(nr_present_core PUBLIC d3d12 dxgi bcrypt)
add_library(nr_live_adapter SHARED export.cpp present_overlay.cpp present_integration.cpp menu_input.cpp "${NR_MULTIPASS_DIR}/multipass_runtime.cpp")

find_path(NR_HIP_INCLUDE_DIR hip/hip_runtime_api.h
  HINTS "$ENV{HIP_PATH}/include" REQUIRED)
target_include_directories(nr_live_adapter SYSTEM PRIVATE "${NR_HIP_INCLUDE_DIR}")
target_compile_definitions(nr_live_adapter PRIVATE __HIP_PLATFORM_AMD__)

set(NR_FRAME_LIMITER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../frame-limiter")
target_sources(nr_live_adapter PRIVATE "${NR_FRAME_LIMITER_DIR}/frame_limiter.cpp")

set(NR_MENU_LAYOUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../layout")
target_sources(nr_live_adapter PRIVATE "${NR_MENU_LAYOUT_DIR}/menu_layout.cpp" "${NR_MENU_LAYOUT_DIR}/fps_counter.cpp")
target_include_directories(nr_live_adapter PRIVATE "${NR_MENU_LAYOUT_DIR}")

target_include_directories(nr_live_adapter PRIVATE "${NR_MULTIPASS_DIR}")

set_target_properties(nr_live_adapter PROPERTIES OUTPUT_NAME nr_native_adapter)
target_link_options(nr_live_adapter PRIVATE "/MAP:${CMAKE_CURRENT_BINARY_DIR}/nr_measure.map")
target_link_libraries(nr_live_adapter PRIVATE nr_adapter_core nr_live_binding nr_scale_policy nr_scale_d3d12 nr_present_core dxguid)
target_compile_options(nr_live_adapter PRIVATE /W4 /WX /permissive- /EHsc)

add_custom_command(TARGET nr_live_adapter POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NR_SCALE_SHADERS} $<TARGET_FILE_DIR:nr_live_adapter>)

target_sources(nr_live_adapter PRIVATE "${NR_PRESENT_DIR}/print_capture.cpp")

target_sources(nr_live_adapter PRIVATE "${NR_PRESENT_DIR}/rec_aviso.cpp")

target_sources(nr_live_adapter PRIVATE "${NR_MULTIPASS_DIR}/overlay_amd.cpp")

if(NOT NR_PUBLICO)

endif()
target_link_libraries(nr_live_adapter PRIVATE windowscodecs ole32)
