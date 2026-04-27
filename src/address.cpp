#include "mhda/address.hpp"

#include <cstdint>
#include <string>

#include "compatibility.hpp"
#include "detail.hpp"
#include "hash.hpp"
#include "mhda/error.hpp"
#include "mhda/parser.hpp"
#include "nss_parser.hpp"

namespace mhda {

namespace {

void append_uint32(std::string& out, std::uint32_t v) {
    if (v == 0) { out.push_back('0'); return; }
    char tmp[16];
    int n = 0;
    while (v) { tmp[n++] = char('0' + (v % 10)); v /= 10; }
    for (int i = n - 1; i >= 0; --i) out.push_back(tmp[i]);
}

}  // namespace

address::address(chain c,
                 std::optional<derivation_path> path,
                 std::string algorithm,
                 std::string format,
                 std::string prefix,
                 std::string suffix)
    : chain_(std::move(c)), path_(std::move(path)) {
    if (!algorithm.empty()) set_address_algorithm(algorithm);
    if (!format.empty())    set_address_format(format);
    if (!prefix.empty())    set_address_prefix(prefix);
    if (!suffix.empty())    set_address_suffix(suffix);
}

derivation_type address::get_derivation_type() const {
    if (!path_) return derivation_type::root;
    if (path_->type().empty()) return derivation_type::root;
    return path_->type();
}

algorithm address::resolved_algorithm() const {
    if (!algorithm_.empty()) return algorithm_;
    return detail::default_algorithm(chain_.network());
}

format address::resolved_format() const {
    if (!format_.empty()) return format_;
    return detail::default_format(chain_.network());
}

void address::set_derivation_type(std::string_view dt) {
    auto trimmed = detail::trim(dt);
    auto lowered = detail::to_lower(trimmed);
    if (!path_) path_.emplace();
    if (lowered.empty()) {
        path_->set_type(derivation_type::root);
        return;
    }
    derivation_type next{lowered};
    if (!next.is_valid()) {
        throw parse_error(error_code::invalid_derivation_type,
                          std::string{"\""} + lowered + "\"");
    }
    path_->set_type(next);
}

void address::set_derivation_path(std::string_view dp) {
    if (!path_) path_.emplace();
    if (path_->type() == derivation_type::root || path_->type().empty()) {
        return;  // root has no path; silent no-op matches Go semantics
    }
    auto trimmed = detail::trim(dp);
    auto lowered = detail::to_lower(trimmed);
    path_->parse_path(lowered);
}

void address::set_coin_type(std::string_view ct) {
    auto trimmed = detail::trim(ct);
    if (trimmed.empty()) {
        throw parse_error(error_code::missing_coin_type);
    }
    std::uint32_t v = 0;
    if (!detail::parse_uint32(trimmed, v)) {
        throw parse_error(error_code::invalid_coin_type,
                          std::string{"\""} + std::string{trimmed} + "\"");
    }
    chain_.set_coin(v);
}

void address::set_address_algorithm(std::string_view aa) {
    auto trimmed = detail::trim(aa);
    auto lowered = detail::to_lower(trimmed);
    if (lowered.empty()) {
        algorithm_ = algorithm{};
        return;
    }
    algorithm next{lowered};
    if (!next.is_valid()) {
        throw parse_error(error_code::invalid_algorithm,
                          std::string{"\""} + lowered + "\"");
    }
    algorithm_ = std::move(next);
}

void address::set_address_format(std::string_view af) {
    auto trimmed = detail::trim(af);
    auto lowered = detail::to_lower(trimmed);
    if (lowered.empty()) {
        format_ = format{};
        return;
    }
    format next{lowered};
    if (!next.is_valid()) {
        throw parse_error(error_code::invalid_format,
                          std::string{"\""} + lowered + "\"");
    }
    format_ = std::move(next);
}

void address::set_address_prefix(std::string_view ap) {
    prefix_ = std::string{detail::trim(ap)};
}

void address::set_address_suffix(std::string_view as) {
    suffix_ = std::string{detail::trim(as)};
}

std::string address::nss() const {
    std::string out;
    out.reserve(64);

    // Chain domain — always present.
    out += "nt:";
    out += chain_.network().str();
    out += ":ct:";
    append_uint32(out, chain_.coin());
    out += ":ci:";
    out += chain_.id();

    // Derivation domain — present when not ROOT and a non-empty type is set.
    if (path_ && !path_->type().empty() && path_->type() != derivation_type::root) {
        out += ":dt:";
        out += path_->type().str();
        out += ":dp:";
        out += path_->str();
    }

    // Address-format metadata — emitted only when explicitly set.
    if (!algorithm_.empty()) {
        out += ":aa:";
        out += algorithm_.str();
    }
    if (!format_.empty()) {
        out += ":af:";
        out += format_.str();
    }
    if (!prefix_.empty()) {
        out += ":ap:";
        out += prefix_;
    }
    if (!suffix_.empty()) {
        out += ":as:";
        out += suffix_;
    }
    return out;
}

std::string address::str() const {
    std::string out = "urn:mhda:";
    out += nss();
    return out;
}

std::string address::marshal_text() const {
    if (chain_.network().empty()) {
        throw parse_error(error_code::uninitialized_address);
    }
    return str();
}

void address::unmarshal_text(std::string_view data) {
    *this = parse_urn(data);
}

void address::validate() const {
    const auto& nt = chain_.network();
    if (nt.empty()) {
        throw parse_error(error_code::uninitialized_address);
    }
    if (!detail::network_is_registered(nt)) {
        throw parse_error(error_code::incompatible,
                          std::string{"unknown network type \""} + nt.str() + "\"");
    }

    auto algo = resolved_algorithm();
    if (algo.empty()) {
        throw parse_error(error_code::incompatible,
                          std::string{"no algorithm resolved for network \""} + nt.str() + "\"");
    }
    if (!detail::network_allows_algorithm(nt, algo)) {
        throw parse_error(error_code::incompatible,
                          std::string{"algorithm \""} + algo.str() +
                          "\" not allowed for network \"" + nt.str() + "\"");
    }

    auto fmt = resolved_format();
    if (!fmt.empty() && !detail::network_allows_format(nt, fmt)) {
        throw parse_error(error_code::incompatible,
                          std::string{"format \""} + fmt.str() +
                          "\" not allowed for network \"" + nt.str() + "\"");
    }

    // ROOT (no derivation path) is always permitted.
    if (path_ && !path_->type().empty() && path_->type() != derivation_type::root) {
        if (!detail::network_allows_derivation(nt, path_->type())) {
            throw parse_error(error_code::incompatible,
                              std::string{"derivation \""} + path_->type().str() +
                              "\" not allowed for network \"" + nt.str() + "\"");
        }
    }
}

std::string address::hash() const {
    return detail::sha1_hex(str());
}

std::string address::nss_hash() const {
    return detail::sha1_hex(nss());
}

std::string address::hash256() const {
    return detail::sha256_hex(str());
}

std::string address::nss_hash256() const {
    return detail::sha256_hex(nss());
}

}  // namespace mhda
