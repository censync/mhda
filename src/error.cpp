#include "mhda/error.hpp"

namespace mhda {

const char* error_message(error_code c) noexcept {
    switch (c) {
        case error_code::invalid_urn:             return "mhda: not a valid mhda urn";
        case error_code::invalid_nss:             return "mhda: cannot parse nss";
        case error_code::missing_network_type:    return "mhda: \"nt\" is required";
        case error_code::invalid_network_type:    return "mhda: invalid \"nt\"";
        case error_code::missing_coin_type:       return "mhda: \"ct\" is required";
        case error_code::invalid_coin_type:       return "mhda: invalid \"ct\"";
        case error_code::missing_chain_id:        return "mhda: \"ci\" is required";
        case error_code::invalid_derivation_type: return "mhda: invalid \"dt\"";
        case error_code::invalid_derivation_path: return "mhda: invalid \"dp\"";
        case error_code::invalid_algorithm:       return "mhda: invalid \"aa\"";
        case error_code::invalid_format:          return "mhda: invalid \"af\"";
        case error_code::incompatible:            return "mhda: incompatible network/algorithm/format";
        case error_code::uninitialized_address:   return "mhda: address is not initialized";
    }
    return "mhda: unknown error";
}

namespace {

std::string compose(error_code c, const std::string& detail) {
    std::string out = error_message(c);
    if (!detail.empty()) {
        out += ": ";
        out += detail;
    }
    return out;
}

}  // namespace

parse_error::parse_error(error_code c, const std::string& detail)
    : std::runtime_error(compose(c, detail)), code_(c) {}

parse_error::parse_error(error_code c)
    : std::runtime_error(error_message(c)), code_(c) {}

}  // namespace mhda
