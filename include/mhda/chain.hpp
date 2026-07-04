#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "mhda/coin_type.hpp"
#include "mhda/network_type.hpp"

namespace mhda {

// chain_id stores the chain-domain identifier (e.g. "1", "0xa86a", "cosmoshub").
// It is opaque to the parser: any non-empty NSS-safe value is accepted.
using chain_id = std::string;

// chain_key is the canonical NSS-style chain identifier
// "nt:<network>:ci:<chainid>" returned by chain::key() and consumed by
// chain::from_key() / chain::from_nss().
using chain_key = std::string;

// chain describes a network: the network type plus the chain id. The pair
// (nt, ci) is the chain identity; key() and str() serialize exactly that pair,
// and chain keys compare as plain strings.
//
// An optional SLIP-44 coin type may be attached as metadata (set_coin). It is
// carried by the full URN form (the "ct" component) but is NOT part of the
// chain identity: it never appears in key()/str() and never participates in
// equality.
//
// All set_* methods modify the receiver in place and are NOT safe for
// concurrent use; callers must synchronise externally if mutating from
// multiple threads.
class chain {
public:
    chain() = default;
    chain(network_type nt, chain_id ci)
        : network_(std::move(nt)), chain_id_(std::move(ci)) {}

    // from_key parses a chain key produced by chain::key(). A chain key is
    // the canonical identity form "nt:<network>:ci:<chain_id>" and nothing
    // else. "ct" (the pre-1.1 key format) is rejected with the dedicated
    // error_code::coin_type_in_chain_key so that legacy keys fail loudly
    // instead of being silently reinterpreted; any other known component and
    // any residue (unknown tokens, reordering, non-canonical spelling) is
    // rejected with error_code::invalid_chain_key — keys compare as plain
    // strings, so every accepted input must BE the canonical string.
    // Surrounding ASCII whitespace is trimmed and tolerated.
    static chain from_key(std::string_view key);

    // from_nss parses the chain-domain components ("nt", "ci" and the
    // optional "ct" metadata) from the given NSS string. Other components are
    // tolerated and ignored, so a full address NSS is valid input.
    static chain from_nss(std::string_view nss);

    void set_network(network_type nt)  { network_  = std::move(nt); }
    void set_chain_id(chain_id ci)     { chain_id_ = std::move(ci); }

    // set_coin attaches the optional SLIP-44 coin-type metadata.
    void set_coin(coin_type ct)        { coin_ = ct; }

    // clear_coin removes the optional SLIP-44 coin-type metadata.
    void clear_coin()                  { coin_.reset(); }

    const network_type& network() const noexcept { return network_; }
    const chain_id&     id()      const noexcept { return chain_id_; }

    // coin returns the optional SLIP-44 coin-type metadata, or an empty
    // optional when unset. An explicit 0 (Bitcoin) is distinct from "not set".
    const std::optional<coin_type>& coin() const noexcept { return coin_; }

    // key returns the canonical NSS-style chain key "nt:<network>:ci:<chainid>".
    // Suitable as a map key, cache key or content-hash input.
    chain_key key() const { return str(); }

    // str returns the canonical chain key form (alias of key()). The optional
    // coin-type metadata is deliberately excluded: the chain identity is the
    // (network type, chain id) pair.
    std::string str() const;

    // Equality compares the chain identity (network type, chain id) only; the
    // optional coin-type metadata never participates.
    bool operator==(const chain& other) const noexcept {
        return network_ == other.network_ && chain_id_ == other.chain_id_;
    }
    bool operator!=(const chain& other) const noexcept { return !(*this == other); }

private:
    network_type             network_;
    chain_id                 chain_id_;
    std::optional<coin_type> coin_;
};

}  // namespace mhda
