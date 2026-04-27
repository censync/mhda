#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace mhda {
namespace detail {

// sha1_hex returns the SHA-1 hash of data as 40 lowercase hex characters.
std::string sha1_hex(std::string_view data);

// sha256_hex returns the SHA-256 hash of data as 64 lowercase hex characters.
std::string sha256_hex(std::string_view data);

}  // namespace detail
}  // namespace mhda
