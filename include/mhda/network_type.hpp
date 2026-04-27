#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace mhda {

// network_type wraps a registered MHDA network identifier (e.g. "evm", "btc",
// "sol"). Comparison and hashing use the canonical lowercase string. Defaults
// to an empty (invalid) value.
class network_type {
public:
    network_type() = default;
    explicit network_type(std::string value) : value_(std::move(value)) {}

    static const network_type bitcoin;
    static const network_type ethereum_vm;
    static const network_type avalanche_vm;
    static const network_type tron_vm;
    static const network_type cosmos;
    static const network_type solana;
    static const network_type xrp_ledger;
    static const network_type stellar;
    static const network_type near_protocol;
    static const network_type aptos;
    static const network_type sui;
    static const network_type cardano;
    static const network_type algorand;
    static const network_type toncoin;

    const std::string& str() const noexcept { return value_; }
    bool empty() const noexcept { return value_.empty(); }
    bool is_valid() const noexcept;

    bool operator==(const network_type& other) const noexcept { return value_ == other.value_; }
    bool operator!=(const network_type& other) const noexcept { return value_ != other.value_; }
    bool operator<(const network_type& other) const noexcept { return value_ < other.value_; }

private:
    std::string value_;
};

// network_type_from_string parses a string into a network_type. Lookup is
// case-insensitive; surrounding whitespace is stripped. Returns an empty
// optional for unknown values.
std::optional<network_type> network_type_from_string(std::string_view s);

}  // namespace mhda

namespace std {
template <>
struct hash<mhda::network_type> {
    std::size_t operator()(const mhda::network_type& n) const noexcept {
        return std::hash<std::string>{}(n.str());
    }
};
}  // namespace std
