#include "mhda/network_type.hpp"

#include <unordered_map>

#include "detail.hpp"

namespace mhda {

namespace {

const std::unordered_map<std::string, network_type>& nt_index() {
    // Allocated once and never destroyed: a consumer's global destructor may
    // still parse after static destruction has begun.
    static const auto* index = new std::unordered_map<std::string, network_type>{
        {"bitcoin",   network_type::bitcoin},
        {"evm",       network_type::ethereum_vm},
        {"avalanche", network_type::avalanche_vm},
        {"tron",      network_type::tron_vm},
        {"cosmos",    network_type::cosmos},
        {"solana",    network_type::solana},
        {"xrpl",      network_type::xrp_ledger},
        {"stellar",   network_type::stellar},
        {"near",      network_type::near_protocol},
        {"aptos",     network_type::aptos},
        {"sui",       network_type::sui},
        {"cardano",   network_type::cardano},
        {"algorand",  network_type::algorand},
        {"ton",       network_type::toncoin},
    };
    return *index;
}

}  // namespace

bool network_type::is_valid() const noexcept {
    return nt_index().find(value_) != nt_index().end();
}

std::optional<network_type> network_type_from_string(std::string_view s) {
    const auto key = detail::normalize(s);
    auto it = nt_index().find(key);
    if (it == nt_index().end()) return std::nullopt;
    return it->second;
}

}  // namespace mhda
