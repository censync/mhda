#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace mhda {
namespace detail {

// MHDA URN structural constants. Mirror the prefix and component-key set used
// by the Go reference implementation.
constexpr std::string_view prefix_mhda = "urn:mhda:";

constexpr std::string_view comp_network_type      = "nt";
constexpr std::string_view comp_coin_type         = "ct";
constexpr std::string_view comp_chain_id          = "ci";
constexpr std::string_view comp_derivation_type   = "dt";
constexpr std::string_view comp_derivation_path   = "dp";
constexpr std::string_view comp_address_algorithm = "aa";
constexpr std::string_view comp_address_format    = "af";
constexpr std::string_view comp_address_prefix    = "ap";
constexpr std::string_view comp_address_suffix    = "as";

bool is_known_component(std::string_view key) noexcept;

// parse_nss splits an NSS string into a component map. Unknown keys are
// silently skipped (forward-compat with future URN extensions); duplicate keys
// and empty values throw parse_error(invalid_nss).
//
// Form: a sequence of `key:value` pairs joined by `:` separators. Values may
// not contain ':'; this holds for every component currently defined.
std::unordered_map<std::string, std::string> parse_nss_map(std::string_view nss);

}  // namespace detail
}  // namespace mhda
