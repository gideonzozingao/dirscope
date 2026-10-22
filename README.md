# dirscope

A fast command-line tool that shows where your disk space goes. Point it at a folder and it gives you a breakdown by subfolder and by file type, plus a list of the largest files, all drawn as colored bar charts right in the terminal.

Written in modern C++20 using only the standard library.

```
dirscope  /usr/share
  901.5 MiB in 35,719 files, 6,256 directories  (scanned in 1.34s)

Top-level breakdown
  fonts/     402.6 MiB  ██████████▊               44.7%  268 files
  texlive/   314.4 MiB  ████████▍                 34.9%  21,771 files
  icons/      29.2 MiB  ▊                          3.2%  5,980 files
  texmf/      28.7 MiB  ▊                          3.2%  537 files
  doc/        20.1 MiB  ▌                          2.2%  1,437 files
  … and 116 more

By file type
  .ttc     314.1 MiB  ████████▍                 34.8%  15 files
  .pfb      83.3 MiB  ██▎                        9.2%  1,400 files
  .sty      66.1 MiB  █▊                         7.3%  4,304 files
  .ttf      54.9 MiB  █▌                         6.1%  71 files
  .otf      47.8 MiB  █▎                         5.3%  154 files
  … and 372 more

Largest files
   1.   26.0 MiB  fonts/opentype/noto/NotoSerifCJK-Bold.ttc
   2.   25.3 MiB  fonts/opentype/noto/NotoSerifCJK-Medium.ttc
   3.   25.2 MiB  fonts/opentype/noto/NotoSerifCJK-SemiBold.ttc
   4.   25.1 MiB  fonts/opentype/noto/NotoSerifCJK-Regular.ttc
   5.   25.0 MiB  fonts/opentype/noto/NotoSerifCJK-Light.ttc
```

## Features

- **Three views in one scan:** usage by top-level subfolder, by file extension, and the N largest files.
- **Fast and memory-light:** the largest-files list is tracked with a fixed-size heap, so memory stays small even across millions of files.
- **Safe traversal:** folders you can't read are skipped and counted rather than crashing the scan, and symlinks are not followed, so nothing is counted twice and loops are impossible.
- **Terminal-friendly output:** Unicode bar charts with sub-character precision, human-readable sizes, and thousands separators.
- **Plays well with pipes:** color turns off automatically when output isn't a terminal, or when the `NO_COLOR` environment variable is set.
- **No dependencies:** just a C++20 compiler and CMake.

## Requirements

- A C++20 compiler: GCC 11+ or Clang 14+
- CMake 3.20 or newer
- Linux, WSL, or macOS (uses `isatty` from `<unistd.h>`)

On Ubuntu or WSL:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build
```

## Building

```bash
git clone <your-repo-url> dirscope
cd dirscope
cmake -S . -B build -G Ninja
cmake --build build
```

The executable is created at `build/dirscope`. Builds default to `Release`; for debugging, configure with `-DCMAKE_BUILD_TYPE=Debug`.

To install it so you can run `dirscope` from anywhere:

```bash
sudo cmake --install build
```

This copies the binary to `/usr/local/bin`.

## Usage

```
dirscope [PATH] [options]
```

If `PATH` is omitted, the current directory is scanned.

| Option | Description |
| --- | --- |
| `-n`, `--top N` | Number of entries to show in each section (default 10) |
| `-a`, `--all` | Include hidden files and folders (names starting with `.`) |
| `--no-color` | Disable colored output |
| `-v`, `--version` | Print the version and exit |
| `-h`, `--help` | Show help and exit |

### Examples

```bash
dirscope                     # scan the current folder
dirscope ~ -n 5              # top 5 entries per section for your home folder
dirscope . --all             # include .git, .cache and other hidden folders
dirscope /var/log > usage.txt   # save a plain-text report (color is disabled automatically)
```

On WSL you can also scan Windows folders through `/mnt/c`, for example `dirscope /mnt/c/Users/<you>/Downloads`. This works but is noticeably slower than scanning the Linux filesystem.

### Exit codes

| Code | Meaning |
| --- | --- |
| `0` | Success |
| `1` | Runtime error, such as a path that doesn't exist or isn't a folder |
| `2` | Invalid command-line usage |

## Project structure

```
dirscope/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── format.hpp     # output helpers: colors, sizes, bars, alignment
│   └── scanner.hpp    # scan options, results, and the scan() function
└── src/
    ├── format.cpp
    ├── main.cpp       # argument parsing and report printing
    └── scanner.cpp    # directory traversal and aggregation
```

## How it works

`scan()` walks the tree with `std::filesystem::recursive_directory_iterator` using the `skip_permission_denied` option. Every filesystem call takes a `std::error_code`, so unreadable entries are counted as skipped instead of throwing exceptions. Hidden folders are pruned with `disable_recursion_pending()`, so the scanner never even enters them unless `--all` is given.

As each regular file is visited, its size is added to running totals in two hash maps: one keyed by lowercase extension, and one keyed by the file's top-level folder under the scan root. The file is also pushed onto a min-heap that holds at most N entries. When the heap grows past N, the smallest entry is removed, so the heap always contains the N largest files seen so far. At the end, the maps are converted into sorted vectors and the heap is drained into a biggest-first list.

Output formatting lives in `format.cpp`. The bar charts use the Unicode eighth-block characters (`▏▎▍▌▋▊▉█`), giving each bar eight steps of precision per character cell. Column alignment counts UTF-8 code points rather than bytes, so names containing non-ASCII characters still line up.

## Roadmap

- [ ] `--depth N` for a nested tree view of folders
- [ ] Duplicate-file detection (group by size, then hash contents)
- [ ] `--json` output for use in scripts
- [ ] Parallel scanning of top-level folders with `std::jthread`
- [ ] Unit tests with GoogleTest

## License

Add a license of your choice here (MIT is a common choice for small tools).
# dirscope
