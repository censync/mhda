// Post-review hardening suite, mirroring hardening_test.go of the Go
// reference: duplicate-component rejection, canonical-only chain keys, ct
// spelling grammar, printable-ASCII value enforcement, free-form setter
// validation, and case-preservation pins. Plus the review-identified test
// gaps: coin-registry spot checks and the explicit ct:0 URN round-trip.

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

// Pins the duplicate-key rule for old and new components alike.
TEST_CASE("duplicate components rejected for ci/ct/wt/wi") {
    const std::vector<std::string> urns = {
        "urn:mhda:nt:evm:ci:1:ci:2",
        "urn:mhda:nt:evm:ci:1:ct:60:ct:61",
        "urn:mhda:nt:evm:ci:1:wt:web3:wt:metamask",
        "urn:mhda:nt:evm:ci:1:wi:a:wi:b",
    };
    for (const auto& urn : urns) {
        EXPECT_THROW_CODE(parse_urn(urn), error_code::invalid_nss);
    }
}

// A chain key must BE the canonical identity string — unknown tokens,
// reordering and surrounding junk are all rejected, not silently normalised.
TEST_CASE("chain::from_key is canonical-only") {
    const std::vector<std::string> keys = {
        "nt:evm:ci:1:zz:junk",  // unknown trailing token
        "nt:evm:ci:1:foo",      // dangling token
        "ci:1:nt:evm",          // reordered
        "nt:EVM:ci:1",          // non-canonical case in the enum value
    };
    for (const auto& k : keys) {
        EXPECT_THROW_CODE(chain::from_key(k), error_code::invalid_chain_key);
    }
    // Surrounding whitespace is tolerated (trimmed before the canonical
    // comparison) — a key embedded in config files commonly carries it.
    EXPECT_NO_THROW(chain::from_key("  nt:evm:ci:1  "));
    auto c = chain::from_key("  nt:evm:ci:1  ");
    EXPECT_EQ(c.key(), std::string{"nt:evm:ci:1"});
}

// The free-form components carry client-supplied strings; a value able to
// smuggle ':' (component injection), '?' / '#' (RFC 8141 truncation) or
// whitespace would break the round-trip guarantee — the setters must reject
// them loudly, and a rejected value must not partially mutate the address.
TEST_CASE("free-form setters reject NSS-corrupting values") {
    auto addr = parse_urn("urn:mhda:nt:evm:ci:1");
    const std::vector<std::pair<std::string, std::function<void(std::string_view)>>> setters = {
        {"set_wallet_type",    [&](std::string_view v) { addr.set_wallet_type(v); }},
        {"set_wallet_id",      [&](std::string_view v) { addr.set_wallet_id(v); }},
        {"set_address_prefix", [&](std::string_view v) { addr.set_address_prefix(v); }},
        {"set_address_suffix", [&](std::string_view v) { addr.set_address_suffix(v); }},
    };
    const std::vector<std::string> bad = {
        "x:dt:bip44", "ci:2", "abc#def", "x?y", "a b",
    };
    for (const auto& s : setters) {
        for (const auto& v : bad) {
            EXPECT_THROW_CODE(s.second(v), error_code::invalid_value);
        }
    }
    // The rejected values must not have partially mutated the address.
    EXPECT_EQ(addr.str(), std::string{"urn:mhda:nt:evm:ci:1"});
}

// Constructor params ride the same validation as parsed input; an invalid
// value throws (the C++ analogue of the Go constructor panic).
TEST_CASE("constructor throws on an NSS-corrupting prefix param") {
    EXPECT_THROW_CODE(
        (address{chain{network_type::ethereum_vm, "1"}, std::nullopt,
                 "", "", "x:evil", ""}),
        error_code::invalid_value);
}

