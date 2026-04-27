#include "mhda/network_type.hpp"

#include <unordered_map>

#include "detail.hpp"

namespace mhda {

const network_type network_type::bitcoin       {"btc"};
const network_type network_type::ethereum_vm   {"evm"};
const network_type network_type::avalanche_vm  {"avm"};
const network_type network_type::tron_vm       {"tvm"};
const network_type network_type::cosmos        {"cosmos"};
const network_type network_type::solana        {"sol"};
const network_type network_type::xrp_ledger    {"xrp"};
const network_type network_type::stellar       {"xlm"};
const network_type network_type::near_protocol {"near"};
const network_type network_type::aptos         {"apt"};
const network_type network_type::sui           {"sui"};
const network_type network_type::cardano       {"ada"};
const network_type network_type::algorand      {"algo"};
const network_type network_type::toncoin       {"ton"};

namespace {

const std::unordered_map<std::string, network_type>& nt_index() {
    static const std::unordered_map<std::string, network_type> index = {
        {"btc",    network_type::bitcoin},
        {"evm",    network_type::ethereum_vm},
        {"avm",    network_type::avalanche_vm},
        {"tvm",    network_type::tron_vm},
        {"cosmos", network_type::cosmos},
        {"sol",    network_type::solana},
        {"xrp",    network_type::xrp_ledger},
        {"xlm",    network_type::stellar},
        {"near",   network_type::near_protocol},
        {"apt",    network_type::aptos},
        {"sui",    network_type::sui},
        {"ada",    network_type::cardano},
        {"algo",   network_type::algorand},
        {"ton",    network_type::toncoin},
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
