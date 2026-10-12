cmake_minimum_required(VERSION 3.16)
project(nr_settings LANGUAGES CXX)
find_package(Threads REQUIRED)
add_library(nr_settings STATIC settings.cpp)
target_include_directories(nr_settings PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(nr_settings PUBLIC cxx_std_17)
target_link_libraries(nr_settings PUBLIC Threads::Threads)

if(MSVC)
  target_compile_options(nr_settings PRIVATE /W4 /WX)

else()
  target_compile_options(nr_settings PRIVATE -Wall -Wextra -Wpedantic -Werror)

endif()