// Pins the documented ct grammar: plain decimal and 0x/0X-prefixed hex only.
// Go integer-literal extras (0o/0b, underscores) must be rejected; a leading
// zero stays decimal ("060" == 60, "08" == 8), never octal.
TEST_CASE("coin-type spellings") {
    const std::vector<std::pair<std::string, coin_type>> valid = {
        {"urn:mhda:nt:evm:ci:1:ct:60",   60},
        {"urn:mhda:nt:evm:ci:1:ct:0x3c", 60},
        {"urn:mhda:nt:evm:ci:1:ct:0X3C", 60},
        {"urn:mhda:nt:evm:ci:1:ct:060",  60},  // leading zero is DECIMAL, not octal
        {"urn:mhda:nt:evm:ci:1:ct:08",   8},   // would be invalid as octal
    };
    for (const auto& v : valid) {
        auto addr = parse_urn(v.first);
        EXPECT_TRUE(addr.get_chain().coin().has_value());
        if (addr.get_chain().coin()) EXPECT_EQ(*addr.get_chain().coin(), v.second);
    }
    const std::vector<std::string> invalid = {
        "urn:mhda:nt:evm:ci:1:ct:6_0",
        "urn:mhda:nt:evm:ci:1:ct:0o74",
        "urn:mhda:nt:evm:ci:1:ct:0b111100",
        "urn:mhda:nt:evm:ci:1:ct:-1",
        "urn:mhda:nt:evm:ci:1:ct:0x",
    };
    for (const auto& urn : invalid) {
        EXPECT_THROW_CODE(parse_urn(urn), error_code::invalid_coin_type);
    }
}

// Values with embedded whitespace cannot appear in a conforming NSS and would
// serialise into a non-parseable form.
TEST_CASE("interior whitespace in values rejected") {
    const std::vector<std::string> urns = {
        "urn:mhda:nt:evm:ci:a b",
        "urn:mhda:nt:evm:ci:1:wt:a b",
        "urn:mhda:nt:evm:ci:1:ap:0x 1",
    };
    for (const auto& urn : urns) {
        EXPECT_THROW_CODE(parse_urn(urn), error_code::invalid_nss);
    }
}

// An NSS is printable ASCII by definition (RFC 8141). Unicode spaces must not
// be silently trimmed into a different chain identity, and non-ASCII value
// bytes must fail loudly.
TEST_CASE("non-ASCII values rejected") {
    const std::vector<std::string> urns = {
        "urn:mhda:nt:evm:ci:1\u3000",                    // ideographic space in ci
        "urn:mhda:nt:evm:ci:1\u00a0",                    // NBSP in ci
        "urn:mhda:nt:evm:ci:\u0442\u0435\u0441\u0442",  // Cyrillic value
        "urn:mhda:nt:evm:ci:1:wt:web\u00a03",            // NBSP inside wt
    };
    for (const auto& urn : urns) {
        EXPECT_THROW_CODE(parse_urn(urn), error_code::invalid_nss);
    }
    // Setters enforce the same rule for programmatic input.
    auto addr = parse_urn("urn:mhda:nt:evm:ci:1");
    EXPECT_THROW_CODE(addr.set_wallet_id("a\u00a0b"), error_code::invalid_value);
    // Keys with a trailing Unicode space are not the canonical string.
    EXPECT_THROW_CODE(chain::from_key("nt:evm:ci:1\u3000"), error_code::invalid_nss);
}

// Enum-valued components normalise to lowercase; free-form values
// (ci/ap/as/wt/wi) are case-preserving and round-trip verbatim.
TEST_CASE("free-form values preserve case") {
    const std::string in = "urn:mhda:nt:evm:ci:0xAbC:ap:0xPREFIX:wt:Web3:wi:ABCDEF";
    auto addr = parse_urn(in);
    EXPECT_EQ(addr.str(), in);
    EXPECT_TRUE(addr.str().find(":wt:Web3") != std::string::npos);
}

// Registry spot checks, incl. the ATOM fix (118; 168 belongs to Helleniccoin).
TEST_CASE("coin registry spot checks") {
    EXPECT_EQ(coins::atom, 118u);
    EXPECT_EQ(coins::btc, 0u);
    EXPECT_EQ(coins::eth, 60u);
    EXPECT_EQ(coins::trx, 195u);
    EXPECT_EQ(coins::sol, 501u);
    EXPECT_EQ(coins::ton, 607u);
    EXPECT_EQ(coins::sui, 784u);
    EXPECT_EQ(coins::mon, 268435779u);
}

// An explicit ct:0 (Bitcoin) is distinct from "not set" and must round-trip
// at the URN level.
TEST_CASE("explicit ct:0 round-trips at URN level") {
    const std::string in = "urn:mhda:nt:bitcoin:ci:bitcoin:ct:0";
    auto addr = parse_urn(in);
    EXPECT_TRUE(addr.get_chain().coin().has_value());
    if (addr.get_chain().coin()) EXPECT_EQ(*addr.get_chain().coin(), 0u);
    EXPECT_EQ(addr.str(), in);
}
