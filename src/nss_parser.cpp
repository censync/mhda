#include "nss_parser.hpp"

#include <unordered_set>

#include "detail.hpp"
#include "mhda/error.hpp"

namespace mhda {
namespace detail {

namespace {

const std::unordered_set<std::string_view>& known_components() {
    static const std::unordered_set<std::string_view> set = {
        comp_network_type, comp_coin_type, comp_chain_id,
        comp_derivation_type, comp_derivation_path,
        comp_address_algorithm, comp_address_format,
        comp_address_prefix, comp_address_suffix,
    };
    return set;
}

}  // namespace

bool is_known_component(std::string_view key) noexcept {
    return known_components().find(key) != known_components().end();
}

std::unordered_map<std::string, std::string> parse_nss_map(std::string_view nss) {
    std::unordered_map<std::string, std::string> out;
    auto parts = split(nss, ':');
    for (std::size_t i = 0; i < parts.size();) {
        auto key = parts[i];
        if (!is_known_component(key)) {
            // Unknown token (could be an unrelated word, a future component
            // name, or part of a value we mis-identified). Skip and move on.
            ++i;
            continue;
        }
        if (i + 1 >= parts.size()) {
            throw parse_error(error_code::invalid_nss,
                              std::string{"missing value for \""} + std::string{key} + "\"");
        }
        // RFC 8141 NSS does not permit unescaped whitespace; trim it so any
        // trailing space (e.g. from "ci:0 #frag" where strip_rqf leaves the
        // space) does not leak into the canonical form and break round-trip.
        auto value = trim(parts[i + 1]);
        if (value.empty()) {
            throw parse_error(error_code::invalid_nss,
                              std::string{"empty value for \""} + std::string{key} + "\"");
        }
        std::string key_str{key};
        if (out.count(key_str)) {
            throw parse_error(error_code::invalid_nss,
                              std::string{"duplicate component \""} + key_str + "\"");
        }
        out.emplace(std::move(key_str), std::string{value});
        i += 2;
    }
    return out;
}

}  // namespace detail
}  // namespace mhda
