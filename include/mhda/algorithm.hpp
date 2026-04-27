#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace mhda {

// algorithm names a signature curve / scheme in canonical lowercase form
// (e.g. "secp256k1", "ed25519"). Empty represents an unset value.
class algorithm {
public:
    algorithm() = default;
    explicit algorithm(std::string value) : value_(std::move(value)) {}

    static const algorithm secp256k1;
    static const algorithm ed25519;
    static const algorithm sr25519;
    static const algorithm secp256r1;
    static const algorithm secp384r1;
    static const algorithm secp521r1;
    static const algorithm prime256v1;

    const std::string& str() const noexcept { return value_; }
    bool empty() const noexcept { return value_.empty(); }
    bool is_valid() const noexcept;

    bool operator==(const algorithm& other) const noexcept { return value_ == other.value_; }
    bool operator!=(const algorithm& other) const noexcept { return value_ != other.value_; }
    bool operator<(const algorithm& other) const noexcept { return value_ < other.value_; }

private:
    std::string value_;
};

// algorithm_from_string parses a string into an algorithm. Lookup is
// case-insensitive; surrounding whitespace is stripped. Returns an empty
// optional for unknown values.
std::optional<algorithm> algorithm_from_string(std::string_view s);

}  // namespace mhda

namespace std {
template <>
struct hash<mhda::algorithm> {
    std::size_t operator()(const mhda::algorithm& a) const noexcept {
        return std::hash<std::string>{}(a.str());
    }
};
}  // namespace std
