#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace dirscope {

// ANSI color helper that becomes a no-op when color is disabled.
class Style {
public:
    explicit Style(bool enabled) : enabled_(enabled) {}

    std::string bold(std::string_view s) const    { return wrap(s, "1"); }
    std::string dim(std::string_view s) const     { return wrap(s, "2"); }
    std::string red(std::string_view s) const     { return wrap(s, "31"); }
    std::string green(std::string_view s) const   { return wrap(s, "32"); }
    std::string yellow(std::string_view s) const  { return wrap(s, "33"); }
    std::string magenta(std::string_view s) const { return wrap(s, "35"); }
    std::string cyan(std::string_view s) const    { return wrap(s, "36"); }

private:
    std::string wrap(std::string_view s, std::string_view code) const;
    bool enabled_;
};

std::string human_size(std::uintmax_t bytes);          // 1536 -> "1.5 KiB"
std::string with_commas(std::uintmax_t n);             // 1234567 -> "1,234,567"
std::string bar(double fraction, int width);           // Unicode block bar
std::string fit(std::string_view text, std::size_t width);          // pad/truncate, left-aligned
std::string align_right(std::string_view text, std::size_t width);  // pad on the left

}  // namespace dirscope
