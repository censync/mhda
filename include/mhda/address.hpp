#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "mhda/algorithm.hpp"
#include "mhda/chain.hpp"
#include "mhda/derivation_path.hpp"
#include "mhda/derivation_type.hpp"
#include "mhda/format.hpp"

namespace mhda {

// address is the full MHDA address descriptor: a chain triple plus an optional
// derivation path plus optional address-format metadata.
//
// All Set* methods modify the receiver in place and are NOT safe for concurrent
// use; callers must synchronise externally if mutating from multiple threads.
// Read-only operations (str, nss, hash*, accessors) are safe to invoke
// concurrently provided the receiver is not being mutated.
class address {
public:
    address() = default;
    address(chain c, std::optional<derivation_path> path)
        : chain_(std::move(c)), path_(std::move(path)) {}
    address(chain c,
            std::optional<derivation_path> path,
            std::string algorithm,
            std::string format,
            std::string prefix = "",
            std::string suffix = "");

    const chain& get_chain() const noexcept { return chain_; }
    chain&       get_chain() noexcept       { return chain_; }

    // derivation type of the path, or ROOT if there is no path.
    derivation_type get_derivation_type() const;

    const std::optional<derivation_path>& path() const noexcept { return path_; }
    std::optional<derivation_path>&       path() noexcept       { return path_; }

    // resolved_algorithm returns the explicit algorithm if set, else the
    // network-type default from the compatibility matrix.
    algorithm resolved_algorithm() const;

    // resolved_format returns the explicit format if set, else the network-type
    // default. Networks with multiple legitimate formats (e.g. Bitcoin) return
    // an empty format when nothing was set explicitly.
    format resolved_format() const;

    const algorithm&   explicit_algorithm() const noexcept { return algorithm_; }
    const format&      explicit_format() const noexcept    { return format_; }
    const std::string& prefix() const noexcept             { return prefix_; }
    const std::string& suffix() const noexcept             { return suffix_; }

    // set_derivation_type sets the path's derivation type; an empty string
    // resolves to ROOT. Allocates a fresh derivation_path if none was attached.
    void set_derivation_type(std::string_view dt);

    // set_derivation_path validates and applies the textual path. If the path's
    // derivation type is ROOT this is a silent no-op. Throws parse_error.
    void set_derivation_path(std::string_view dp);

    // set_coin_type sets the chain's coin type from a decimal or "0x"-prefixed
    // hex string. Throws parse_error on invalid input.
    void set_coin_type(std::string_view ct);

    // set_address_algorithm / _format / _prefix / _suffix mirror the Go API:
    // an empty string resets the field; non-empty values are validated.
    void set_address_algorithm(std::string_view aa);
    void set_address_format(std::string_view af);
    void set_address_prefix(std::string_view ap);
    void set_address_suffix(std::string_view as);

    // str returns the canonical "urn:mhda:..." form of this address.
    std::string str() const;

    // nss returns the URN namespace-specific string in canonical form. Optional
    // components are emitted only when explicitly set, preserving the round-trip
    // with short input forms.
    std::string nss() const;

    // marshal_text returns the canonical URN form, throwing parse_error
    // (uninitialized_address) when the address has not been initialised.
    std::string marshal_text() const;

    // unmarshal_text replaces the contents of this address with the result of
    // parsing the given URN. Equivalent to address::parse_urn.
    void unmarshal_text(std::string_view data);

    // Validate enforces the per-network compatibility matrix. Throws
    // parse_error with code error_code::incompatible if the resolved
    // (algorithm, format, derivation) triple is not in the network's allowed
    // set. ROOT is always permitted regardless of network.
    void validate() const;

    // Hash and nss_hash return SHA-1 digests (40 hex chars). Retained for
    // backward compatibility; SHA-1 is no longer collision-resistant. Prefer
    // hash256 / nss_hash256.
    std::string hash() const;
    std::string nss_hash() const;
    std::string hash256() const;
    std::string nss_hash256() const;

private:
    chain                          chain_;
    std::optional<derivation_path> path_;
    algorithm                      algorithm_;
    format                         format_;
    std::string                    prefix_;
    std::string                    suffix_;
};

}  // namespace mhda
