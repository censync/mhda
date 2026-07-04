#include "mhda/chain.hpp"

#include <string>

#include "detail.hpp"
#include "mhda/error.hpp"
#include "nss_parser.hpp"

namespace mhda {

namespace {

// build_chain extracts the chain identity (nt, ci) and the optional coin-type
// metadata (ct) from a parsed NSS component map. Shared by chain::from_nss,
// chain::from_key and the address parser.
chain build_chain(const std::unordered_map<std::string, std::string>& m) {
    auto nt_it = m.find(std::string{detail::comp_network_type});
    if (nt_it == m.end()) {
        throw parse_error(error_code::missing_network_type);
    }
    const std::string nt_norm = detail::normalize(nt_it->second);
    if (nt_norm.empty()) {
        throw parse_error(error_code::missing_network_type);
    }
    network_type nt{nt_norm};
    if (!nt.is_valid()) {
        throw parse_error(error_code::invalid_network_type,
                          std::string{"\""} + nt_norm + "\"");
    }

    auto ci_it = m.find(std::string{detail::comp_chain_id});
    if (ci_it == m.end()) {
        throw parse_error(error_code::missing_chain_id);
    }
    const auto ci_trim = detail::trim(ci_it->second);
    if (ci_trim.empty()) {
        throw parse_error(error_code::missing_chain_id);
    }

    chain out{std::move(nt), std::string{ci_trim}};

    auto ct_it = m.find(std::string{detail::comp_coin_type});
    if (ct_it != m.end()) {
        const auto ct_trim = detail::trim(ct_it->second);
        coin_type ct = 0;
        if (!detail::parse_uint32(ct_trim, ct)) {
            throw parse_error(error_code::invalid_coin_type,
                              std::string{"\""} + std::string{ct_trim} + "\"");
        }
        out.set_coin(ct);
    }

    return out;
}

}  // namespace

namespace internal {
chain build_chain_from_components(const std::unordered_map<std::string, std::string>& m) {
    return build_chain(m);
}
}  // namespace internal

chain chain::from_nss(std::string_view nss) {
    auto components = detail::parse_nss_map(nss);
    if (components.find(std::string{detail::comp_network_type}) == components.end()) {
        throw parse_error(error_code::missing_network_type);
    }
    return build_chain(components);
}

chain chain::from_key(std::string_view key) {
    const auto trimmed = detail::trim(key);
    auto components = detail::parse_nss_map(trimmed);
    if (components.find(std::string{detail::comp_coin_type}) != components.end()) {
        throw parse_error(error_code::coin_type_in_chain_key);
    }
    for (const auto& kv : components) {
        if (kv.first != detail::comp_network_type && kv.first != detail::comp_chain_id) {
            throw parse_error(error_code::invalid_chain_key,
                              std::string{"unexpected component \""} + kv.first + "\"");
        }
    }
    if (components.find(std::string{detail::comp_network_type}) == components.end()) {
        throw parse_error(error_code::missing_network_type);
    }
    chain out = build_chain(components);
    // Keys compare as plain strings, so every accepted input must BE the
    // canonical string: unknown tokens, reordering and non-canonical spelling
    // are rejected, not silently normalised.
    if (out.str() != trimmed) {
        throw parse_error(error_code::invalid_chain_key,
                          std::string{"not in canonical form: \""} +
                              std::string{trimmed} + "\"");
    }
    return out;
}

std::string chain::str() const {
    std::string out = "nt:";
    out += network_.str();
    out += ":ci:";
    out += chain_id_;
    return out;
}

}  // namespace mhda
