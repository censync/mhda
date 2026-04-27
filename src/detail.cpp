#include "detail.hpp"

#include <cstdint>
#include <limits>

namespace mhda {
namespace detail {

namespace {

bool parse_in_base(std::string_view s, unsigned base, std::uint64_t& out) noexcept {
    if (s.empty()) return false;
    std::uint64_t value = 0;
    for (char c : s) {
        unsigned digit;
        if (c >= '0' && c <= '9') {
            digit = unsigned(c - '0');
        } else if (base == 16 && c >= 'a' && c <= 'f') {
            digit = unsigned(c - 'a') + 10;
        } else if (base == 16 && c >= 'A' && c <= 'F') {
            digit = unsigned(c - 'A') + 10;
        } else {
            return false;
        }
        if (digit >= base) return false;
        // detect overflow before performing the shift+add
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / base) {
            return false;
        }
        value = value * base + digit;
    }
    out = value;
    return true;
}

}  // namespace

bool parse_uint32(std::string_view s, std::uint32_t& out) noexcept {
    if (s.empty()) return false;
    unsigned base = 10;
    std::string_view body = s;
    if (body.size() >= 2 && body[0] == '0' && (body[1] == 'x' || body[1] == 'X')) {
        base = 16;
        body.remove_prefix(2);
        if (body.empty()) return false;
    }
    std::uint64_t value = 0;
    if (!parse_in_base(body, base, value)) return false;
    if (value > std::numeric_limits<std::uint32_t>::max()) return false;
    out = std::uint32_t(value);
    return true;
}

bool parse_uint32_dec(std::string_view s, std::uint32_t& out) noexcept {
    std::uint64_t value = 0;
    if (!parse_in_base(s, 10, value)) return false;
    if (value > std::numeric_limits<std::uint32_t>::max()) return false;
    out = std::uint32_t(value);
    return true;
}

}  // namespace detail
}  // namespace mhda
