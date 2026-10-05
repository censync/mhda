# Changelog

All notable changes to this project will be documented here. The format
follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the
project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.0] — 2026-10-05

URNs and values accepted by 1.1 are refused now: see Changed. Code that
builds addresses gets stricter constructors and setters. Mirrors go-mhda
1.2.0.

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
- **Fixed path levels are spelled exactly**, as in go-mhda. The purpose,
  the fixed coin of `cip11` / `cip1852` / `zip32` and the `0`/`1` charge of
  `bip32` and the BIP-44 family are literals in the Go reference's grammar;
  the port compared their values only and so accepted a leading zero that
  Go refuses (`m/044'/60'/0'/0/0`, `m/44'/60'/0'/00/0`,
  `m/1852'/01815'/0'/0/0`). These now throw
  `parse_error(invalid_derivation_path)`. Leading zeros in a variable level
  (`m/44'/060'/0'/0/0`) are still accepted, as in Go, and dropped in the
  canonical form.
- **Programmatic values are validated like parsed input.** The network
  type, the chain id and the derivation type are written verbatim into
  every URN, and nothing checked them when set in code: a chain id
  `1:dt:bip44:dp:m/44'/60'/0'/0/666` on a root address produced a URN that
  re-parsed as a bip44 path. The `chain` constructor, `set_network` and
  `set_chain_id` now require a registered network type and a non-empty
  chain id of printable ASCII without `:`, `?` or `#` (ASCII-trimmed) and
  throw `parse_error` (`invalid_network_type`, `missing_chain_id`,
  `invalid_value`) otherwise, leaving the chain unchanged. The
  `derivation_path` constructor, `from_levels` and `set_type` throw
  `parse_error(invalid_derivation_type)` on an unregistered type. Mirrors
  go-mhda.
- **A derivation path without a derivation type, or under `dt:root`, is
  refused** with `parse_error(invalid_derivation_path)`. Both used to be
  dropped silently: `urn:mhda:nt:evm:ci:1:dp:m/44'/60'/0'/0/0` and
  `urn:mhda:nt:evm:ci:1:dt:root:dp:m/0` parsed as the root address
  `urn:mhda:nt:evm:ci:1`, naming the root key instead of the path they
  spell out. `address::set_derivation_path` on a root address accepts only
  an empty path.
- **The NSS is parsed strictly as `key:value` pairs.** An unknown key was
  skipped as a single token, so its value was read as the next key and a
  key with the wrong case vanished: `...:ci:1:DT:bip44:DP:m/44'/60'/0'/0/5`
  parsed as the root address `urn:mhda:nt:evm:ci:1`, and
  `...:ci:1:ext:wi:dt:bip44:...` read `dt` as the wallet id. An unknown
  component is now skipped together with its value; a key that differs from
  a known key only by case, a dangling token, a trailing `:` and an empty
  key throw `parse_error(invalid_nss)`. `chain::from_key` reports a
  dangling token as `invalid_chain_key`, as before.
- **`parse_nss`, `chain::from_nss` and `chain::from_key` refuse `?` and
  `#`** with `parse_error(invalid_nss)`. `parse_urn` strips the RFC 8141
  r/q/f components before parsing, but an NSS given on its own kept the
  byte in a value, so the URN emitted from it was truncated on the next
  parse (`nt:evm:ci:1:x#y:z` was accepted). The NSS fuzz test now checks
  that the emitted URN re-parses to itself.
- **A `slip10` path has at most 255 levels**; a deeper one throws
  `parse_error(invalid_derivation_path)`. BIP-32 serialises a key's depth
  in one byte, so no wallet can represent a deeper key.
