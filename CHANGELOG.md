# Changelog

All notable changes to this project will be documented here. The format
follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the
project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] — 2026-04-27

Initial public release. C++17 port of the
[go-mhda](https://github.com/censync/go-mhda) reference implementation,
mirroring its parser, validator, derivation-path support and hash surface.

### Added

- Public API under `mhda::` matching the Go surface 1:1: `parse_urn`,
  `parse_urn_strict`, `parse_nss`, `chain::from_key`, `chain::from_nss`,
  `derivation_path::parse`, `derivation_path::from_levels`, `address` with
  `str` / `nss` / `marshal_text` / `unmarshal_text` / `validate` /
  `hash` / `nss_hash` / `hash256` / `nss_hash256`.
- Strongly typed wrappers `network_type`, `algorithm`, `format`,
  `derivation_type` with named constants for every registered value.
- Sentinel error catalogue exposed as `mhda::error_code`, raised through
  `mhda::parse_error : std::runtime_error` (preserves stable `code()` across
  any wrap-up of the human-readable `what()`).
- Per-network compatibility matrix covering BTC, EVM, AVM, TVM, Cosmos,
  Solana, XRP, Stellar, NEAR, Aptos, Sui (3 schemes), Cardano (CIP-1852),
  Algorand, TON.
- Derivation-path types: ROOT, BIP-32 / 44 / 49 / 54 / 74 / 84 / 86, SLIP-10
  (variable length), CIP-11, CIP-1852, ZIP-32 (3- and 4-level).
- RFC 8141 prefix case folding and rq/f-component stripping.
- Hardened-marker normalisation (`'`, `H`, `h` → canonical `'`).
- Hand-rolled SHA-1 / SHA-256 — zero runtime dependencies.
- CMake target `mhda::mhda` with install-rules, `mhdaTargets` export and
  GitHub Actions CI matrix (Linux + macOS, Release + Debug).
- 71 unit + fuzz-equivalent tests; ≈11 000 randomised parser iterations per
  run; clean under `-fsanitize=address,undefined,leak` and under
  `-Werror -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`.
