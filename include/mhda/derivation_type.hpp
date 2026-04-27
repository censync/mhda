#pragma once

#include <string>
#include <string_view>

namespace mhda {

// derivation_type names a path-shape scheme (e.g. "bip44", "slip10"). The
// special value "root" represents the non-HD form (no path). Empty means the
// value has not been set.
class derivation_type {
public:
    derivation_type() = default;
    explicit derivation_type(std::string value) : value_(std::move(value)) {}

    static const derivation_type root;
    static const derivation_type bip32;
    static const derivation_type bip44;
    static const derivation_type bip49;
    static const derivation_type bip54;
    static const derivation_type bip74;
    static const derivation_type bip84;
    static const derivation_type bip86;
    static const derivation_type slip10;
    static const derivation_type cip1852;
    static const derivation_type cip11;
    static const derivation_type zip32;

    const std::string& str() const noexcept { return value_; }
    bool empty() const noexcept { return value_.empty(); }
    bool is_valid() const noexcept;

    bool operator==(const derivation_type& other) const noexcept { return value_ == other.value_; }
    bool operator!=(const derivation_type& other) const noexcept { return value_ != other.value_; }
    bool operator<(const derivation_type& other) const noexcept { return value_ < other.value_; }

private:
    std::string value_;
};

// derivation_type_from_string parses a string into a derivation_type. Lookup
// is case-insensitive; surrounding whitespace is stripped. Throws parse_error
// with code error_code::invalid_derivation_type for unknown values.
derivation_type derivation_type_from_string(std::string_view s);

}  // namespace mhda

namespace std {
template <>
struct hash<mhda::derivation_type> {
    std::size_t operator()(const mhda::derivation_type& d) const noexcept {
        return std::hash<std::string>{}(d.str());
    }
};
}  // namespace std
