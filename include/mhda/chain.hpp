#pragma once

#include <string>
#include <string_view>

#include "mhda/coin_type.hpp"
#include "mhda/network_type.hpp"

namespace mhda {

// chain_id stores the chain-domain identifier (e.g. "1", "0xa86a", "cosmoshub").
// It is opaque to the parser: any non-empty NSS-safe value is accepted.
using chain_id = std::string;

// chain_key is the canonical NSS-style chain identifier
// "nt:<network>:ct:<coin>:ci:<chainid>" returned by chain::key() and consumed
// by chain::from_key() / chain::from_nss().
using chain_key = std::string;

// chain represents the chain-domain triple (nt, ct, ci) of an MHDA address.
// All Set* methods modify the receiver in place and are NOT safe for concurrent
// use; callers must synchronise externally if mutating from multiple threads.
class chain {
public:
    chain() = default;
    chain(network_type nt, coin_type ct, chain_id ci)
        : network_(std::move(nt)), coin_(ct), chain_id_(std::move(ci)) {}

    // from_key parses a chain key produced by chain::key().
    static chain from_key(std::string_view key);

    // from_nss parses just the chain-domain components ("nt", "ct", "ci")
    // from an NSS string. Other components are tolerated and ignored.
    static chain from_nss(std::string_view nss);

    void set_network(network_type nt)  { network_  = std::move(nt); }
    void set_coin(coin_type ct)        { coin_     = ct; }
    void set_chain_id(chain_id ci)     { chain_id_ = std::move(ci); }

    const network_type& network() const noexcept { return network_; }
    coin_type         coin() const noexcept    { return coin_; }
    const chain_id&   id()   const noexcept    { return chain_id_; }

    // key returns the canonical NSS-style chain key
    // "nt:<network>:ct:<coin>:ci:<chainid>". Suitable as a map key, cache key
    // or content-hash input.
    chain_key key() const { return str(); }

    // str returns the canonical chain key form (alias of key()).
    std::string str() const;

    bool operator==(const chain& other) const noexcept {
        return network_ == other.network_ && coin_ == other.coin_ && chain_id_ == other.chain_id_;
    }
    bool operator!=(const chain& other) const noexcept { return !(*this == other); }

private:
    network_type network_;
    coin_type    coin_ = 0;
    chain_id     chain_id_;
};

}  // namespace mhda
