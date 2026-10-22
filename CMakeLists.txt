cmake_minimum_required(VERSION 3.20)
project(dirscope VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(dirscope
  src/main.cpp
  src/scanner.cpp
  src/format.cpp
)

target_include_directories(dirscope PRIVATE include)
target_compile_definitions(dirscope PRIVATE DIRSCOPE_VERSION="${PROJECT_VERSION}")
target_compile_options(dirscope PRIVATE -Wall -Wextra -Wpedantic -Wshadow)

install(TARGETS dirscope RUNTIME DESTINATION bin)
