#include "mhda/format.hpp"

#include <unordered_set>

#include "detail.hpp"

namespace mhda {

const format format::hex       {"hex"};
const format format::p2pkh     {"p2pkh"};
const format format::p2sh      {"p2sh"};
const format format::p2wpkh    {"p2wpkh"};
const format format::p2wsh     {"p2wsh"};
const format format::p2tr      {"p2tr"};
const format format::bech32    {"bech32"};
const format format::bech32m   {"bech32m"};
const format format::base58    {"base58"};
const format format::base32    {"base32"};
const format format::strkey    {"strkey"};
const format format::base64url {"base64url"};
const format format::ss58      {"ss58"};

namespace {

const std::unordered_set<std::string>& format_index() {
    static const std::unordered_set<std::string> index = {
        "hex", "p2pkh", "p2sh", "p2wpkh", "p2wsh", "p2tr",
        "bech32", "bech32m", "base58", "base32", "strkey",
        "base64url", "ss58",
    };
    return index;
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
