#include "mhda/derivation_type.hpp"

#include <unordered_set>

#include "detail.hpp"
#include "mhda/error.hpp"

namespace mhda {

const derivation_type derivation_type::root    {"root"};
const derivation_type derivation_type::bip32   {"bip32"};
const derivation_type derivation_type::bip44   {"bip44"};
const derivation_type derivation_type::bip49   {"bip49"};
const derivation_type derivation_type::bip54   {"bip54"};
const derivation_type derivation_type::bip74   {"bip74"};
const derivation_type derivation_type::bip84   {"bip84"};
const derivation_type derivation_type::bip86   {"bip86"};
const derivation_type derivation_type::slip10  {"slip10"};
const derivation_type derivation_type::cip1852 {"cip1852"};
const derivation_type derivation_type::cip11   {"cip11"};
const derivation_type derivation_type::zip32   {"zip32"};

namespace {

const std::unordered_set<std::string>& dt_index() {
    static const std::unordered_set<std::string> index = {
        "root", "bip32", "bip44", "bip49",
        "bip54", "bip74", "bip84", "bip86",
        "slip10", "cip1852", "cip11", "zip32",
    };
    return index;
}

}  // namespace

bool derivation_type::is_valid() const noexcept {
    return dt_index().find(value_) != dt_index().end();
}

derivation_type derivation_type_from_string(std::string_view s) {
    auto key = detail::normalize(s);
    if (dt_index().find(key) == dt_index().end()) {
        throw parse_error(error_code::invalid_derivation_type, std::string{"\""} + std::string{s} + "\"");
    }
    return derivation_type{std::move(key)};
}

}  // namespace mhda
