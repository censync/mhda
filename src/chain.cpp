#include "mhda/chain.hpp"

#include <string>

#include "detail.hpp"
#include "mhda/error.hpp"
#include "nss_parser.hpp"

namespace mhda {

namespace {

// build_chain extracts the (nt, ct, ci) triple from a parsed NSS component
// map. Shared by chain::from_nss and the address parser.
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

    auto ct_it = m.find(std::string{detail::comp_coin_type});
    if (ct_it == m.end()) {
        throw parse_error(error_code::missing_coin_type);
    }
    const auto ct_trim = detail::trim(ct_it->second);
    if (ct_trim.empty()) {
        throw parse_error(error_code::missing_coin_type);
    }
    coin_type ct = 0;
    if (!detail::parse_uint32(ct_trim, ct)) {
        throw parse_error(error_code::invalid_coin_type,
                          std::string{"\""} + std::string{ct_trim} + "\"");
    }

    auto ci_it = m.find(std::string{detail::comp_chain_id});
    if (ci_it == m.end()) {
        throw parse_error(error_code::missing_chain_id);
    }
    const auto ci_trim = detail::trim(ci_it->second);
    if (ci_trim.empty()) {
        throw parse_error(error_code::missing_chain_id);
    }

    return chain{std::move(nt), ct, std::string{ci_trim}};
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
    return from_nss(key);
}

namespace {

void append_uint32(std::string& out, std::uint32_t v) {
    if (v == 0) { out.push_back('0'); return; }
    char tmp[16];
    int n = 0;
    while (v) { tmp[n++] = char('0' + (v % 10)); v /= 10; }
    for (int i = n - 1; i >= 0; --i) out.push_back(tmp[i]);
}

}  // namespace

std::string chain::str() const {
    std::string out = "nt:";
    out += network_.str();
    out += ":ct:";
    append_uint32(out, coin_);
    out += ":ci:";
    out += chain_id_;
    return out;
}

}  // namespace mhda
