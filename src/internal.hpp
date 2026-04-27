#pragma once

#include <string>
#include <unordered_map>

#include "mhda/chain.hpp"

namespace mhda {
namespace internal {

// build_chain_from_components is a private helper exposed across translation
// units so the URN parser can build a chain from an already-parsed component
// map without re-running the NSS tokeniser.
chain build_chain_from_components(const std::unordered_map<std::string, std::string>& m);

}  // namespace internal
}  // namespace mhda
