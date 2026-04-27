// Boundary / null / overflow / huge-input tests. Each case targets a
// specific failure mode discovered (or ruled out) during the v0.1.0 audit:
// numeric edges around 2^32, very long inputs, embedded NUL bytes, default-
// constructed value types, repeated keys, malformed paths, and degenerate
// hardening markers.

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

// ---------------------------------------------------------------------------
// Numeric boundaries
// ---------------------------------------------------------------------------

TEST_CASE("coin_type accepts uint32 max (decimal)") {
    auto a = parse_urn("urn:mhda:nt:evm:ct:4294967295:ci:1");
    EXPECT_EQ(a.get_chain().coin(), std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ct:4294967295:ci:1"});
}

TEST_CASE("coin_type accepts uint32 max (hex)") {
    auto a = parse_urn("urn:mhda:nt:evm:ct:0xFFFFFFFF:ci:1");
    EXPECT_EQ(a.get_chain().coin(), std::numeric_limits<std::uint32_t>::max());
    // Canonical str() emits decimal.
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ct:4294967295:ci:1"});
}

TEST_CASE("coin_type rejects uint32 max + 1 (decimal)") {
    EXPECT_THROW_CODE(parse_urn("urn:mhda:nt:evm:ct:4294967296:ci:1"),
                      error_code::invalid_coin_type);
}

TEST_CASE("coin_type rejects uint32 max + 1 (hex)") {
    EXPECT_THROW_CODE(parse_urn("urn:mhda:nt:evm:ct:0x100000000:ci:1"),
                      error_code::invalid_coin_type);
}

TEST_CASE("coin_type rejects bare 0x with no digits") {
    EXPECT_THROW_CODE(parse_urn("urn:mhda:nt:evm:ct:0x:ci:1"),
                      error_code::invalid_coin_type);
}

TEST_CASE("coin_type rejects negative sign") {
    EXPECT_THROW_CODE(parse_urn("urn:mhda:nt:evm:ct:-1:ci:1"),
                      error_code::invalid_coin_type);
}

TEST_CASE("coin_type rejects leading plus") {
    EXPECT_THROW_CODE(parse_urn("urn:mhda:nt:evm:ct:+1:ci:1"),
                      error_code::invalid_coin_type);
}

TEST_CASE("derivation_path leaf accepts uint32 max") {
    auto a = parse_urn(
        "urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/0/4294967295");
    EXPECT_EQ(a.path()->index().index, std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(a.str(),
        std::string{"urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/0/4294967295"});
}

TEST_CASE("derivation_path leaf rejects uint32 max + 1") {
    EXPECT_THROW_CODE(parse_urn(
        "urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/0/4294967296"),
        error_code::invalid_derivation_path);
}

TEST_CASE("BIP-44 charge field rejects values outside {0,1}") {
    EXPECT_THROW_CODE(parse_urn(
        "urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/2/0"),
        error_code::invalid_derivation_path);
}

TEST_CASE("CIP-11 charge accepts non-{0,1} (Cosmos charge_extra)") {
    auto a = parse_urn(
        "urn:mhda:nt:cosmos:ct:118:ci:cosmoshub:dt:cip11:dp:m/44'/118'/0'/9/0");
    EXPECT_EQ(a.path()->charge(), charge_type{9});
}

TEST_CASE("BIP-44 wrong purpose level rejected") {
    EXPECT_THROW_CODE(parse_urn(
        "urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/45'/60'/0'/0/0"),
        error_code::invalid_derivation_path);
}

// ---------------------------------------------------------------------------
// Path shape edge cases
// ---------------------------------------------------------------------------

TEST_CASE("derivation_path rejects bare 'm' (slip10)") {
    EXPECT_THROW_CODE(derivation_path::parse(derivation_type::slip10, "m"),
                      error_code::invalid_derivation_path);
}

TEST_CASE("derivation_path rejects leading slash without m") {
    EXPECT_THROW_CODE(derivation_path::parse(derivation_type::slip10, "/0"),
                      error_code::invalid_derivation_path);
}

TEST_CASE("derivation_path rejects double slash") {
    EXPECT_THROW_CODE(derivation_path::parse(derivation_type::slip10, "m//0"),
                      error_code::invalid_derivation_path);
}

TEST_CASE("derivation_path rejects trailing slash") {
    EXPECT_THROW_CODE(derivation_path::parse(derivation_type::slip10, "m/0/"),
                      error_code::invalid_derivation_path);
}

TEST_CASE("derivation_path rejects segment that is only a marker") {
    EXPECT_THROW_CODE(derivation_path::parse(derivation_type::slip10, "m/'"),
                      error_code::invalid_derivation_path);
    EXPECT_THROW_CODE(derivation_path::parse(derivation_type::slip10, "m/H"),
                      error_code::invalid_derivation_path);
}

TEST_CASE("derivation_path rejects mixed alphanumeric in segment") {
    EXPECT_THROW_CODE(derivation_path::parse(derivation_type::slip10, "m/12a"),
                      error_code::invalid_derivation_path);
}

TEST_CASE("derivation_path rejects double hardening marker") {
    EXPECT_THROW_CODE(derivation_path::parse(derivation_type::slip10, "m/0''"),
                      error_code::invalid_derivation_path);
}

TEST_CASE("derivation_path accepts SLIP-10 with single level") {
    auto dp = derivation_path::parse(derivation_type::slip10, "m/0");
    EXPECT_EQ(dp.str(), std::string{"m/0"});
    EXPECT_EQ(dp.levels().size(), 1u);
}

// ---------------------------------------------------------------------------
// Huge inputs
// ---------------------------------------------------------------------------

TEST_CASE("very long SLIP-10 path (256 levels) round-trips") {
    std::string path = "m";
    for (int i = 0; i < 256; ++i) path += "/0'";
    auto dp = derivation_path::parse(derivation_type::slip10, path);
    EXPECT_EQ(dp.levels().size(), 256u);
    EXPECT_EQ(dp.str(), path);
}

TEST_CASE("very long chain_id (4 KiB) round-trips") {
    std::string ci(4096, 'x');
    std::string urn = "urn:mhda:nt:evm:ct:60:ci:" + ci;
    auto a = parse_urn(urn);
    EXPECT_EQ(a.get_chain().id().size(), 4096u);
    EXPECT_EQ(a.str(), urn);
}

TEST_CASE("very long prefix and suffix round-trip") {
    std::string ap(512, 'p');
    std::string as(512, 's');
    std::string urn = "urn:mhda:nt:evm:ct:60:ci:1:ap:" + ap + ":as:" + as;
    auto a = parse_urn(urn);
    EXPECT_EQ(a.prefix().size(), 512u);
    EXPECT_EQ(a.suffix().size(), 512u);
    EXPECT_EQ(a.str(), urn);
}

TEST_CASE("megabyte URN is parsed without crashing") {
    // 1 MiB chain_id — guards against accidental quadratic copies in the
    // canonical-form rebuild path.
    std::string ci(1u << 20, 'A');
    std::string urn = "urn:mhda:nt:evm:ct:60:ci:" + ci;
    auto a = parse_urn(urn);
    EXPECT_EQ(a.get_chain().id().size(), std::size_t{1} << 20);
    // Round-tripping a megabyte string is wasteful in tests; just confirm
    // that str() produces something with the expected suffix length.
    auto out = a.str();
    EXPECT_TRUE(out.size() == urn.size());
}

// ---------------------------------------------------------------------------
// NUL / non-printable bytes
// ---------------------------------------------------------------------------

TEST_CASE("embedded NUL byte in chain_id is preserved (no truncation)") {
    std::string urn = "urn:mhda:nt:evm:ct:60:ci:abc";
    urn[urn.size() - 2] = '\0';  // "ab\0c" inside ci
    auto a = parse_urn(urn);
    EXPECT_EQ(a.get_chain().id().size(), 3u);
    EXPECT_EQ(a.get_chain().id()[1], '\0');
}

TEST_CASE("input with only the URN prefix is rejected") {
    EXPECT_THROW_CODE(parse_urn("urn:mhda:"), error_code::missing_network_type);
}

TEST_CASE("single-character noise inputs reject without crash") {
    const std::vector<std::string> noise = {
        "", " ", "\t", "\n", "x", ":", "::::", "urn:", "urn:mhda",
    };
    for (const auto& s : noise) {
        bool threw = false;
        try { parse_urn(s); } catch (const parse_error&) { threw = true; }
        EXPECT_TRUE(threw);
    }
}

// ---------------------------------------------------------------------------
// Component-level edge cases
// ---------------------------------------------------------------------------

TEST_CASE("duplicate components are rejected with invalid_nss") {
    EXPECT_THROW_CODE(parse_urn("urn:mhda:nt:evm:nt:btc:ct:60:ci:1"),
                      error_code::invalid_nss);
    EXPECT_THROW_CODE(parse_urn("urn:mhda:nt:evm:ct:60:ct:0:ci:1"),
                      error_code::invalid_nss);
}

TEST_CASE("unknown component keys are silently skipped") {
    // Forward-compat: a future "xx" component must not break parsing of the
    // surrounding components.
    auto a = parse_urn("urn:mhda:nt:evm:ct:60:ci:1:xx:future");
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ct:60:ci:1"});
}

TEST_CASE("string_view trim of fully-whitespace value rejects") {
    EXPECT_THROW_CODE(parse_urn("urn:mhda:nt:   :ct:60:ci:1"),
                      error_code::invalid_nss);
}

// ---------------------------------------------------------------------------
// Default-constructed value types
// ---------------------------------------------------------------------------

TEST_CASE("default-constructed network_type is empty and invalid") {
    network_type nt;
    EXPECT_TRUE(nt.empty());
    EXPECT_FALSE(nt.is_valid());
    EXPECT_EQ(nt.str(), std::string{});
}

TEST_CASE("default-constructed algorithm and format are empty") {
    EXPECT_TRUE(algorithm{}.empty());
    EXPECT_FALSE(algorithm{}.is_valid());
    EXPECT_TRUE(format{}.empty());
    EXPECT_FALSE(format{}.is_valid());
}

TEST_CASE("default-constructed derivation_type behaves as empty") {
    derivation_type dt;
    EXPECT_TRUE(dt.empty());
    EXPECT_FALSE(dt.is_valid());
    EXPECT_NE(dt, derivation_type::root);
}

TEST_CASE("default-constructed address has empty serialised form bits") {
    address a;
    // No invariants beyond \"validate throws\" — but str() must not crash.
    auto s = a.str();
    EXPECT_TRUE(s.find("urn:mhda:") == 0);
    EXPECT_THROW_CODE(a.validate(), error_code::uninitialized_address);
}

TEST_CASE("default-constructed derivation_path serialises empty") {
    derivation_path dp;
    EXPECT_EQ(dp.str(), std::string{});
    EXPECT_TRUE(dp.levels().empty());
}

TEST_CASE("default-constructed chain serialises with empty network slot") {
    chain c;
    EXPECT_EQ(c.str(), std::string{"nt::ct:0:ci:"});
}

// ---------------------------------------------------------------------------
// Mutation safety
// ---------------------------------------------------------------------------

TEST_CASE("set_address_algorithm rejects unknown without mutating") {
    auto a = parse_urn("urn:mhda:nt:evm:ct:60:ci:1:aa:secp256k1");
    bool threw = false;
    try { a.set_address_algorithm("rsa"); } catch (const parse_error&) { threw = true; }
    EXPECT_TRUE(threw);
    EXPECT_EQ(a.explicit_algorithm(), algorithm::secp256k1);
}

TEST_CASE("set_address_format rejects unknown without mutating") {
    auto a = parse_urn("urn:mhda:nt:evm:ct:60:ci:1:af:hex");
    bool threw = false;
    try { a.set_address_format("zzz"); } catch (const parse_error&) { threw = true; }
    EXPECT_TRUE(threw);
    EXPECT_EQ(a.explicit_format(), format::hex);
}

TEST_CASE("set_coin_type rejects bad input without mutating") {
    auto a = parse_urn("urn:mhda:nt:evm:ct:60:ci:1");
    bool threw = false;
    try { a.set_coin_type("not_a_number"); } catch (const parse_error&) { threw = true; }
    EXPECT_TRUE(threw);
    EXPECT_EQ(a.get_chain().coin(), 60u);
}

TEST_CASE("self-assignment via reference leaves address unchanged") {
    auto a = parse_urn("urn:mhda:nt:evm:ct:60:ci:1");
    const std::string before = a.str();
    address& ref = a;
    a = ref;  // routed through a reference to silence -Wself-assign-overloaded
    EXPECT_EQ(a.str(), before);
}

TEST_CASE("from_levels with zero levels for SLIP10 produces empty path") {
    auto dp = derivation_path::from_levels(derivation_type::slip10, {});
    EXPECT_EQ(dp.str(), std::string{"m"});
    EXPECT_FALSE(dp.has_index());
}

TEST_CASE("from_levels under-populated for BIP44 leaves shortcuts at zero") {
    auto dp = derivation_path::from_levels(derivation_type::bip44, {
        {44, true}, {60, true},  // only 2 levels, well below the 5 expected
    });
    EXPECT_EQ(dp.coin(), 0u);
    EXPECT_EQ(dp.account(), 0u);
    EXPECT_FALSE(dp.has_index());
}

TEST_CASE("hash and nss_hash do not crash on uninitialised address") {
    address a;
    // Both must be deterministic over the (empty) serialised form. The check
    // is mainly that no UB/crash occurs on the sanitiser-instrumented path.
    EXPECT_EQ(a.hash().size(), 40u);
    EXPECT_EQ(a.hash256().size(), 64u);
    EXPECT_EQ(a.nss_hash().size(), 40u);
    EXPECT_EQ(a.nss_hash256().size(), 64u);
}

// ---------------------------------------------------------------------------
// chain factory boundaries
// ---------------------------------------------------------------------------

TEST_CASE("chain::from_nss rejects empty input") {
    EXPECT_THROW_CODE(chain::from_nss(""), error_code::missing_network_type);
}

TEST_CASE("chain::from_nss rejects unknown network") {
    EXPECT_THROW_CODE(chain::from_nss("nt:xyz:ct:0:ci:any"),
                      error_code::invalid_network_type);
}

TEST_CASE("chain::from_key with extra components ignores them") {
    auto c = chain::from_key("nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0");
    EXPECT_EQ(c.network(), network_type::ethereum_vm);
    EXPECT_EQ(c.id(), std::string{"1"});
    EXPECT_EQ(c.str(), std::string{"nt:evm:ct:60:ci:1"});
}