- **Strict validation refuses curve, purpose and format combinations no
  wallet can derive.** `validate` checked the algorithm, the format and the
  derivation type each on its own, so it accepted SLIP-10 ed25519 paths
  with soft levels (`nt:solana:...:dp:m/44'/501'/0/0`; ed25519 has no soft
  derivation), `bip44` with ed25519 on XRPL, NEAR and Aptos, Sui `bip54`
  with ed25519 or secp256r1, a Bitcoin `bip84` path with `af:p2pkh`, and
  Cosmos `bip44` with coin `118'`, which is the `cip11` path again.
  `parse_urn_strict` / `validate` now throw `parse_error(incompatible)`
  for: an ed25519 path with an unhardened level (except `cip1852`, which
  is BIP32-Ed25519); a derivation type with another curve than its own on
  Sui (`slip10` ed25519, `bip54` secp256k1, `bip74` secp256r1), Aptos and
  NEAR (`slip10` ed25519, `bip44` secp256k1); an explicit Bitcoin format
  that does not match the purpose (`bip44` p2pkh, `bip49` p2sh, `bip84`
  p2wpkh or bech32, `bip86` p2tr or bech32m); and Cosmos `bip44` with coin
  `118'`. A Bitcoin URN without `af` stays valid; SPEC.md no longer claims
  strict mode requires it. Lenient parsing is unchanged. Mirrors go-mhda.
- **No whitespace inside the NSS.** Each value was trimmed, so
  `urn:mhda:nt:evm:ci: 1 :dt: bip44 :...` parsed like the URN without the
  spaces, against SPEC §6.1. ASCII whitespace is now trimmed only around the
  whole URN, around an NSS or chain key parsed on its own, and before a
  stripped r/q/f component (`ci:0 #frag` still parses); around a key or
  value it throws `parse_error(invalid_nss)`. Mirrors go-mhda.
- **NSS bytes follow RFC 3986.** Keys and values accepted any printable
  ASCII, so `"`, `<`, `>`, `\`, `^`, `` ` ``, `{`, `|`, `}`, `[`, `]` and a
  raw `%` passed and were emitted in URNs that RFC 8141 does not allow. A
  key or value is now made of letters, digits and `-._~!$&'()*+,;=@/`
  only (RFC 3986 pchar and `/`, without `:` and `%`); anything else throws
  `invalid_nss` when parsed and `invalid_value` from the setters and the
  `chain` constructor. `%` is refused because percent-encoding is not
  supported and `%41` would be a second spelling of `A`. Mirrors go-mhda.

### Fixed

- **`charge_type` is 32 bits wide (was `std::uint8_t`).** The CIP-11 charge
  and the CIP-1852 role accept any level index, but the parsed value was
  truncated to a byte: `m/1852'/1815'/0'/256/0` parsed as role 0, so it
  named the role-0 key, re-serialised as `m/1852'/1815'/0'/0/0`, and
  `levels()` returned the truncated value. The full value is now kept in
  `charge()`, `levels()` and `str()`. This changes the ABI of
  `derivation_path` (its constructor, `charge()` and its layout); rebuild
  dependants.
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
- **No stale or mixed derivation state.** `set_type` and
  `address::set_derivation_type` changed the type and kept the old path, so
  a bip44 address switched to zip32 serialised as
  `dt:zip32:dp:m/32'/133'/3'/9`, a valid URN naming a key nobody gave, and
  a SLIP-10 path parsed after it kept the BIP-44 coin, account and charge.
  `set_derivation_type` followed by a failing `set_derivation_path` left
  that mixed address behind, and a `bad_alloc` inside `parse_path` left
  `str()` and `levels()` disagreeing. A new type now clears the path;
  until one is set the URN carries `dt` without `dp` (and does not parse),
  and `validate` / `marshal_text` throw `invalid_derivation_path`.
  `parse_path` parses into a fresh path and replaces the old one only on
  success. The new `address::set_derivation(dt, dp)` sets both at once and
  changes nothing on error; the URN parser uses it. Mirrors go-mhda.
- **`levels()` and `str()` cannot disagree.** `from_levels` kept any
  levels it was given while `str()` printed the type's template: a BIP-44
  path built from `49'/60'/0/0/0` printed `m/44'/60'/0'/0/0` but returned
  purpose 49' and an unhardened account from `levels()`, which is what a
  wallet derives from. Too few levels (or none for SLIP-10) or an index of
  2^31 produced a URN that does not parse, and the five-argument
  constructor accepted a charge of 7 or an account of 2^31. Both now
  return exactly the path `parse()` gives back for their own `str()`, and
  throw `parse_error(invalid_derivation_path)` otherwise. Two boundary
  tests that pinned the old results now expect the refusal. Mirrors
  go-mhda.
