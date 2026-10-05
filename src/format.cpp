#include "mhda/format.hpp"

#include <unordered_set>

#include "detail.hpp"

namespace mhda {

namespace {

const std::unordered_set<std::string>& format_index() {
    // Allocated once and never destroyed: a consumer's global destructor may
    // still parse after static destruction has begun.
    static const auto* index = new std::unordered_set<std::string>{
        "hex", "p2pkh", "p2sh", "p2wpkh", "p2wsh", "p2tr",
        "bech32", "bech32m", "base58", "base32", "strkey",
        "base64url", "ss58",
    };
    return *index;
}

}  // namespace

bool format::is_valid() const noexcept {
    return format_index().find(value_) != format_index().end();
}

std::optional<format> format_from_string(std::string_view s) {
    auto key = detail::normalize(s);
    if (format_index().find(key) == format_index().end()) return std::nullopt;
    return format{std::move(key)};
}

}  // namespace mhda
