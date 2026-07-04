#pragma once

#include <stdexcept>
#include <string>

namespace mhda {

// error_code mirrors the Go sentinel errors. Callers can discriminate failure
// modes by comparing parse_error::code() against these constants instead of
// pattern-matching on what() text.
enum class error_code {
    invalid_urn,
    invalid_nss,
    missing_network_type,
    invalid_network_type,
    invalid_coin_type,
    missing_chain_id,
    invalid_derivation_type,
    invalid_derivation_path,
    invalid_algorithm,
    invalid_format,
    incompatible,
    uninitialized_address,
    // invalid_value is raised by the free-form component setters (ap/as/wt/wi)
    // when a value contains characters that would corrupt the serialised NSS:
    // the ':' separator, the RFC 8141 r/q/f delimiters ('?', '#') or anything
    // outside printable ASCII (whitespace of any kind, control bytes).
    invalid_value,
    // coin_type_in_chain_key is raised by chain::from_key when the input
    // carries a "ct" component. Chain keys are the bare identity
    // "nt:<network>:ci:<id>"; a key with "ct" is the pre-1.1 format and must
    // be regenerated.
    coin_type_in_chain_key,
    // invalid_chain_key is raised by chain::from_key when the input is not
    // the canonical identity form "nt:<network>:ci:<id>": any other known
    // component, unknown tokens, reordering or non-canonical spelling.
    invalid_chain_key,
};

const char* error_message(error_code c) noexcept;

// parse_error is the single exception type thrown by ParseURN, ParseNSS,
// ParseDerivationPath, Validate and the chain-domain factories. The wrapped
// error_code identifies the failure category; what() includes a human-readable
// message that may quote the offending input but is not part of the API
// contract.
class parse_error : public std::runtime_error {
public:
    parse_error(error_code c, const std::string& detail);
    explicit parse_error(error_code c);

    error_code code() const noexcept { return code_; }

private:
    error_code code_;
};

}  // namespace mhda
