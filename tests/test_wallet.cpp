// Wallet-domain (wt/wi) and optional coin-type (ct) semantics, mirroring the
// wallet tests of the Go reference implementation.

#include <string>
#include <vector>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

// Both components together, each alone, and their canonical trailing position.
TEST_CASE("wallet domain round-trip") {
    const std::vector<std::string> urns = {
        "urn:mhda:nt:evm:ci:1:wt:web3:wi:5f2a8c31",
        "urn:mhda:nt:evm:ci:1:wt:metamask",
        "urn:mhda:nt:evm:ci:1:wi:c0a8f2d4-3b6e-4a51-9c7d-2f8e1a0b5c93",
        "urn:mhda:nt:ton:ci:mainnet:wt:tonconnect",
        "urn:mhda:nt:evm:ci:1:ct:60:dt:bip44:dp:m/44'/60'/0'/0/0:aa:secp256k1:af:hex:ap:0x:wt:web3:wi:5f2a8c31",
    };
    for (const auto& urn : urns) {
        auto addr = parse_urn(urn);
        EXPECT_EQ(addr.str(), urn);
    }
}

TEST_CASE("wallet domain accessors") {
    auto addr = parse_urn("urn:mhda:nt:evm:ci:1:wt:web3:wi:5f2a8c31");
    EXPECT_EQ(addr.wallet_type(), std::string{"web3"});
    EXPECT_EQ(addr.wallet_id(), std::string{"5f2a8c31"});

    // Unset wallet domain reads as empty.
    auto bare = parse_urn("urn:mhda:nt:evm:ci:1");
    EXPECT_TRUE(bare.wallet_type().empty());
    EXPECT_TRUE(bare.wallet_id().empty());
}

// Empty-string set_x resets the field, matching the semantics of the other
// optional components.
TEST_CASE("wallet domain setters reset") {
    auto addr = parse_urn("urn:mhda:nt:evm:ci:1");
    addr.set_wallet_type("web3");
    addr.set_wallet_id("5f2a8c31");
    EXPECT_EQ(addr.str(), std::string{"urn:mhda:nt:evm:ci:1:wt:web3:wi:5f2a8c31"});
    addr.set_wallet_type("");
    addr.set_wallet_id("");
    EXPECT_EQ(addr.str(), std::string{"urn:mhda:nt:evm:ci:1"});
}

// Parsers accept any component order on input; the wallet domain re-serializes
// in the canonical trailing position.
TEST_CASE("wallet domain order independence") {
    const std::string canonical = "urn:mhda:nt:evm:ci:1:ct:60:wt:web3:wi:5f2a8c31";
    const std::vector<std::string> inputs = {
        "urn:mhda:wt:web3:wi:5f2a8c31:nt:evm:ci:1:ct:60",
        "urn:mhda:nt:evm:wt:web3:ci:1:wi:5f2a8c31:ct:60",
        "urn:mhda:nt:evm:ct:60:ci:1:wt:web3:wi:5f2a8c31",  // pre-1.1 ct position
    };
    for (const auto& in : inputs) {
        auto addr = parse_urn(in);
        EXPECT_EQ(addr.str(), canonical);
    }
}

// Empty values for wt/wi are malformed, consistent with every other component.
TEST_CASE("wallet domain empty values rejected") {
    const std::vector<std::string> bad = {
        "urn:mhda:nt:evm:ci:1:wt:",
        "urn:mhda:nt:evm:ci:1:wt::wi:x",
        "urn:mhda:nt:evm:ci:1:wi:",
    };
    for (const auto& urn : bad) {
        bool threw = false;
        try { parse_urn(urn); } catch (const parse_error&) { threw = true; }
        EXPECT_TRUE(threw);
    }
}

// The wallet domain is orthogonal metadata; strict validation must pass with
// it present and keep rejecting incompatible triples regardless of it.
TEST_CASE("wallet domain is validation-orthogonal") {
    EXPECT_NO_THROW(parse_urn_strict(
        "urn:mhda:nt:evm:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0:wt:web3:wi:5f2a8c31"));
    EXPECT_THROW_CODE(parse_urn_strict("urn:mhda:nt:evm:ci:1:aa:ed25519:wt:web3"),
                      error_code::incompatible);
}

// Pins the 1.1 semantics of ct: absent stays absent, present round-trips in
// the canonical position after ci, and the value normalizes to decimal.
TEST_CASE("coin type is optional metadata") {
    // Absent: no ct in output, coin() empty.
    auto addr = parse_urn("urn:mhda:nt:evm:ci:1");
    EXPECT_FALSE(addr.get_chain().coin().has_value());
    EXPECT_EQ(addr.str(), std::string{"urn:mhda:nt:evm:ci:1"});

    // Present: captured, emitted after ci, decimal-normalized (0x3c -> 60).
    addr = parse_urn("urn:mhda:nt:evm:ci:1:ct:0x3c");
    EXPECT_TRUE(addr.get_chain().coin().has_value());
    if (addr.get_chain().coin()) EXPECT_EQ(*addr.get_chain().coin(), 60u);
    EXPECT_EQ(addr.str(), std::string{"urn:mhda:nt:evm:ci:1:ct:60"});

    // set_coin_type with an empty string clears the metadata.
    addr.set_coin_type("");
    EXPECT_FALSE(addr.get_chain().coin().has_value());
    EXPECT_EQ(addr.str(), std::string{"urn:mhda:nt:evm:ci:1"});
}
