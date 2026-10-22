#include "format.hpp"
#include "scanner.hpp"

#include <unistd.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

#ifndef DIRSCOPE_VERSION
#define DIRSCOPE_VERSION "dev"
#endif

namespace {

using namespace dirscope;

struct Args {
    fs::path root = ".";
    std::size_t top = 10;
    bool all = false;
    bool color = true;
    bool help = false;
    bool version = false;
};

void print_usage() {
    std::cout << "Usage: dirscope [PATH] [options]\n"
                 "\n"
                 "Show where disk space goes inside a directory tree.\n"
                 "\n"
                 "Options:\n"
                 "  -n, --top N     entries per section (default 10)\n"
                 "  -a, --all       include hidden files and directories\n"
                 "      --no-color  disable colored output (or set NO_COLOR)\n"
                 "  -v, --version   print version and exit\n"
                 "  -h, --help      show this help and exit\n";
}

std::optional<Args> parse_args(int argc, char* argv[]) {
    Args args;
    args.color = isatty(STDOUT_FILENO) && std::getenv("NO_COLOR") == nullptr;
    bool have_path = false;

    for (int i = 1; i < argc; ++i) {
        const std::string_view a = argv[i];

        if (a == "-h" || a == "--help") {
            args.help = true;
        } else if (a == "-v" || a == "--version") {
            args.version = true;
        } else if (a == "-a" || a == "--all") {
            args.all = true;
        } else if (a == "--no-color") {
            args.color = false;
        } else if (a == "-n" || a == "--top") {
            if (i + 1 >= argc) {
                std::cerr << "dirscope: " << a << " requires a number\n";
                return std::nullopt;
            }
            const std::string_view v = argv[++i];
            const auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), args.top);
            if (ec != std::errc{} || ptr != v.data() + v.size() || args.top == 0) {
                std::cerr << "dirscope: invalid number for " << a << ": '" << v << "'\n";
                return std::nullopt;
            }
        } else if (a.size() > 1 && a.front() == '-') {
            std::cerr << "dirscope: unknown option '" << a << "'\n";
            return std::nullopt;
        } else if (!have_path) {
            args.root = a;
            have_path = true;
        } else {
            std::cerr << "dirscope: only one PATH may be given\n";
            return std::nullopt;
        }
    }
    return args;
}

std::string percent(double fraction) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%5.1f%%", fraction * 100.0);
    return buf;
}

void print_heading(const Style& st, std::string_view title) {
    std::cout << '\n' << st.bold(title) << '\n';
}

void print_buckets(const std::vector<Bucket>& buckets, std::uintmax_t total, std::size_t top,
                   const Style& st, bool show_counts) {
    const std::size_t shown = std::min(top, buckets.size());

    std::size_t name_w = 6;
    for (std::size_t i = 0; i < shown; ++i) {
        name_w = std::max(name_w, buckets[i].name.size() + (buckets[i].is_dir ? 1 : 0));
    }
    name_w = std::min<std::size_t>(name_w, 28);

    for (std::size_t i = 0; i < shown; ++i) {
        const Bucket& b = buckets[i];
        const double frac = total ? static_cast<double>(b.bytes) / static_cast<double>(total) : 0.0;
        const std::string name = fit(b.is_dir ? b.name + "/" : b.name, name_w);

        std::cout << "  " << (b.is_dir ? st.cyan(name) : name) << "  "
                  << align_right(human_size(b.bytes), 10) << "  " << st.green(bar(frac, 24)) << ' '
                  << percent(frac);
        if (show_counts) {
            std::cout << st.dim("  " + with_commas(b.count) + (b.count == 1 ? " file" : " files"));
        }
        std::cout << '\n';
    }
    if (buckets.size() > shown) {
        std::cout << st.dim("  … and " + with_commas(buckets.size() - shown) + " more") << '\n';
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    const auto args = parse_args(argc, argv);
    if (!args) {
        std::cerr << "Try 'dirscope --help' for usage.\n";
        return 2;
    }
    if (args->help) {
        print_usage();
        return 0;
    }
    if (args->version) {
        std::cout << "dirscope " << DIRSCOPE_VERSION << '\n';
        return 0;
    }

    const Style st(args->color);

    std::error_code ec;
    if (!fs::is_directory(args->root, ec)) {
        std::cerr << "dirscope: not a directory: " << args->root.string() << '\n';
        return 1;
    }
    fs::path root = fs::canonical(args->root, ec);
    if (ec) root = fs::absolute(args->root);

    const auto start = std::chrono::steady_clock::now();
    ScanResult r;
    try {
        r = scan(root, ScanOptions{args->all, args->top});
    } catch (const fs::filesystem_error& e) {
        std::cerr << "dirscope: " << e.what() << '\n';
        return 1;
    }
    const double secs =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    char time_buf[32];
    std::snprintf(time_buf, sizeof time_buf, "%.2fs", secs);

    std::cout << st.bold(st.magenta("dirscope")) << "  " << root.string() << '\n'
              << "  " << st.bold(human_size(r.total_bytes)) << " in "
              << with_commas(r.file_count) << " files, " << with_commas(r.dir_count)
              << " directories" << st.dim("  (scanned in " + std::string(time_buf) + ")") << '\n';

    if (r.file_count == 0) {
        std::cout << st.dim("\n  Nothing to report — no readable files found.") << '\n';
        return 0;
    }

    print_heading(st, "Top-level breakdown");
    print_buckets(r.top_level, r.total_bytes, args->top, st, true);

    print_heading(st, "By file type");
    print_buckets(r.by_extension, r.total_bytes, args->top, st, true);

    print_heading(st, "Largest files");
    for (std::size_t i = 0; i < r.largest_files.size(); ++i) {
        const FileEntry& f = r.largest_files[i];
        std::cout << "  " << st.dim(align_right(std::to_string(i + 1) + ".", 3)) << ' '
                  << st.yellow(align_right(human_size(f.size), 10)) << "  "
                  << f.path.lexically_relative(root).string() << '\n';
    }

    if (r.skipped > 0 || r.incomplete) {
        std::cout << '\n';
        if (r.skipped > 0) {
            std::cout << st.yellow("  ! " + with_commas(r.skipped) + " entries could not be read")
                      << '\n';
        }
        if (r.incomplete) {
            std::cout << st.red("  ! scan stopped early because of a filesystem error") << '\n';
        }
    }
    return 0;
}
