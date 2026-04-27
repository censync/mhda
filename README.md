# mhda

C++17 port of [go-mhda](https://github.com/censync/go-mhda).

MultiChain Hierarchical Deterministic Address (MHDA) is a URN-based descriptor
for blockchain HD addresses, with [RFC 8141](https://datatracker.ietf.org/doc/rfc8141/)
compatibility.

A single string captures everything needed to identify a derived address:
network, derivation scheme, path, signature curve, encoding format and any
prefix/suffix conventions.

```
urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip86:dp:m/86'/0'/0'/0/0:af:bech32m:ap:bc1p
```

Supported networks: Bitcoin, EVM, Avalanche, Tron, Cosmos, Solana, XRP,
Stellar, NEAR, Aptos, Sui, Cardano, Algorand, TON.

## Building

The library is plain C++17 with no external runtime dependencies. CMake 3.14+
is required.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

The library target is `mhda::mhda`. Public headers live under `include/mhda/`.

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
    auto key = bip.get_chain().key();   // "nt:evm:ct:60:ci:1"
    auto parsed = chain::from_key(key);
    (void)parsed;
}
```

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

## Specification

The full specification — components, networks, derivation types, formats,
algorithms, validation rules, error sentinels, concurrency model and known
limitations — is in [SPEC.md](./SPEC.md).

## License

See [LICENSE](./LICENSE).
