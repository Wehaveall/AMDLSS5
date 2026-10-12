cmake_minimum_required(VERSION 3.20)
project(nr_compat LANGUAGES CXX)

add_library(nr_compat STATIC compat.cpp)
target_compile_features(nr_compat PUBLIC cxx_std_17)
target_include_directories(nr_compat PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(nr_compat PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(nr_compat PRIVATE /W4 /WX /permissive- /EHsc)
