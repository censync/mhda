#include "mhda/algorithm.hpp"

#include <unordered_set>

#include "detail.hpp"

namespace mhda {

const algorithm algorithm::secp256k1  {"secp256k1"};
const algorithm algorithm::ed25519    {"ed25519"};
const algorithm algorithm::sr25519    {"sr25519"};
const algorithm algorithm::secp256r1  {"secp256r1"};
const algorithm algorithm::secp384r1  {"secp384r1"};
const algorithm algorithm::secp521r1  {"secp521r1"};
const algorithm algorithm::prime256v1 {"prime256v1"};

namespace {

const std::unordered_set<std::string>& algo_index() {
    static const std::unordered_set<std::string> index = {
        "secp256k1", "ed25519", "sr25519",
        "secp256r1", "secp384r1", "secp521r1", "prime256v1",
    };
    return index;
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
