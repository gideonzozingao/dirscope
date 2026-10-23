#include "scanner.hpp"

#include "platform.hpp"

#include <algorithm>
#include <cctype>
#include <queue>
#include <system_error>
#include <unordered_map>

namespace dirscope {
namespace {

using BucketMap = std::unordered_map<std::string, Bucket>;

std::string extension_key(const fs::path& p) {
    std::string ext = platform::to_utf8(p.extension());
    if (ext.empty()) return "(none)";
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

Bucket& bucket_for(BucketMap& map, const std::string& key) {
    auto [it, inserted] = map.try_emplace(key);
    if (inserted) it->second.name = key;
    return it->second;
}

std::vector<Bucket> sorted(BucketMap& map) {
    std::vector<Bucket> out;
    out.reserve(map.size());
    for (auto& [key, bucket] : map) out.push_back(std::move(bucket));
    std::sort(out.begin(), out.end(), [](const Bucket& a, const Bucket& b) {
        return a.bytes != b.bytes ? a.bytes > b.bytes : a.name < b.name;
    });
    return out;
}

}  // namespace

ScanResult scan(const fs::path& root, const ScanOptions& opts) {
    ScanResult result;
    BucketMap by_ext;
    BucketMap top_level;

    // Min-heap capped at top_n: the smallest of the "largest" sits on top,
    // so memory stays O(top_n) no matter how many files we visit.
    auto smaller_on_top = [](const FileEntry& a, const FileEntry& b) { return a.size > b.size; };
    std::priority_queue<FileEntry, std::vector<FileEntry>, decltype(smaller_on_top)>
        largest(smaller_on_top);

    std::error_code ec;
    fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
    if (ec) throw fs::filesystem_error("cannot open directory", root, ec);

    const fs::recursive_directory_iterator end;
    while (it != end) {
        const fs::directory_entry& entry = *it;
        const bool at_top = it.depth() == 0;

        if (!opts.include_hidden && platform::is_hidden(entry.path())) {
            if (entry.is_directory(ec)) it.disable_recursion_pending();
        } else if (entry.is_symlink(ec) ||
                   (entry.is_directory(ec) && platform::is_directory_link(entry.path()))) {
            // A link, junction or symlinked folder: never descend into it.
            it.disable_recursion_pending();
            // Skipped on purpose: avoids double counting and symlink loops.
        } else if (entry.is_directory(ec)) {
            ++result.dir_count;
            if (at_top) bucket_for(top_level, platform::to_utf8(entry.path().filename())).is_dir = true;
        } else if (entry.is_regular_file(ec)) {
            const std::uintmax_t size = entry.file_size(ec);
            if (ec) {
                ++result.skipped;
            } else {
                ++result.file_count;
                result.total_bytes += size;

                Bucket& ext = bucket_for(by_ext, extension_key(entry.path()));
                ext.bytes += size;
                ++ext.count;

                const fs::path first = *entry.path().lexically_relative(root).begin();
                Bucket& child = bucket_for(top_level, platform::to_utf8(first));
                child.bytes += size;
                ++child.count;

                largest.push({entry.path(), size});
                if (largest.size() > opts.top_n) largest.pop();
            }
        }

        ec.clear();
        it.increment(ec);
        if (ec) {
            result.incomplete = true;
            break;
        }
    }

    // Drain the heap back-to-front so the vector ends up biggest-first.
    result.largest_files.resize(largest.size());
    for (std::size_t i = largest.size(); i-- > 0;) {
        result.largest_files[i] = largest.top();
        largest.pop();
    }

    result.by_extension = sorted(by_ext);
    result.top_level = sorted(top_level);
    return result;
}

}  // namespace dirscope
