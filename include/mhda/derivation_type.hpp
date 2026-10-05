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

// The named constants are inline variables defined in this header: every
// translation unit that names one includes their definitions, so they are
// initialised before any namespace-scope object defined after the include,
// and a consumer's global constructors and destructors can use them.
inline const derivation_type derivation_type::root    {"root"};
inline const derivation_type derivation_type::bip32   {"bip32"};
inline const derivation_type derivation_type::bip44   {"bip44"};
inline const derivation_type derivation_type::bip49   {"bip49"};
inline const derivation_type derivation_type::bip54   {"bip54"};
inline const derivation_type derivation_type::bip74   {"bip74"};
inline const derivation_type derivation_type::bip84   {"bip84"};
inline const derivation_type derivation_type::bip86   {"bip86"};
inline const derivation_type derivation_type::slip10  {"slip10"};
inline const derivation_type derivation_type::cip1852 {"cip1852"};
inline const derivation_type derivation_type::cip11   {"cip11"};
inline const derivation_type derivation_type::zip32   {"zip32"};

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
