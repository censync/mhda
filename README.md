# mhda

C++17 implementation of MHDA — MultiChain Hierarchical Deterministic Address.
A faithful port of [censync/go-mhda](https://github.com/censync/go-mhda)
(which remains the normative reference); the URN format and validation rules
are kept bit-identical so a URN produced by either implementation parses and
round-trips through the other.

[![ci](https://github.com/censync/mhda/actions/workflows/ci.yml/badge.svg)](https://github.com/censync/mhda/actions)

MHDA is a URN-based descriptor for blockchain HD addresses, with
[RFC 8141](https://datatracker.ietf.org/doc/rfc8141/) compatibility.

A single string captures everything needed to identify a derived address:
network, derivation scheme, path, signature curve, encoding format and any
prefix/suffix conventions.

```
urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip86:dp:m/86'/0'/0'/0/0:af:bech32m:ap:bc1p
```

Supported networks: Bitcoin, EVM, Avalanche, Tron, Cosmos, Solana, XRP,
Stellar, NEAR, Aptos, Sui, Cardano, Algorand, TON.

## Status

- Version: **1.0.0**
- Standard: **C++17**, no external runtime dependencies
- Tests: **71** unit + fuzz-equivalent stress cases (≈11 000 randomised
  iterations), passing under `-fsanitize=address,undefined,leak`
- Compilers verified: GCC 11.4 (Ubuntu 22.04), Clang 14 (when libstdc++ is
  available); the CI matrix runs Linux + macOS, Release + Debug
- Warning policy: clean under `-Wall -Wextra -Wpedantic -Wshadow -Wconversion
  -Wsign-conversion -Werror`
- API surface frozen at **0.1.0**; binary stability is not yet guaranteed
  across pre-1.0 minor versions

## Building

CMake 3.14+ and a C++17 compiler are required.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The library target is `mhda::mhda`; public headers live under `include/mhda/`.
Both options below are ON by default and can be disabled with
`-DMHDA_BUILD_TESTS=OFF` / `-DMHDA_BUILD_EXAMPLES=OFF`.

### Installing

```sh
cmake --install build --prefix /usr/local
```

This installs `libmhda.a`, the `include/mhda/` headers and a
`mhda::mhda` CMake export so downstream projects can:

```cmake
find_package(mhda REQUIRED)
target_link_libraries(my_app PRIVATE mhda::mhda)
```

### Embedding via FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(mhda
    GIT_REPOSITORY https://github.com/censync/mhda.git
    GIT_TAG        v1.0.0
)
FetchContent_MakeAvailable(mhda)
target_link_libraries(my_app PRIVATE mhda::mhda)
```

## Quick start

```cpp
#include <iostream>
#include "mhda/mhda.hpp"

int main() {
    using namespace mhda;

    // Lenient parsing: structural validation only.
    auto addr = parse_urn("urn:mhda:nt:evm:ct:60:ci:1");
    std::cout << addr.get_chain().network().str() << "\n";  // evm
    std::cout << addr.resolved_algorithm().str() << "\n";   // secp256k1
    std::cout << addr.resolved_format().str() << "\n";      // hex

    // Strict parsing also checks the (network, algorithm, format, derivation)
    // combination is in the known-good compatibility matrix.
    try {
        parse_urn_strict("urn:mhda:nt:evm:ct:60:ci:1:aa:ed25519");
    } catch (const parse_error& e) {
        if (e.code() == error_code::incompatible) {
            std::cout << "evm + ed25519 rejected, as expected\n";
        }
    }

    // Type-agnostic level-by-level path inspection.
    auto bip = parse_urn("urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0");
    for (const auto& lvl : bip.path()->levels()) {
        std::cout << "  " << lvl.index << (lvl.is_hardened ? "'" : "") << "\n";
    }

    // Hashing for content-addressing or deduplication.
    std::cout << bip.hash256() << "\n";   // SHA-256 hex

    // The chain-domain triple (nt, ct, ci) is itself a parseable key.
    auto key    = bip.get_chain().key();   // "nt:evm:ct:60:ci:1"
    auto parsed = chain::from_key(key);
    (void)parsed;
}
```

A runnable version is in [`examples/basic.cpp`](./examples/basic.cpp).

## Examples

| Network        | Example URN                                                                                          |
|----------------|------------------------------------------------------------------------------------------------------|
| Ethereum       | `urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0`                                            |
| Bitcoin (BIP86)| `urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip86:dp:m/86'/0'/0'/0/0:af:bech32m:ap:bc1p`                     |
| Solana         | `urn:mhda:nt:sol:ct:501:ci:mainnet:dt:slip10:dp:m/44'/501'/0'/0'`                                    |
| Stellar        | `urn:mhda:nt:xlm:ct:148:ci:mainnet:dt:slip10:dp:m/44'/148'/0'`                                       |
| Sui (ed25519)  | `urn:mhda:nt:sui:ct:784:ci:mainnet:dt:slip10:dp:m/44'/784'/0'/0'/0'`                                 |
| Cardano        | `urn:mhda:nt:ada:ct:1815:ci:mainnet:dt:cip1852:dp:m/1852'/1815'/0'/0/0`                              |
| Algorand       | `urn:mhda:nt:algo:ct:283:ci:mainnet` (non-HD)                                                        |
| TON            | `urn:mhda:nt:ton:ct:607:ci:mainnet` (non-HD, friendly base64url default)                             |
| Cosmos         | `urn:mhda:nt:cosmos:ct:118:ci:cosmoshub:dt:cip11:dp:m/44'/118'/0'/0/0`                               |
| EVM short form | `urn:mhda:nt:evm:ct:60:ci:1` (defaults: bip44, secp256k1, hex)                                       |

## API mapping (Go → C++)

| Go                                | C++                                          |
|-----------------------------------|----------------------------------------------|
| `mhda.ParseURN`                   | `mhda::parse_urn`                            |
| `mhda.ParseURNStrict`             | `mhda::parse_urn_strict`                     |
| `mhda.ParseNSS`                   | `mhda::parse_nss`                            |
| `mhda.ChainFromKey` / `FromNSS`   | `mhda::chain::from_key` / `from_nss`         |
| `mhda.NewChain(...)`              | `mhda::chain{...}`                           |
| `mhda.ParseDerivationPath`        | `mhda::derivation_path::parse`               |
| `mhda.NewDerivationPathFromLevels`| `mhda::derivation_path::from_levels`         |
| `Address.String()` / `NSS()`      | `address::str` / `address::nss`              |
| `Address.MarshalText`             | `address::marshal_text`                      |
| `Address.UnmarshalText`           | `address::unmarshal_text`                    |
| `Address.Hash` / `Hash256`        | `address::hash` / `hash256`                  |
| `Address.NSSHash` / `NSSHash256`  | `address::nss_hash` / `nss_hash256`          |
| `Address.Validate`                | `address::validate`                          |
| `errors.Is(err, ErrXxx)`          | `parse_error::code() == error_code::xxx`     |

Sentinel constants:

| Go                            | C++                                            |
|-------------------------------|------------------------------------------------|
| `ErrInvalidURN`               | `error_code::invalid_urn`                      |
| `ErrInvalidNSS`               | `error_code::invalid_nss`                      |
| `ErrMissingNetworkType`       | `error_code::missing_network_type`             |
| `ErrInvalidNetworkType`       | `error_code::invalid_network_type`             |
| `ErrMissingCoinType`          | `error_code::missing_coin_type`                |
| `ErrInvalidCoinType`          | `error_code::invalid_coin_type`                |
| `ErrMissingChainID`           | `error_code::missing_chain_id`                 |
| `ErrInvalidDerivationType`    | `error_code::invalid_derivation_type`          |
| `ErrInvalidDerivationPath`    | `error_code::invalid_derivation_path`          |
| `ErrInvalidAlgorithm`         | `error_code::invalid_algorithm`                |
| `ErrInvalidFormat`            | `error_code::invalid_format`                   |
| `ErrIncompatible`             | `error_code::incompatible`                     |
| `ErrUninitializedAddress`     | `error_code::uninitialized_address`            |

## Concurrency

Mirrors the [SPEC §8](./SPEC.md#8-concurrency) contract.

- `address`, `chain`, `derivation_path` are mutable value types. Their
  `set_*` and `parse_path` methods modify the receiver in place; calling them
  concurrently on the same instance is a data race and undefined behaviour.
  Synchronise externally if mutation from multiple threads is required.
- Read-only operations (`str`, `nss`, all `hash*`, `marshal_text`, `validate`,
  parser entry points and getter methods) are safe to invoke concurrently
  provided the receiver is not being mutated at the same time.
- All package-level lookup tables (the network-compatibility matrix, network
  index, algorithm/format/derivation registries) are populated lazily inside
  `static const` function-locals (Magic Statics — guaranteed thread-safe by
  C++11 §6.7.4) and are read-only thereafter; concurrent reads are safe and
  introduce no locking.
- No `std::mutex`, `std::atomic`, recursive locks or condition variables are
  used anywhere in the library — deadlock by construction is impossible.

## Testing & validation

- 71 unit + fuzz-equivalent test cases.
- Fuzz harness runs ≈11 000 randomised mutations of the historical Go-fuzz
  seed corpus per execution (URN, NSS and derivation-path entry points).
  Contracts verified: no exception other than `parse_error`/`std::invalid_argument`,
  serialised form always carries the canonical prefix, and
  `parse(parse(s).str()).str() == parse(s).str()` for every successful parse.
- Reference SHA-1 / SHA-256 vectors compared against `sha1sum` / `sha256sum`.
- Suite is run under `-fsanitize=address,undefined,leak` with
  `halt_on_error=1` and `strict_string_checks=1`; no diagnostic was raised.

To reproduce locally:

```sh
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined,leak -fno-omit-frame-pointer -fno-sanitize-recover=all -g" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined,leak"
cmake --build build-asan --parallel
ASAN_OPTIONS=halt_on_error=1:detect_leaks=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ./build-asan/tests/mhda_tests
```

## Specification

The full specification — components, networks, derivation types, formats,
algorithms, validation rules, error sentinels, concurrency model and known
limitations — is in [SPEC.md](./SPEC.md).

## Upstream

This repository is a C++ port of <https://github.com/censync/go-mhda>. The
Go implementation is the normative reference: when the spec evolves it lands
there first and is mirrored here. Bug reports that affect both repos should
prefer go-mhda; reports specific to the C++ surface (CMake, compilers,
ABI, ergonomics) belong here.

## License

MIT — see [LICENSE](./LICENSE).
