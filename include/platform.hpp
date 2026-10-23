#pragma once

// Everything that differs between Windows and Linux lives behind this header.

#include <filesystem>
#include <string>
#include <string_view>

namespace dirscope::platform {

namespace fs = std::filesystem;

// Dot-files on both systems; on Windows also files marked Hidden or System.
bool is_hidden(const fs::path& path);

// Windows: true for junctions and directory symlinks, which the scanner must
// not walk into (they would count the same files twice). Linux: false, since
// the directory iterator already skips symlinked directories.
bool is_directory_link(const fs::path& path);

// Windows: switch the console to UTF-8 and enable ANSI colors. No-op on Linux.
void setup_console();
bool stdout_is_terminal();

// Convert between paths and UTF-8 text (Windows paths are UTF-16 internally).
std::string to_utf8(const fs::path& path);
fs::path from_utf8(std::string_view text);

}  // namespace dirscope::platform
