# Changelog

All notable changes to this project will be documented here. The format
follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the
project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.0] — 2026-10-05

URNs accepted by 1.1 are refused now: a derivation path with a level index
of 2^31 or more no longer parses. Mirrors go-mhda 1.2.0.

### Changed

- **A derivation-path level index of 2^31 or more is refused** with
  `parse_error(invalid_derivation_path)`, at every level of every
  derivation type (`bip32`, `bip44`, `bip49`, `bip54`, `bip74`, `bip84`,
  `bip86`, `cip11`, `cip1852`, `zip32`, `slip10`), hardened or not, by
  `parse_urn`, `parse_urn_strict`, `parse_nss`, `derivation_path::parse`
  and `validate_derivation_path`. 1.1 accepted any value up to 2^32-1. A
  BIP-32 child number keeps the hardened flag in its top bit (`n'` is child
  number 2^31+n), so such an index aliased another key: a consumer
  computing `hardened ? 0x80000000 | index : index` derives the key of
  `m/44'/60'/0'/0/0` for `m/44'/60'/2147483648'/0/0`, and an unhardened
  `m/44'/60'/0'/0/2147483648` asks for a hardened child number through the
  public-key formula. No longer parse: any `dp` with a level of
  `2147483648` to `4294967295`, with or without a hardened marker, e.g.
  `urn:mhda:nt:evm:ci:1:dt:bip44:dp:m/44'/60'/2147483648'/0/0` and
  `urn:mhda:nt:evm:ci:1:dt:bip44:dp:m/44'/60'/0'/0/4294967295`. The largest
  index is `2147483647`. The `ct` component is not a path level and keeps
  its 32-bit range.

### Fixed

- **Static initialisation and destruction order.** The named constants
  (`network_type::bitcoin`, `algorithm::secp256k1`, `format::hex`,
  `derivation_type::root`, ...) were defined in the library's `.cpp` files
  and built at start-up in an unspecified order relative to the consumer's
  globals, and the lookup tables copied them on first use. A consumer
  global that used one (a built-in chain table, a URN validated up front)
  could run first: it saw empty values and left the tables wrong for the
  whole process (`parse_urn_strict` then refused every network as
  unknown). A global destructor that parsed at exit read destroyed tables
  and crashed. The constants are now inline variables defined in the
  public headers, so they are initialised before any global defined after
  the include, and the lookup tables are allocated once and never
  destroyed.
- **Fixed path levels are spelled exactly**, as in go-mhda. The purpose,
  the fixed coin of `cip11` / `cip1852` / `zip32` and the `0`/`1` charge of
  `bip32` and the BIP-44 family are literals in the Go reference's grammar;
  the port compared their values only and so accepted a leading zero that
  Go refuses (`m/044'/60'/0'/0/0`, `m/44'/60'/0'/00/0`,
  `m/1852'/01815'/0'/0/0`). These now throw
  `parse_error(invalid_derivation_path)`. Leading zeros in a variable level
  (`m/44'/060'/0'/0/0`) are still accepted, as in Go, and dropped in the
  canonical form.

- **`charge_type` is 32 bits wide (was `std::uint8_t`).** The CIP-11 charge
  and the CIP-1852 role accept any level index, but the parsed value was
  truncated to a byte: `m/1852'/1815'/0'/256/0` parsed as role 0, so it
  named the role-0 key, re-serialised as `m/1852'/1815'/0'/0/0`, and
  `levels()` returned the truncated value. The full value is now kept in
  `charge()`, `levels()` and `str()`. This changes the ABI of
  `derivation_path` (its constructor, `charge()` and its layout); rebuild
  dependants.

### Tests

- `tests/data/dp_conformance.txt`: a derivation-path conformance table
  shared verbatim with go-mhda (`testdata/dp_conformance.txt`) and run by
  both suites, standalone and inside a URN, so a path one implementation
  refuses, the other refuses too. The boundary test and the fuzz seed that
  pinned `4294967295` now sit at the new bound; 145 test cases total.

## [1.1.0] — 2026-07-04

URN grammar 1.1, mirroring the go-mhda reference. The chain identity is now
the `(nt, ci)` pair; the SLIP-44 coin type is optional metadata. Existing
pre-1.1 URNs still parse (any component order is accepted on input), but
pre-1.1 chain keys are rejected loudly and must be regenerated.

### Changed

- **Chain identity is `(nt, ci)`.** `chain::str()` / `chain::key()` return
  `nt:<network>:ci:<chain_id>` — the coin type never appears in the key —
  and `chain::operator==` compares network and chain id only. The constructor
  is now `chain(network_type, chain_id)`; the old three-argument form
  (with a coin type) is gone.
