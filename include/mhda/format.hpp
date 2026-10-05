#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace mhda {

// format names a serialised address encoding (e.g. "hex", "bech32", "base58").
// Empty represents an unset value.
class format {
public:
    format() = default;
    explicit format(std::string value) : value_(std::move(value)) {}

    static const format hex;
    static const format p2pkh;
    static const format p2sh;
    static const format p2wpkh;
    static const format p2wsh;
    static const format p2tr;
    static const format bech32;
    static const format bech32m;
    static const format base58;
    static const format base32;
    static const format strkey;
    static const format base64url;
    static const format ss58;

    const std::string& str() const noexcept { return value_; }
    bool empty() const noexcept { return value_.empty(); }
    bool is_valid() const noexcept;

    bool operator==(const format& other) const noexcept { return value_ == other.value_; }
    bool operator!=(const format& other) const noexcept { return value_ != other.value_; }
    bool operator<(const format& other) const noexcept { return value_ < other.value_; }

private:
    std::string value_;
};

// The named constants are inline variables defined in this header: every
// translation unit that names one includes their definitions, so they are
// initialised before any namespace-scope object defined after the include,
// and a consumer's global constructors and destructors can use them.
inline const format format::hex       {"hex"};
inline const format format::p2pkh     {"p2pkh"};
inline const format format::p2sh      {"p2sh"};
inline const format format::p2wpkh    {"p2wpkh"};
inline const format format::p2wsh     {"p2wsh"};
inline const format format::p2tr      {"p2tr"};
inline const format format::bech32    {"bech32"};
inline const format format::bech32m   {"bech32m"};
inline const format format::base58    {"base58"};
inline const format format::base32    {"base32"};
inline const format format::strkey    {"strkey"};
inline const format format::base64url {"base64url"};
inline const format format::ss58      {"ss58"};

// format_from_string parses a string into a format. Lookup is case-insensitive;
// surrounding whitespace is stripped. Returns an empty optional for unknown
// values.
std::optional<format> format_from_string(std::string_view s);

}  // namespace mhda

namespace std {
template <>
struct hash<mhda::format> {
    std::size_t operator()(const mhda::format& f) const noexcept {
        return std::hash<std::string>{}(f.str());
    }
};
}  // namespace std
