cmake_minimum_required(VERSION 3.20)
project(nr_menu_bridge LANGUAGES CXX)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Windows x64 required")
endif()
add_library(nr_menu_bridge STATIC menu_bridge.cpp menu_i18n.cpp)
target_compile_features(nr_menu_bridge PUBLIC cxx_std_17)
target_include_directories(nr_menu_bridge PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if(MSVC)
  target_compile_options(nr_menu_bridge PRIVATE /W4 /WX /permissive- /utf-8)

endif()
