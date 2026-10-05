#include "mhda/derivation_type.hpp"

#include <unordered_set>

#include "detail.hpp"
#include "mhda/error.hpp"

namespace mhda {

namespace {

const std::unordered_set<std::string>& dt_index() {
    // Allocated once and never destroyed: a consumer's global destructor may
    // still parse after static destruction has begun.
    static const auto* index = new std::unordered_set<std::string>{
        "root", "bip32", "bip44", "bip49",
        "bip54", "bip74", "bip84", "bip86",
        "slip10", "cip1852", "cip11", "zip32",
    };
    return *index;
}

}  // namespace

bool derivation_type::is_valid() const noexcept {
    return dt_index().find(value_) != dt_index().end();
}

derivation_type derivation_type_from_string(std::string_view s) {
    auto key = detail::normalize(s);
    if (dt_index().find(key) == dt_index().end()) {
        throw parse_error(error_code::invalid_derivation_type, detail::quote(s));
    }
    return derivation_type{std::move(key)};
}

}  // namespace mhda
