#pragma once

#include <ostream>

#include "mhda/algorithm.hpp"
#include "mhda/chain.hpp"
#include "mhda/derivation_path.hpp"
#include "mhda/derivation_type.hpp"
#include "mhda/format.hpp"
#include "mhda/network_type.hpp"

// ostream operators for the public mhda types so the EXPECT_EQ macro can
// stringify diagnostic output. Kept out of the public headers because
// streaming wrappers are a test-suite concern, not part of the API contract.

namespace mhda {

inline std::ostream& operator<<(std::ostream& os, const network_type& v) {
    return os << v.str();
}
inline std::ostream& operator<<(std::ostream& os, const algorithm& v) {
    return os << v.str();
}
inline std::ostream& operator<<(std::ostream& os, const format& v) {
    return os << v.str();
}
inline std::ostream& operator<<(std::ostream& os, const derivation_type& v) {
    return os << v.str();
}
inline std::ostream& operator<<(std::ostream& os, const address_index& v) {
    os << v.index;
    if (v.is_hardened) os << "'";
    return os;
}
inline std::ostream& operator<<(std::ostream& os, const chain& c) {
    return os << c.str();
}

}  // namespace mhda
