#include "mhda/parser.hpp"

#include <string>
#include <unordered_map>

#include "detail.hpp"
#include "internal.hpp"
#include "mhda/error.hpp"
#include "nss_parser.hpp"

namespace mhda {

namespace {

address parse_address_from_components(const std::unordered_map<std::string, std::string>& m) {
    auto chain = internal::build_chain_from_components(m);
    address addr{std::move(chain), std::nullopt};

    auto get = [&](std::string_view key) -> std::string {
        auto it = m.find(std::string{key});
        if (it == m.end()) return {};
        return it->second;
    };

    addr.set_derivation_type(get(detail::comp_derivation_type));
    addr.set_derivation_path(get(detail::comp_derivation_path));
    addr.set_address_algorithm(get(detail::comp_address_algorithm));
    addr.set_address_format(get(detail::comp_address_format));
    addr.set_address_prefix(get(detail::comp_address_prefix));
    addr.set_address_suffix(get(detail::comp_address_suffix));
    return addr;
}

}  // namespace

address parse_urn(std::string_view src) {
    auto trimmed = detail::trim(src);
    if (!detail::has_prefix_fold(trimmed, detail::prefix_mhda)) {
        throw parse_error(error_code::invalid_urn);
    }
    auto nss = detail::strip_rqf(trimmed.substr(detail::prefix_mhda.size()));
    return parse_nss(nss);
}

address parse_urn_strict(std::string_view src) {
    auto addr = parse_urn(src);
    addr.validate();
    return addr;
}

address parse_nss(std::string_view nss) {
    auto components = detail::parse_nss_map(nss);
    if (components.find(std::string{detail::comp_network_type}) == components.end()) {
        throw parse_error(error_code::missing_network_type);
    }
    return parse_address_from_components(components);
}

}  // namespace mhda
