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

// network_is_registered reports whether the network type is in the
// compatibility matrix at all.
bool network_is_registered(const network_type& nt);

}  // namespace detail
}  // namespace mhda