- **`ct` is optional metadata.** `chain::coin()` returns
  `std::optional<coin_type>` (`set_coin` / `clear_coin` manage it);
  `address::set_coin_type("")` clears it. Parsers accept a URN/NSS without
  `ct`; when present it must still be a valid uint32 (decimal or 0x-hex) and
  is re-emitted in decimal.
- **Canonical NSS order** is now `nt:ci[:ct][:dt:dp][:aa][:af][:ap][:as]
  [:wt][:wi]` — the chain key is a strict prefix of every NSS. Input order
  remains free.
- **`chain::from_key` is canonical-only.** A chain key must BE the canonical
  identity string `nt:<network>:ci:<chain_id>`: an input with `ct` throws the
  new `error_code::coin_type_in_chain_key` (pre-1.1 keys fail loudly instead
  of being silently reinterpreted); any other known non-identity component,
  unknown tokens, reordering and non-canonical spelling throw the new
  `error_code::invalid_chain_key`. Surrounding ASCII whitespace is trimmed
  and tolerated. `chain::from_nss` stays lenient and still extracts `nt`,
  `ci` and the optional `ct` from any NSS.
- **Strict `ct` grammar.** Coin-type values parse as plain decimal or
  `0x`/`0X`-prefixed hex only: `0o`/`0b` prefixes, digit-group underscores,
  signs and a bare `0x` are rejected, and a leading zero is plain decimal
  (`060` == 60, never octal).
- **Printable-ASCII values.** Every NSS value must consist of printable
  ASCII (0x21–0x7E) after ASCII trimming: control bytes, interior whitespace
  and non-ASCII bytes (incl. Unicode spaces) throw
  `parse_error(invalid_nss)` instead of being silently normalised.
- **Validated free-form setters.** `set_address_prefix` / `set_address_suffix`
  / `set_wallet_type` / `set_wallet_id` reject values containing `:`, `?`,
  `#` or anything outside printable ASCII with the new
  `error_code::invalid_value` (empty still resets). The
  `address(chain, path, aa, af, ap, as)` constructor routes its params
  through the same setters, so invalid constructor input throws too.
- **Network-type values renamed** to the commonly accepted network names
  (constant identifiers unchanged): `bitcoin` (was `btc`), `avalanche`
  (was `avm`), `tron` (was `tvm`), `solana` (was `sol`), `xrpl` (was `xrp`),
  `stellar` (was `xlm`), `aptos` (was `apt`), `cardano` (was `ada`),
  `algorand` (was `algo`). `evm`, `cosmos`, `near`, `sui`, `ton` are
  unchanged. There are no aliases: the old short names are invalid.
- `coins::atom` fixed to 118 (was 168, which SLIP-44 assigns to
  Helleniccoin); 118 also matches the coin level of CIP-11 paths.

### Added

- **Wallet domain** on `address`: free-form `wt` (wallet type, e.g. `web3`,
  `tonconnect`) and `wi` (wallet instance id) components, each independently
  optional, emitted last in the canonical NSS and orthogonal to strict
  validation. API: `wallet_type()` / `wallet_id()` accessors and
  `set_wallet_type` / `set_wallet_id` setters (empty string resets).
- Coin-type registry extended with 34 SLIP-44 entries (etc, bch, eos, icp,
  ckb, zil, luna, dot, ksm, kava, fil, cspr, egld, scrt, flow, vet, rune,
  ftm, one, xtz, hype, hbar, move, stx, bera, xch, strk, mina, wax, kas,
  osmo, sei, inj, mon); the list is ordered ascending by index.
- `error_code::invalid_value` — raised by the free-form component setters
  (ap/as/wt/wi) and the address constructor on NSS-corrupting values.

### Removed

- `error_code::missing_coin_type` — `ct` is never required anymore.

### Documentation / tests

- SPEC.md and README brought in lockstep with the Go reference (grammar 1.1,
  wallet domain, chain API, charset and value-validation rules, error table).
- Test corpus mirrors the Go fixtures: new wallet-domain suite, strict
  chain-key suite, optional-ct semantics, updated hash reference vectors for
  the new canonical form, and the post-review hardening suite (canonical-only
  chain keys, ct spellings, printable-ASCII enforcement, setter validation,
  case-preservation, coin-registry spot checks); 139 test cases total.

## [1.0.0] — 2026-04-27

Initial public release. C++17 port of the
[go-mhda](https://github.com/censync/go-mhda) reference implementation,
mirroring its parser, validator, derivation-path support and hash surface.
Shipped as tag v1.0.0; the in-tree version markers of that tree still read
0.1.0.

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
