#include "mhda/network_type.hpp"

#include <unordered_map>

#include "detail.hpp"

namespace mhda {

// Network types use the commonly accepted network names, lowercase. Family
// types that cover a single ecosystem carry that ecosystem's name (tron,
// avalanche); "evm" stays as-is because it covers many independent networks.
const network_type network_type::bitcoin       {"bitcoin"};
const network_type network_type::ethereum_vm   {"evm"};
const network_type network_type::avalanche_vm  {"avalanche"};
const network_type network_type::tron_vm       {"tron"};
const network_type network_type::cosmos        {"cosmos"};
const network_type network_type::solana        {"solana"};
const network_type network_type::xrp_ledger    {"xrpl"};
const network_type network_type::stellar       {"stellar"};
const network_type network_type::near_protocol {"near"};
const network_type network_type::aptos         {"aptos"};
const network_type network_type::sui           {"sui"};
const network_type network_type::cardano       {"cardano"};
const network_type network_type::algorand      {"algorand"};
const network_type network_type::toncoin       {"ton"};

namespace {

const std::unordered_map<std::string, network_type>& nt_index() {
    static const std::unordered_map<std::string, network_type> index = {
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
    return index;
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
