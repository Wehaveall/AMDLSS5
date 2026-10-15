cmake_minimum_required(VERSION 3.20)
project(nr_native_adapter LANGUAGES CXX)
if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "MSVC x64 required")
endif()

add_subdirectory(../native-menu "${CMAKE_CURRENT_BINARY_DIR}/native-menu")
add_subdirectory(../settings "${CMAKE_CURRENT_BINARY_DIR}/settings")
add_library(nr_adapter_core STATIC adapter_menu.cpp host_menu.cpp)
option(NR_PRESENT_RUNTIME "Expose experimental Present restart selector" OFF)

  target_compile_definitions(nr_adapter_core PUBLIC NR_PRESENT_RUNTIME=1)

option(NR_GAME_PRE_ONLY_MENU "Hide working resolution in game menu" OFF)

  target_compile_definitions(nr_adapter_core PUBLIC NR_GAME_PRE_ONLY_MENU=1)

target_link_libraries(nr_adapter_core PUBLIC nr_menu_bridge nr_settings)
target_include_directories(nr_adapter_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(nr_adapter_core PUBLIC cxx_std_17)
add_library(nr_native_adapter SHARED export.cpp)
target_link_libraries(nr_native_adapter PRIVATE nr_adapter_core)

foreach(target nr_adapter_core nr_native_adapter)
  target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8)
endforeach()
