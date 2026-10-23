#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dirscope {

namespace fs = std::filesystem;

struct FileEntry {
    fs::path path;
    std::uintmax_t size = 0;
};

// An aggregated group of files: one extension, or one top-level child.
struct Bucket {
    std::string name;
    std::uintmax_t bytes = 0;
    std::size_t count = 0;
    bool is_dir = false;
};

struct ScanOptions {
    bool include_hidden = false;
    std::size_t top_n = 10;
};

struct ScanResult {
    std::uintmax_t total_bytes = 0;
    std::size_t file_count = 0;
    std::size_t dir_count = 0;
    std::size_t skipped = 0;      // entries we couldn't read
    bool incomplete = false;      // traversal stopped early on an error

    std::vector<FileEntry> largest_files;  // sorted, biggest first
    std::vector<Bucket> by_extension;      // sorted, biggest first
    std::vector<Bucket> top_level;         // immediate children of root
};

// Walks `root` recursively. Symlinks are not followed.
// Throws fs::filesystem_error if `root` can't be opened.
ScanResult scan(const fs::path& root, const ScanOptions& opts);

}  // namespace dirscope
