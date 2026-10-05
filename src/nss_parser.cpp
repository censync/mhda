#include "nss_parser.hpp"

#include <unordered_set>

#include "detail.hpp"
#include "mhda/error.hpp"

namespace mhda {
namespace detail {

namespace {

const std::unordered_set<std::string_view>& known_components() {
    // Allocated once and never destroyed: a consumer's global destructor may
    // still parse after static destruction has begun.
    static const auto* set = new std::unordered_set<std::string_view>{
        comp_network_type, comp_chain_id, comp_coin_type,
        comp_derivation_type, comp_derivation_path,
        comp_address_algorithm, comp_address_format,
        comp_address_prefix, comp_address_suffix,
        comp_wallet_type, comp_wallet_id,
    };
    return *set;
}

}  // namespace

bool is_known_component(std::string_view key) noexcept {
    return known_components().find(key) != known_components().end();
}

std::unordered_map<std::string, std::string> parse_nss_map(std::string_view nss) {
    std::unordered_map<std::string, std::string> out;
    if (nss.empty()) return out;
    // '?' and '#' open the RFC 8141 r/q/f components. parse_urn strips them
    // before the NSS reaches this parser; an NSS that still carries one
    // (parse_nss, chain::from_nss, chain::from_key) is rejected, since the
    // URN emitted from it would be truncated at that byte on the next parse.
    if (const auto i = nss.find_first_of("?#"); i != std::string_view::npos) {
        throw parse_error(error_code::invalid_nss,
                          std::string{"'"} + nss[i] + "' inside the NSS");
    }
    auto parts = split(nss, ':');
    if (parts.size() % 2 != 0) {
        throw parse_error(error_code::invalid_nss,
                          std::string{"missing value for "} + quote(parts.back()));
    }
    for (std::size_t i = 0; i < parts.size(); i += 2) {
        const auto key = parts[i];
        if (key.empty()) {
            throw parse_error(error_code::invalid_nss, "empty component key");
        }
        for (char c : key) {
            if (!nss_byte(c)) {
                throw parse_error(error_code::invalid_nss,
                                  std::string{"byte not allowed in component key "} + quote(key));
            }
        }
        // Nothing is trimmed: the caller removes whitespace around the whole
        // NSS, and whitespace around a key or value is malformed input.
        const auto value = parts[i + 1];
        if (value.empty()) {
            throw parse_error(error_code::invalid_nss,
                              std::string{"empty value for "} + quote(key));
        }
        // Everything that survives the trim must be printable ASCII (SPEC
        // §1.5) — interior whitespace, control bytes and Unicode spaces are
        // all malformed input, never silently normalised.
        for (char c : value) {
            if (!nss_byte(c)) {
                throw parse_error(error_code::invalid_nss,
                                  std::string{"byte not allowed in value for "} + quote(key));
            }
        }
        if (!is_known_component(key)) {
            if (is_known_component(to_lower(key))) {
                throw parse_error(error_code::invalid_nss,
                                  "component key " + quote(key) + " must be lowercase");
            }
            continue;  // unknown component, skipped with its value
        }
        std::string key_str{key};
        if (out.count(key_str)) {
            throw parse_error(error_code::invalid_nss,
                              std::string{"duplicate component "} + quote(key_str));
        }
        out.emplace(std::move(key_str), std::string{value});
    }
    return out;
}

}  // namespace detail
}  // namespace mhda