- **Error messages escape the input they quote.** `what()` copied the
  caller's bytes verbatim, so a newline or an ANSI escape in a
  client-supplied value (`set_wallet_id("x\n[INFO] ok")`) forged or
  coloured lines in any log that recorded the exception. Quoted input is
  now escaped like Go's `%q` (`\"`, `\\`, `\n`, `\r`, `\t`, `\xNN` for
  other control and non-ASCII bytes), so `what()` is one line of printable
  ASCII.
- **The headers compile under `<windows.h>`.** Its `near` macro (empty, from
  `minwindef.h`) turned `coins::near` into a syntax error in any
  translation unit that included `<windows.h>` first. `coin_type.hpp` now
  sets the macro aside for its list and restores it, and the new alias
  `coins::near_protocol` (after `network_type::near_protocol`) stays usable
  while the macro is in effect.
- **Embedding mhda leaves the parent build alone.** Added with
  `add_subdirectory` or FetchContent, mhda forced `CMAKE_BUILD_TYPE=Release`
  into the cache of a parent that had none (so the parent's own code was
  built with `-O3 -DNDEBUG` and its asserts off), built its tests and
  examples into the parent and added its install rules to the parent's
  install. These now apply only to a top-level build; `MHDA_BUILD_TESTS`,
  `MHDA_BUILD_EXAMPLES` and the new `MHDA_INSTALL` default to it.
- **`BUILD_SHARED_LIBS=ON` produces a usable library.** The hidden-symbol
  preset, without export macros, left `libmhda.so` with no mhda symbols
  and every consumer failed to link. A shared mhda now exports its symbols
  (`WINDOWS_EXPORT_ALL_SYMBOLS` for a DLL); a static one keeps them hidden.
- **The installed package is complete.** `mhda::mhda` now requires
  `cxx_std_17` publicly, so a consumer asking for C++14 is raised to C++17
  instead of failing on `std::optional`, and the install include path
  follows `CMAKE_INSTALL_INCLUDEDIR`.

### Documentation

- SPEC.md states that an explicit `dt:root` is folded away (root is the
  default and has no path, so a root address has one canonical form and
  one hash); the Algorand and TON notes no longer call `dt:root` the
  canonical form. It also lists ZIP-32 as a known limitation: `zip32`
  parses, but no network registers it, so strict parsing refuses it. Both
  behaviours are unchanged and now pinned by tests.
- SPEC.md notes that an EVM chain id is opaque: `ci:1`, `ci:0x1` and
  `ci:01` are three chain keys for one chain, and a producer must keep to
  one spelling.

### Tests

- `tests/data/dp_conformance.txt`: a derivation-path conformance table
  shared verbatim with go-mhda (`testdata/dp_conformance.txt`) and run by
  both suites, standalone and inside a URN, so a path one implementation
  refuses, the other refuses too. The boundary test and the fuzz seed that
  pinned `4294967295` now sit at the new bound.
- `tests/data/urn_conformance.txt`: a URN-level table shared verbatim with
  go-mhda, run through `parse_urn` / `parse_nss` with the expected error
  code, for the parser changes above.
- `tests/test_static_init.cpp` uses the library from a global constructor
  and a global destructor, `tests/test_state.cpp` covers construction,
  re-parsing and type changes, and `tests/test_windows_macros.cpp` compiles
  the headers under the `<windows.h>` macros; hash digests are pinned at
  the SHA block and padding boundaries (55 to 1000 bytes).
- CI adds an ASan/UBSan job and a `-Werror` job with the README's warning
  set, a shared-library job, and a packaging job that builds the
  `find_package` and `add_subdirectory` consumers in `tests/cmake/`; it
  runs with read-only permissions. 160 test cases total.

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
