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

// nss_byte reports whether c may appear in an NSS key or value (SPEC §1.5):
// an RFC 3986 pchar or "/" - letters, digits and -._~!$&'()*+,;=@/ - except
// ':' (the component separator) and '%' (percent-encoding is not supported,
// and a raw "%41" would be a second spelling of "A"). Whitespace, control
// bytes, non-ASCII bytes and the printable ASCII outside pchar cannot appear
// in a conforming URN. Mirrors go-mhda's nssByte.
inline bool nss_byte(char c) noexcept {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        return true;
    }
    for (char ok : std::string_view{"-._~!$&'()*+,;=@/"}) {
        if (c == ok) return true;
    }
    return false;
}

// trim_right removes trailing ASCII whitespace only.
inline std::string_view trim_right(std::string_view s) noexcept {
    std::size_t j = s.size();
    while (j > 0 && is_ascii_space(s[j - 1])) --j;
    return s.substr(0, j);
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

// quote returns s in double quotes for an error message, escaping like Go's
// %q: '"' and '\\' get a backslash, \n \r \t their escapes, and every other
// control or non-ASCII byte \xNN. The message stays one line of printable
// ASCII, so input logged through what() cannot forge or colour log lines.
std::string quote(std::string_view s);

// validate_free_form_value guards a value written verbatim into the NSS
// (ap/as/wt/wi and the chain id) with the NSS byte set (see nss_byte): the
// ':' component separator would inject foreign components on re-parse,
// '?' / '#' would truncate the URN at the RFC 8141 r/q/f delimiters, and
// whitespace, control bytes, Unicode and the rest of the printable ASCII
// outside RFC 3986 pchar cannot appear in a conforming NSS at all. Throws
// parse_error(invalid_value). Mirrors the Go reference's
// validateFreeFormValue.
void validate_free_form_value(std::string_view component, std::string_view v);

// parse_uint31_dec parses a strictly decimal value below 2^31. Used by
// per-level derivation-path parsing: a BIP-32 child number keeps the hardened
// flag in its top bit (n' is 2^31+n), so a level index has 31 bits. Mirrors
// Go's strconv.ParseUint(s, 10, 31).
bool parse_uint31_dec(std::string_view s, std::uint32_t& out) noexcept;

}  // namespace detail
}  // namespace mhda
