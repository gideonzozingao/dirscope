#include "platform.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#endif

#include <cstdio>

namespace dirscope::platform {
namespace {

bool is_dot_name(const fs::path& path) {
    const fs::path filename = path.filename();  // keep alive: native() refers into it
    const auto& name = filename.native();       // std::wstring on Windows
    return name.size() > 1 && name.front() == '.';
}

}  // namespace

std::string to_utf8(const fs::path& path) {
    const std::u8string s = path.u8string();
    return std::string(reinterpret_cast<const char*>(s.data()), s.size());
}

fs::path from_utf8(std::string_view text) {
    return fs::path(std::u8string(reinterpret_cast<const char8_t*>(text.data()), text.size()));
}

#ifdef _WIN32
// ─────────────────────────────── Windows ───────────────────────────────

bool is_hidden(const fs::path& path) {
    if (is_dot_name(path)) return true;
    const DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES &&
           (attrs & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0;
}

bool is_directory_link(const fs::path& path) {
    const DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) &&
           (attrs & FILE_ATTRIBUTE_REPARSE_POINT);
}

void setup_console() {
    SetConsoleOutputCP(CP_UTF8);
    for (const DWORD which : {STD_OUTPUT_HANDLE, STD_ERROR_HANDLE}) {
        HANDLE h = GetStdHandle(which);
        DWORD mode = 0;
        if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode)) {
            SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
}

bool stdout_is_terminal() { return _isatty(_fileno(stdout)) != 0; }

#else
// ──────────────────────────── Linux / macOS ────────────────────────────

bool is_hidden(const fs::path& path) { return is_dot_name(path); }

bool is_directory_link(const fs::path&) { return false; }

void setup_console() {}

bool stdout_is_terminal() { return isatty(STDOUT_FILENO) != 0; }

#endif

}  // namespace dirscope::platform
