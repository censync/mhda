#pragma once

// Umbrella include for the public MHDA C++ API.
//
// MHDA (MultiChain Hierarchical Deterministic Address) is a URN-based
// descriptor for blockchain HD addresses. It is RFC 8141 compatible and
// unifies the encoding of derivation paths, signature curves and address
// formats across EVM, Bitcoin, Cosmos, Solana, XRP, Stellar, NEAR, Aptos,
// Sui, Cardano, Algorand and TON.
//
// Format:
//
//   urn:mhda:nt:<network>:ci:<chain_id>:ct:<slip44>:dt:<derivation>:dp:<path>:aa:<algorithm>:af:<format>:ap:<prefix>:as:<suffix>:wt:<wallet_type>:wi:<wallet_id>
//
// Only nt and ci are required — the (nt, ci) pair is the chain identity. The
// SLIP-44 coin type (ct) is optional metadata; ct/dt/dp/aa/af/ap/as/wt/wi are
// emitted on str() only when explicitly set, preserving short-form
// round-trip. The chain-identity prefix nt:X:ci:Y is itself a valid chain key
// returned by chain::key() and consumed by chain::from_key / chain::from_nss.

#include "mhda/address.hpp"
#include "mhda/algorithm.hpp"
#include "mhda/chain.hpp"
#include "mhda/coin_type.hpp"
#include "mhda/derivation_path.hpp"
#include "mhda/derivation_type.hpp"
#include "mhda/error.hpp"
#include "mhda/format.hpp"
#include "mhda/network_type.hpp"
#include "mhda/parser.hpp"
