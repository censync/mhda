#pragma once

#include <string>
#include <string_view>

#include "mhda/address.hpp"

namespace mhda {

// parse_urn is the lenient parsing entry point. It performs structural
// validation only: the (network, algorithm, format, derivation) combination is
// not checked against the compatibility matrix. RFC 8141 case-insensitive
// "urn:mhda:" prefix and rq/f-components are accepted; surrounding whitespace
// is trimmed.
//
// Throws parse_error on any failure. Use the corresponding code() to
// discriminate failure modes.
address parse_urn(std::string_view src);

// parse_urn_strict is parse_urn followed by address::validate. Rejects URNs
// whose (networkType, algorithm, format, derivation) combination is not in the
// known-good compatibility matrix.
address parse_urn_strict(std::string_view src);

// parse_nss parses an MHDA namespace-specific string (the substring after
// "urn:mhda:"). The "nt" component is required.
address parse_nss(std::string_view nss);

}  // namespace mhda
