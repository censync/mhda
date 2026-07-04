#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mhda {
namespace detail {

inline char ascii_lower(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? char(c + 32) : c;
}

// is_ascii_space matches exactly the six ASCII whitespace characters
// (" \t\n\v\f\r"), mirroring Go's asciiTrim cutset. Deliberately not
// std::isspace: that is locale-sensitive, and Unicode spaces (NBSP,
// ideographic space, ...) must NOT count as whitespace — an NSS is ASCII by
// definition (RFC 8141), so a Unicode space is not decoration to strip; it
// stays in place and the value charset check rejects it loudly.
inline bool is_ascii_space(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

// trim removes leading and trailing ASCII whitespace only (see is_ascii_space).
inline std::string_view trim(std::string_view s) noexcept {
    std::size_t i = 0;
    while (i < s.size() && is_ascii_space(s[i])) ++i;
    std::size_t j = s.size();
    while (j > i && is_ascii_space(s[j - 1])) --j;
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

// parse_uint32 parses the two documented "ct" spellings — plain decimal or
// "0x"/"0X"-prefixed hex — into a uint32_t. Returns false on invalid input or
// 32-bit overflow. Mirrors Go's parseCoinType: Go integer-literal extras
// (0o/0b prefixes, digit-group underscores, signs) are deliberately rejected,
// a leading zero is plain decimal ("060" == 60, never octal), and a bare "0x"
// with no digits is invalid. Allowing several spellings of one value would
// defeat duplicate detection and the canonical-form guarantees. Used only by
// the coin-type ("ct") parsing paths.
bool parse_uint32(std::string_view s, std::uint32_t& out) noexcept;

// parse_uint32_dec parses a strictly decimal uint32_t. Used by per-level
// derivation-path parsing where Go uses base 10.
bool parse_uint32_dec(std::string_view s, std::uint32_t& out) noexcept;

}  // namespace detail
}  // namespace mhda
