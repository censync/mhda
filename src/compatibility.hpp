#pragma once

#include "mhda/algorithm.hpp"
#include "mhda/derivation_type.hpp"
#include "mhda/format.hpp"
#include "mhda/network_type.hpp"

namespace mhda {
namespace detail {

// default_algorithm returns the per-network default signature algorithm, or an
// empty algorithm if the network is unknown or has no default.
algorithm default_algorithm(const network_type& nt);

// default_format returns the per-network default address format, or an empty
// format if the network is unknown or has multiple legitimate formats and no
// default.
format default_format(const network_type& nt);

// network_allows_algorithm reports whether nt registers algo as a valid
// signature algorithm.
bool network_allows_algorithm(const network_type& nt, const algorithm& algo);

// network_allows_format reports whether nt registers fmt as a valid address
// format.
bool network_allows_format(const network_type& nt, const format& fmt);

// network_allows_derivation reports whether nt registers dt as a valid
// derivation scheme. ROOT is considered valid for every registered network.
bool network_allows_derivation(const network_type& nt, const derivation_type& dt);

// derivation_algorithm returns the one algorithm dt derives on nt (Sui:
// slip10 ed25519, bip54 secp256k1, bip74 secp256r1; Aptos and NEAR: slip10
// ed25519, bip44 secp256k1), or an empty algorithm when nt does not bind dt.
algorithm derivation_algorithm(const network_type& nt, const derivation_type& dt);

// derivation_allows_format reports whether fmt is a script the purpose of dt
// defines on nt (Bitcoin: bip44 p2pkh, bip49 p2sh, bip84 p2wpkh or bech32,
// bip86 p2tr or bech32m); true when nt does not bind dt to formats.
bool derivation_allows_format(const network_type& nt, const derivation_type& dt, const format& fmt);

// network_is_registered reports whether the network type is in the
// compatibility matrix at all.
bool network_is_registered(const network_type& nt);

}  // namespace detail
}  // namespace mhda
