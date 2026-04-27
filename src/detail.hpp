#pragma once

#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mhda {
namespace detail {

inline char ascii_lower(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? char(c + 32) : c;
}

// trim removes leading and trailing ASCII whitespace.
inline std::string_view trim(std::string_view s) noexcept {
    std::size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    std::size_t j = s.size();
    while (j > i && std::isspace(static_cast<unsigned char>(s[j - 1]))) --j;
    return s.substr(i, j - i);
}

// to_lower returns an ASCII-lowercased copy of s.
inline std::string to_lower(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out.push_back(ascii_lower(c));
    return out;
}

// normalize trims surrounding whitespace and ASCII-lowercases the input.
inline std::string normalize(std::string_view s) {
    return to_lower(trim(s));
}

// equal_ascii_fold compares two strings ignoring ASCII case.
inline bool equal_ascii_fold(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (ascii_lower(a[i]) != ascii_lower(b[i])) return false;
    }
    return true;
}

// has_prefix_fold reports whether s begins with prefix, ignoring ASCII case.
// RFC 8141 §5.1: the leading "urn:" sequence and the NID are case-insensitive.
inline bool has_prefix_fold(std::string_view s, std::string_view prefix) noexcept {
    if (s.size() < prefix.size()) return false;
    return equal_ascii_fold(s.substr(0, prefix.size()), prefix);
}

// strip_rqf strips the optional rq-components ("?+" / "?=") and f-component
// ("#") trailing the assigned-name part, per RFC 8141 §2.
inline std::string_view strip_rqf(std::string_view nss) noexcept {
    auto i = nss.find_first_of("?#");
    if (i == std::string_view::npos) return nss;
    return nss.substr(0, i);
}

// split splits s on a single delimiter, mirroring strings.Split. Empty fields
// between adjacent delimiters are preserved (so "a::b" -> ["a", "", "b"]).
inline std::vector<std::string_view> split(std::string_view s, char sep) {
    std::vector<std::string_view> out;
    std::size_t start = 0;
    for (std::size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == sep) {
            out.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    return out;
}

// parse_uint32 parses a decimal or "0x"-prefixed hex string into a uint32_t.
// Returns false on invalid input or 32-bit overflow. Mirrors Go's
// strconv.ParseUint(s, 0, 32).
bool parse_uint32(std::string_view s, std::uint32_t& out) noexcept;

// parse_uint32_dec parses a strictly decimal uint32_t. Used by per-level
// derivation-path parsing where Go uses base 10.
bool parse_uint32_dec(std::string_view s, std::uint32_t& out) noexcept;

}  // namespace detail
}  // namespace mhda
