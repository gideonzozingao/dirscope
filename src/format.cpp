#include "format.hpp"

#include <algorithm>
#include <array>
#include <cstdio>

namespace dirscope {

// Counts UTF-8 code points, so "…" and non-English names align correctly.
std::size_t display_width(std::string_view s) {
    return static_cast<std::size_t>(
        std::count_if(s.begin(), s.end(), [](unsigned char c) { return (c & 0xC0) != 0x80; }));
}

std::string Style::wrap(std::string_view s, std::string_view code) const {
    if (!enabled_) return std::string(s);
    std::string out = "\x1b[";
    out += code;
    out += 'm';
    out += s;
    out += "\x1b[0m";
    return out;
}

std::string human_size(std::uintmax_t bytes) {
    static constexpr std::array<const char*, 5> units{"B", "KiB", "MiB", "GiB", "TiB"};
    double value = static_cast<double>(bytes);
    std::size_t unit = 0;
    while (value >= 1024.0 && unit + 1 < units.size()) {
        value /= 1024.0;
        ++unit;
    }
    char buf[32];
    if (unit == 0) {
        std::snprintf(buf, sizeof buf, "%ju B", bytes);
    } else {
        std::snprintf(buf, sizeof buf, "%.1f %s", value, units[unit]);
    }
    return buf;
}

std::string with_commas(std::uintmax_t n) {
    const std::string digits = std::to_string(n);
    std::string out;
    for (std::size_t i = 0; i < digits.size(); ++i) {
        if (i != 0 && (digits.size() - i) % 3 == 0) out += ',';
        out += digits[i];
    }
    return out;
}

std::string bar(double fraction, int width) {
    // Eighth-blocks give the bar sub-character precision.
    static constexpr std::array<const char*, 8> partial{"", "▏", "▎", "▍", "▌", "▋", "▊", "▉"};
    fraction = std::clamp(fraction, 0.0, 1.0);
    const int eighths = static_cast<int>(fraction * width * 8 + 0.5);

    std::string out;
    for (int i = 0; i < eighths / 8; ++i) out += "█";
    out += partial[static_cast<std::size_t>(eighths % 8)];

    const int used = eighths / 8 + (eighths % 8 != 0 ? 1 : 0);
    out.append(static_cast<std::size_t>(width - used), ' ');
    return out;
}

std::string fit(std::string_view text, std::size_t width) {
    if (width == 0) return {};
    const std::size_t w = display_width(text);
    if (w <= width) {
        std::string out(text);
        out.append(width - w, ' ');
        return out;
    }
    std::string out;
    std::size_t code_points = 0;
    for (char ch : text) {
        const auto c = static_cast<unsigned char>(ch);
        if ((c & 0xC0) != 0x80) {
            if (code_points == width - 1) break;
            ++code_points;
        }
        out += ch;
    }
    out += "…";
    return out;
}

std::string align_right(std::string_view text, std::size_t width) {
    const std::size_t w = display_width(text);
    std::string out(w < width ? width - w : 0, ' ');
    out += text;
    return out;
}

}  // namespace dirscope
