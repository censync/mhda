#include "mhda/algorithm.hpp"

#include <unordered_set>

#include "detail.hpp"

namespace mhda {

namespace {

const std::unordered_set<std::string>& algo_index() {
    // Allocated once and never destroyed: a consumer's global destructor may
    // still parse after static destruction has begun.
    static const auto* index = new std::unordered_set<std::string>{
        "secp256k1", "ed25519", "sr25519",
        "secp256r1", "secp384r1", "secp521r1", "prime256v1",
    };
    return *index;
}

}  // namespace

bool algorithm::is_valid() const noexcept {
    return algo_index().find(value_) != algo_index().end();
}

std::optional<algorithm> algorithm_from_string(std::string_view s) {
    auto key = detail::normalize(s);
    if (algo_index().find(key) == algo_index().end()) return std::nullopt;
    return algorithm{std::move(key)};
}

}  // namespace mhda
