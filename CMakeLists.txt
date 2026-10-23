cmake_minimum_required(VERSION 3.20)
project(dirscope VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(dirscope
  src/main.cpp
  src/scanner.cpp
  src/format.cpp
  src/platform.cpp
)

target_include_directories(dirscope PRIVATE include)
target_compile_definitions(dirscope PRIVATE DIRSCOPE_VERSION="${PROJECT_VERSION}")
if(MSVC)
  # /utf-8: the sources contain UTF-8 characters such as █, · and …
  target_compile_options(dirscope PRIVATE /W4 /permissive- /utf-8)
  target_compile_definitions(dirscope PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
  target_compile_options(dirscope PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
endif()

if(WIN32)
  target_compile_definitions(dirscope PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE _WIN32_WINNT=0x0A00)
  # Embed the manifest (long paths, UTF-8) into dirscope.exe.
  set(DIRSCOPE_MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/windows/dirscope.manifest)
  if(MSVC)
    target_sources(dirscope PRIVATE ${DIRSCOPE_MANIFEST})
  elseif(MINGW)
    enable_language(RC)
    configure_file(windows/dirscope.rc.in ${CMAKE_CURRENT_BINARY_DIR}/dirscope.rc @ONLY)
    target_sources(dirscope PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/dirscope.rc)
    target_compile_definitions(dirscope PRIVATE __USE_MINGW_ANSI_STDIO=1)
    target_link_options(dirscope PRIVATE -municode)  # entry point is wmain()
  endif()
endif()

install(TARGETS dirscope RUNTIME DESTINATION bin)

# ── Tests: run with `ctest --test-dir build` or `make test` ──
enable_testing()

add_test(NAME version   COMMAND dirscope --version)
add_test(NAME help      COMMAND dirscope --help)
add_test(NAME scan_self COMMAND dirscope ${PROJECT_SOURCE_DIR} -n 3 --no-color)
add_test(NAME rejects_bad_number   COMMAND dirscope -n abc)
add_test(NAME rejects_missing_path COMMAND dirscope /does/not/exist)

set_tests_properties(version PROPERTIES
  PASS_REGULAR_EXPRESSION "dirscope [0-9]+\\.[0-9]+\\.[0-9]+")
set_tests_properties(rejects_bad_number rejects_missing_path PROPERTIES WILL_FAIL TRUE)
