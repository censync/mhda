#include "compatibility.hpp"

#include <unordered_map>
#include <unordered_set>

namespace mhda {
namespace detail {

namespace {

struct network_compat {
    std::unordered_set<algorithm>       algorithms;
    std::unordered_set<format>          formats;
    std::unordered_set<derivation_type> derivations;
    algorithm default_algorithm;
    format    default_format;
    // derivation_algorithm binds a derivation type to the one algorithm it
    // derives on this network; unbound types take any listed algorithm.
    std::unordered_map<derivation_type, algorithm> derivation_algorithm = {};
    // derivation_formats binds a derivation type to the formats its purpose
    // defines, checked when a format is resolved; unbound types take any.
    std::unordered_map<derivation_type, std::unordered_set<format>> derivation_formats = {};
};

const std::unordered_map<network_type, network_compat>& matrix() {
    // Allocated once and never destroyed: a consumer's global destructor may
    // still parse after static destruction has begun.
    static const auto* cache = new std::unordered_map<network_type, network_compat>([]() {
        std::unordered_map<network_type, network_compat> m;
        // Bitcoin
        m.emplace(network_type::bitcoin, network_compat{
            {algorithm::secp256k1},
            {format::p2pkh, format::p2sh, format::p2wpkh, format::p2wsh,
             format::p2tr, format::bech32, format::bech32m},
            {derivation_type::bip32, derivation_type::bip44,
             derivation_type::bip49, derivation_type::bip84,
             derivation_type::bip86},
            algorithm::secp256k1,
            format{},  // no default — multiple legitimate scripts
            {},
            // The purpose defines the script, so an explicit format must
            // match it.
            {
                {derivation_type::bip44, {format::p2pkh}},
                {derivation_type::bip49, {format::p2sh}},
                {derivation_type::bip84, {format::p2wpkh, format::bech32}},
                {derivation_type::bip86, {format::p2tr, format::bech32m}},
            },
        });
        // EthereumVM
        m.emplace(network_type::ethereum_vm, network_compat{
            {algorithm::secp256k1},
            {format::hex},
            {derivation_type::bip32, derivation_type::bip44},
            algorithm::secp256k1,
            format::hex,
        });
        // AvalancheVM
        m.emplace(network_type::avalanche_vm, network_compat{
            {algorithm::secp256k1},
            {format::hex, format::bech32},
            {derivation_type::bip44},
            algorithm::secp256k1,
            format{},  // C-Chain hex, X/P-Chain bech32
        });
        // TronVM
        m.emplace(network_type::tron_vm, network_compat{
            {algorithm::secp256k1},
            {format::base58},
            {derivation_type::bip44},
            algorithm::secp256k1,
            format::base58,
        });
        // Cosmos
        m.emplace(network_type::cosmos, network_compat{
            {algorithm::secp256k1, algorithm::ed25519},
            {format::bech32},
            {derivation_type::bip44, derivation_type::cip11},
            algorithm::secp256k1,
            format::bech32,
        });
        // Solana
        m.emplace(network_type::solana, network_compat{
            {algorithm::ed25519},
            {format::base58},
            {derivation_type::slip10},
            algorithm::ed25519,
            format::base58,
        });
        // XRP Ledger
        m.emplace(network_type::xrp_ledger, network_compat{
            {algorithm::secp256k1, algorithm::ed25519},
            {format::base58},
            {derivation_type::bip44},
            algorithm::secp256k1,
            format::base58,
        });
        // Stellar
        m.emplace(network_type::stellar, network_compat{
            {algorithm::ed25519},
            {format::strkey},
            {derivation_type::slip10},
            algorithm::ed25519,
            format::strkey,
        });
        // NEAR Protocol
        m.emplace(network_type::near_protocol, network_compat{
            {algorithm::ed25519, algorithm::secp256k1},
            {format::hex},
            {derivation_type::slip10, derivation_type::bip44},
            algorithm::ed25519,
            format::hex,
            {{derivation_type::slip10, algorithm::ed25519},
             {derivation_type::bip44, algorithm::secp256k1}},
        });
        // Aptos
        m.emplace(network_type::aptos, network_compat{
            {algorithm::ed25519, algorithm::secp256k1},
            {format::hex},
            {derivation_type::slip10, derivation_type::bip44},
            algorithm::ed25519,
            format::hex,
            {{derivation_type::slip10, algorithm::ed25519},
             {derivation_type::bip44, algorithm::secp256k1}},
        });
        // Sui
        m.emplace(network_type::sui, network_compat{
            {algorithm::ed25519, algorithm::secp256k1, algorithm::secp256r1},
            {format::hex},
            {derivation_type::slip10, derivation_type::bip54, derivation_type::bip74},
            algorithm::ed25519,
            format::hex,
            {{derivation_type::slip10, algorithm::ed25519},
             {derivation_type::bip54, algorithm::secp256k1},
             {derivation_type::bip74, algorithm::secp256r1}},
        });
        // Cardano
        m.emplace(network_type::cardano, network_compat{
            {algorithm::ed25519},
            {format::bech32, format::base58},
            {derivation_type::cip1852},
            algorithm::ed25519,
            format::bech32,
        });
        // Algorand
        m.emplace(network_type::algorand, network_compat{
            {algorithm::ed25519},
            {format::base32},
            {derivation_type::slip10},
            algorithm::ed25519,
            format::base32,
        });
        // TON
        m.emplace(network_type::toncoin, network_compat{
            {algorithm::ed25519},
            {format::base64url, format::hex},
            {derivation_type::slip10},
            algorithm::ed25519,
            format::base64url,
        });
        return m;
    }());
    return *cache;
}

}  // namespace

algorithm default_algorithm(const network_type& nt) {
    auto it = matrix().find(nt);
    if (it == matrix().end()) return algorithm{};
    return it->second.default_algorithm;
}

format default_format(const network_type& nt) {
    auto it = matrix().find(nt);
    if (it == matrix().end()) return format{};
    return it->second.default_format;
}

bool network_is_registered(const network_type& nt) {
    return matrix().find(nt) != matrix().end();
}

bool network_allows_algorithm(const network_type& nt, const algorithm& algo) {
    auto it = matrix().find(nt);
    if (it == matrix().end()) return false;
    return it->second.algorithms.find(algo) != it->second.algorithms.end();
}

bool network_allows_format(const network_type& nt, const format& fmt) {
    auto it = matrix().find(nt);
    if (it == matrix().end()) return false;
    return it->second.formats.find(fmt) != it->second.formats.end();
}

algorithm derivation_algorithm(const network_type& nt, const derivation_type& dt) {
    auto it = matrix().find(nt);
    if (it == matrix().end()) return algorithm{};
    auto bound = it->second.derivation_algorithm.find(dt);
    if (bound == it->second.derivation_algorithm.end()) return algorithm{};
    return bound->second;
}

bool derivation_allows_format(const network_type& nt, const derivation_type& dt, const format& fmt) {
    auto it = matrix().find(nt);
    if (it == matrix().end()) return true;
    auto bound = it->second.derivation_formats.find(dt);
    if (bound == it->second.derivation_formats.end()) return true;
    return bound->second.find(fmt) != bound->second.end();
}

bool network_allows_derivation(const network_type& nt, const derivation_type& dt) {
    auto it = matrix().find(nt);
    if (it == matrix().end()) return false;
    if (dt == derivation_type::root) return true;
    return it->second.derivations.find(dt) != it->second.derivations.end();
}

}  // namespace detail
}  // namespace mhda
