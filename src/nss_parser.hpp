#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace mhda {
namespace detail {

// MHDA URN structural constants. Mirror the prefix and component-key set used
// by the Go reference implementation.
constexpr std::string_view prefix_mhda = "urn:mhda:";

// Chain domain. The chain identity is the (nt, ci) pair; ct is the OPTIONAL
// SLIP-44 coin-type metadata (never part of the identity).
constexpr std::string_view comp_network_type      = "nt";
constexpr std::string_view comp_chain_id          = "ci";
constexpr std::string_view comp_coin_type         = "ct";
// Derivation path domain.
constexpr std::string_view comp_derivation_type   = "dt";
constexpr std::string_view comp_derivation_path   = "dp";
// Address format domain.
constexpr std::string_view comp_address_algorithm = "aa";
constexpr std::string_view comp_address_format    = "af";
constexpr std::string_view comp_address_prefix    = "ap";
constexpr std::string_view comp_address_suffix    = "as";
// Wallet domain (optional). comp_wallet_type is a free-form wallet/client
// type, e.g. "web3", "metamask", "tonconnect"; comp_wallet_id is a free-form
// wallet instance identifier, e.g. a UUID or an HD root key fingerprint.
constexpr std::string_view comp_wallet_type       = "wt";
constexpr std::string_view comp_wallet_id         = "wi";

bool is_known_component(std::string_view key) noexcept;

// parse_nss splits an NSS string into a component map; an empty NSS has no
// components. The NSS is a sequence of `key:value` pairs joined by `:`
// separators. A pair with an unknown key is skipped together with its value
// (forward-compat with future URN extensions), so a value is never read as a
// key and a following known key is never read as a value. These throw
// parse_error(invalid_nss): a key without a value (an odd number of tokens),
// an empty key or value, a key that differs from a known key only by case
// (it would drop the component silently), a duplicate known key, and a key
// or value with anything outside printable ASCII (0x21–0x7E: control bytes,
// whitespace of any kind, non-ASCII bytes). Nothing is trimmed: the caller
// removes whitespace around the whole NSS. '?' and '#' anywhere in the NSS throw parse_error(invalid_nss):
// parse_urn strips the RFC 8141 r/q/f components first, and an NSS parsed on
// its own must not carry them. Mirrors go-mhda's parseNSS.
//
// Values may not contain ':'; this holds for every component currently
// defined.
std::unordered_map<std::string, std::string> parse_nss_map(std::string_view nss);

}  // namespace detail
}  // namespace mhda
