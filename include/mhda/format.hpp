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
