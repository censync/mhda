#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "mhda/coin_type.hpp"
#include "mhda/derivation_type.hpp"

namespace mhda {

using account_index = std::uint32_t;
using charge_type   = std::uint8_t;

constexpr charge_type charge_external = 0;
constexpr charge_type charge_internal = 1;

// address_index represents a single level of a derivation path (an unsigned
// 32-bit index plus a hardening flag).
struct address_index {
    std::uint32_t index = 0;
    bool          is_hardened = false;

    bool operator==(const address_index& other) const noexcept {
        return index == other.index && is_hardened == other.is_hardened;
    }
    bool operator!=(const address_index& other) const noexcept { return !(*this == other); }
};

// derivation_path is the canonical representation of a hierarchical-deterministic
// path. For BIP-family schemes (BIP-32/44/49/54/74/84/86, CIP-11, CIP-1852,
// ZIP-32) the BIP-44 shortcut fields (coin/account/charge/index) are kept as
// convenient accessors and stay in sync with the underlying levels[] view.
//
// For SLIP-10 (variable length) levels[] is the source of truth; the BIP-family
// shortcuts are not meaningful and are left at zero.
//
// All set/parse methods modify the receiver in place and are NOT safe for
// concurrent use; callers must synchronise externally if mutating from multiple
// threads. Read-only operations (str, levels, accessors) are safe to invoke
// concurrently provided the receiver is not being mutated.
class derivation_path {
public:
    derivation_path() = default;

    // Construct a path for a fixed-shape BIP-family scheme. SLIP-10 cannot be
    // reconstructed from the five shortcut fields; passing it here throws
    // std::invalid_argument (mirroring the panic in the Go implementation).
    derivation_path(derivation_type dt,
                    coin_type coin,
                    account_index account,
                    charge_type charge,
                    address_index index);

    // Construct a path from an explicit sequence of levels. Required for
    // SLIP-10 and a convenient alternative for any scheme. For fixed-shape
    // schemes the shortcut fields are populated from the levels.
    static derivation_path from_levels(derivation_type dt,
                                       std::vector<address_index> levels);

    // Parse a textual path (e.g. "m/44'/60'/0'/0/0") under the given derivation
    // type. Throws parse_error on shape or numeric errors.
    static derivation_path parse(derivation_type dt, std::string_view path);

    const derivation_type& type() const noexcept { return type_; }
    coin_type     coin() const noexcept { return coin_; }
    account_index account() const noexcept { return account_; }
    charge_type   charge() const noexcept { return charge_; }
    address_index index() const noexcept { return index_; }
    bool          is_hardened_address() const noexcept { return index_.is_hardened; }
    bool          has_index() const noexcept { return has_index_; }

    // Canonical level-by-level view of the path. Empty for ROOT.
    const std::vector<address_index>& levels() const noexcept { return levels_; }

    // str renders the canonical textual form. Hardening markers are emitted as
    // "'" regardless of which form ("'", "H", "h") was supplied on input.
    std::string str() const;

    // parse_path replaces the contents of this path with the result of parsing
    // the given string under the path's current derivation type. Throws
    // parse_error on failure. The derivation type must already be set (via
    // construction or set_type); for ROOT, path must be empty.
    void parse_path(std::string_view path);

    void set_type(const derivation_type& dt);

private:
    void rebuild_levels();
    void populate_shortcuts_from_levels();
    bool fixed_prefix(std::uint32_t& purpose, std::uint32_t& coin) const;

    derivation_type type_;
    coin_type       coin_ = 0;
    account_index   account_ = 0;
    charge_type     charge_ = 0;
    address_index   index_;
    bool            has_index_ = false;
    std::vector<address_index> levels_;
};

// validate_derivation_path returns true if path matches the regex of
// derivation type dt. Used internally and surfaced for callers who want to
// pre-flight a string without constructing a derivation_path.
bool validate_derivation_path(const derivation_type& dt, std::string_view path);

}  // namespace mhda
