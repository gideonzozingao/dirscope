# Cross-compile a Windows dirscope.exe from Linux or WSL with MinGW-w64.
#
#   sudo apt install mingw-w64
#   cmake -S . -B build-windows -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake
#   cmake --build build-windows        # or simply: make windows

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

# The "-posix" compiler variants are the ones that support std::thread.
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc-posix)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++-posix)
set(CMAKE_RC_COMPILER  ${TOOLCHAIN_PREFIX}-windres)

# Look for headers and libraries in the MinGW sysroot, programs on the host.
set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Link the C++ runtime statically: the .exe runs on any Windows PC on its own.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT "-s")  # strip debug symbols: smaller .exe

# If Wine is installed, `ctest` can run the Windows test programs right here.
find_program(WINE_EXECUTABLE NAMES wine wine64)
if(WINE_EXECUTABLE)
  set(CMAKE_CROSSCOMPILING_EMULATOR ${WINE_EXECUTABLE})
endif()
