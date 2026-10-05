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
    derivation_type next = derivation_type::root;
    if (!lowered.empty()) {
        next = derivation_type{lowered};
        if (!next.is_valid()) {
            throw parse_error(error_code::invalid_derivation_type,
                              detail::quote(lowered));
        }
    }
    if (!path_) path_.emplace();
    path_->set_type(next);  // a new type drops the old path
}

void address::set_derivation(std::string_view dt, std::string_view dp) {
    address scratch;
    scratch.set_derivation_type(dt);
    scratch.set_derivation_path(dp);
    path_ = std::move(scratch.path_);
}

void address::check_path_set() const {
    if (path_ && !path_->type().empty() && path_->type() != derivation_type::root &&
        path_->levels().empty()) {
        throw parse_error(error_code::invalid_derivation_path,
                          std::string{"derivation type \""} + path_->type().str() +
                              "\" is set without a path");
    }
}

void address::set_derivation_path(std::string_view dp) {
    if (!path_) path_.emplace();
    auto trimmed = detail::trim(dp);
    if (path_->type() == derivation_type::root || path_->type().empty()) {
        // A root address has no path. Dropping one instead would let a URN
        // with a dp but no dt (which parses as root) name the root key rather
        // than the path it spells out. Mirrors go-mhda.
        if (!trimmed.empty()) {
            throw parse_error(error_code::invalid_derivation_path,
                              std::string{"root derivation must have empty path, got "} + detail::quote(trimmed));
        }
        return;
    }
    auto lowered = detail::to_lower(trimmed);
    path_->parse_path(lowered);
}

void address::set_coin_type(std::string_view ct) {
    auto trimmed = detail::trim(ct);
    if (trimmed.empty()) {
        chain_.clear_coin();
        return;
    }
    std::uint32_t v = 0;
    if (!detail::parse_uint32(trimmed, v)) {
        throw parse_error(error_code::invalid_coin_type,
                          detail::quote(trimmed));
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
                          detail::quote(lowered));
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
                          detail::quote(lowered));
    }
    format_ = std::move(next);
}

void address::set_address_prefix(std::string_view ap) {
    auto trimmed = detail::trim(ap);
    detail::validate_free_form_value(detail::comp_address_prefix, trimmed);
    prefix_ = std::string{trimmed};
}

void address::set_address_suffix(std::string_view as) {
    auto trimmed = detail::trim(as);
    detail::validate_free_form_value(detail::comp_address_suffix, trimmed);
    suffix_ = std::string{trimmed};
}

void address::set_wallet_type(std::string_view wt) {
    auto trimmed = detail::trim(wt);
    detail::validate_free_form_value(detail::comp_wallet_type, trimmed);
    wallet_type_ = std::string{trimmed};
}

void address::set_wallet_id(std::string_view wi) {
    auto trimmed = detail::trim(wi);
    detail::validate_free_form_value(detail::comp_wallet_id, trimmed);
    wallet_id_ = std::string{trimmed};
}

// nss returns the URN namespace-specific string in canonical form. The
// emission order is the chain identity (nt/ci) first — so the chain key is a
// strict prefix of the NSS — then the optional coin-type metadata (ct), the
// derivation domain (dt/dp), address-format metadata (aa/af/ap/as), and the
// wallet domain (wt/wi) last. Optional components are emitted only when
// explicitly set, preserving the round-trip with short input forms.
std::string address::nss() const {
    std::string out;
    out.reserve(64);

    // Chain identity — always present.
    out += "nt:";
    out += chain_.network().str();
    out += ":ci:";
    out += chain_.id();

    // Coin-type metadata — emitted only when explicitly set.
    if (chain_.coin()) {
        out += ":ct:";
        append_uint32(out, *chain_.coin());
    }

    // Derivation domain — present when not ROOT and a non-empty type is set.
    // A type set without a path yet is emitted without dp, so the URN fails
    // to parse rather than name another key.
    if (path_ && !path_->type().empty() && path_->type() != derivation_type::root) {
        out += ":dt:";
        out += path_->type().str();
        const std::string p = path_->str();
        if (!p.empty()) {
            out += ":dp:";
            out += p;
        }
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

    // Wallet domain — emitted only when explicitly set.
    if (!wallet_type_.empty()) {
        out += ":wt:";
        out += wallet_type_;
    }
    if (!wallet_id_.empty()) {
        out += ":wi:";
        out += wallet_id_;
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
    check_path_set();
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
    check_path_set();
    if (!detail::network_is_registered(nt)) {
        throw parse_error(error_code::incompatible,
                          std::string{"unknown network type "} + detail::quote(nt.str()));
    }

    auto algo = resolved_algorithm();
    if (algo.empty()) {
        throw parse_error(error_code::incompatible,
                          std::string{"no algorithm resolved for network "} + detail::quote(nt.str()));
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
    if (!path_ || path_->type().empty() || path_->type() == derivation_type::root) return;
    const derivation_type& dt = path_->type();
    if (!detail::network_allows_derivation(nt, dt)) {
        throw parse_error(error_code::incompatible,
                          std::string{"derivation \""} + dt.str() +
                          "\" not allowed for network \"" + nt.str() + "\"");
    }
    const algorithm want = detail::derivation_algorithm(nt, dt);
    if (!want.empty() && want != algo) {
        throw parse_error(error_code::incompatible,
                          std::string{"derivation \""} + dt.str() + "\" derives \"" + want.str() +
                          "\" keys on network \"" + nt.str() + "\", not \"" + algo.str() + "\"");
    }
    if (!fmt.empty() && !detail::derivation_allows_format(nt, dt, fmt)) {
        throw parse_error(error_code::incompatible,
                          std::string{"format \""} + fmt.str() +
                          "\" does not match the purpose of derivation \"" + dt.str() +
                          "\" on network \"" + nt.str() + "\"");
    }
    // SLIP-10 derives ed25519 keys through hardened levels only; CIP-1852
    // (BIP32-Ed25519) is the one ed25519 scheme with soft derivation.
    if (algo == algorithm::ed25519 && dt != derivation_type::cip1852) {
        const auto& lvls = path_->levels();
        for (std::size_t i = 0; i < lvls.size(); ++i) {
            if (!lvls[i].is_hardened) {
                throw parse_error(error_code::incompatible,
                                  "ed25519 derives hardened levels only, level " +
                                      std::to_string(i) + " of \"" + path_->str() +
                                      "\" is not hardened");
            }
        }
    }
    // On Cosmos, bip44 with coin 118' is the cip11 path under another name;
    // strict mode keeps the one spelling.
    if (nt == network_type::cosmos && dt == derivation_type::bip44 &&
        path_->coin() == coins::atom) {
        throw parse_error(error_code::incompatible,
                          "\"bip44\" with coin 118' is the cip11 path, use dt:cip11");
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
