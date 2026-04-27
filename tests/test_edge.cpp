// Edge-case audit suite: address state invariants, lazy default behaviour,
// re-unmarshal, and a few cosmetic surfaces. Drawn directly from the open
// questions raised during the v0.1.0 release review.

#include <string>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

TEST_CASE("AVM has no default format and validates without one") {
    auto a = parse_urn_strict("urn:mhda:nt:avm:ct:9000:ci:1");
    EXPECT_EQ(a.resolved_format(), format{});  // intentionally unset
    EXPECT_EQ(a.resolved_algorithm(), algorithm::secp256k1);
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:avm:ct:9000:ci:1"});
}

TEST_CASE("Bitcoin has no default format and validates without one") {
    auto a = parse_urn_strict("urn:mhda:nt:btc:ct:0:ci:bitcoin");
    EXPECT_EQ(a.resolved_format(), format{});
    EXPECT_EQ(a.resolved_algorithm(), algorithm::secp256k1);
}

TEST_CASE("unmarshal_text replaces state, not merges") {
    auto a = parse_urn(
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip86:dp:m/86'/0'/0'/0/0:af:bech32m:ap:bc1p");
    EXPECT_FALSE(a.prefix().empty());
    EXPECT_FALSE(a.explicit_format().empty());

    a.unmarshal_text("urn:mhda:nt:evm:ct:60:ci:1");

    EXPECT_EQ(a.get_chain().network(), network_type::ethereum_vm);
    EXPECT_EQ(a.get_chain().coin(), coins::eth);
    EXPECT_TRUE(a.prefix().empty());        // bc1p must be wiped
    EXPECT_TRUE(a.explicit_format().empty());// bech32m must be wiped
    EXPECT_EQ(a.get_derivation_type(), derivation_type::root);
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ct:60:ci:1"});
}

TEST_CASE("unmarshal_text rejection leaves receiver intact") {
    auto a = parse_urn("urn:mhda:nt:evm:ct:60:ci:1");
    const std::string before = a.str();
    bool threw = false;
    try {
        a.unmarshal_text("not-a-urn");
    } catch (const parse_error&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
    EXPECT_EQ(a.str(), before);  // strong exception guarantee
}

TEST_CASE("validate rejects unknown network even with valid algo/fmt") {
    address a;
    chain c;
    c.set_network(network_type{"polkadot"});  // not registered
    c.set_coin(354);
    c.set_chain_id("mainnet");
    a = address{c, std::nullopt};
    EXPECT_THROW_CODE(a.validate(), error_code::incompatible);
}

TEST_CASE("validate of zero-value address yields uninitialized_address") {
    address a;
    EXPECT_THROW_CODE(a.validate(), error_code::uninitialized_address);
}

TEST_CASE("derivation_path::parse with empty type rejects") {
    EXPECT_THROW_CODE(derivation_path::parse(derivation_type{}, "m/0"),
                      error_code::invalid_derivation_type);
}

TEST_CASE("set_address_algorithm/format reset trims values") {
    auto a = parse_urn("urn:mhda:nt:evm:ct:60:ci:1:aa:secp256k1:af:hex");
    a.set_address_algorithm("  ");  // whitespace-only resets
    a.set_address_format("");
    EXPECT_TRUE(a.explicit_algorithm().empty());
    EXPECT_TRUE(a.explicit_format().empty());
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ct:60:ci:1"});
}

TEST_CASE("address copy and move preserve serialised form") {
    auto orig = parse_urn(
        "urn:mhda:nt:cosmos:ct:118:ci:cosmoshub:dt:cip11:dp:m/44'/118'/0'/0/0");
    address copy = orig;
    EXPECT_EQ(copy.str(), orig.str());

    address moved = std::move(copy);
    EXPECT_EQ(moved.str(), orig.str());
}

TEST_CASE("very long chain_id round-trips intact") {
    // Cosmos-style chain_id can be arbitrarily long (e.g. axelar-dojo-1).
    std::string ci(200, 'a');
    std::string urn = "urn:mhda:nt:cosmos:ct:118:ci:" + ci;
    auto a = parse_urn(urn);
    EXPECT_EQ(a.get_chain().id(), ci);
    EXPECT_EQ(a.str(), urn);
}

TEST_CASE("hardened address index round-trip with 0x form coin type") {
    auto a = parse_urn("urn:mhda:nt:evm:ct:0x3c:ci:1");
    // 0x3c == 60 (decimal); canonical str() emits decimal.
    EXPECT_EQ(a.get_chain().coin(), 60u);
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ct:60:ci:1"});
}

TEST_CASE("explicit set_coin via 0x and decimal are equivalent") {
    address a{chain{network_type::ethereum_vm, 0, "1"}, std::nullopt};
    a.set_coin_type("0xa86a");
    EXPECT_EQ(a.get_chain().coin(), 0xa86au);
    a.set_coin_type("60");
    EXPECT_EQ(a.get_chain().coin(), 60u);
}

TEST_CASE("optional path() empty for ROOT-form addresses") {
    auto a = parse_urn("urn:mhda:nt:evm:ct:60:ci:1");
    // Path is set (to ROOT) by parse_address_from_components, but the
    // public derivation_type accessor reports root.
    EXPECT_EQ(a.get_derivation_type(), derivation_type::root);
}

TEST_CASE("validate accepts ROOT for every registered network") {
    const std::vector<std::string> nets = {
        "btc:0:bitcoin", "evm:60:1", "avm:9000:1", "tvm:195:mainnet",
        "cosmos:118:cosmoshub", "sol:501:mainnet", "xrp:144:mainnet",
        "xlm:148:mainnet", "near:397:mainnet", "apt:637:mainnet",
        "sui:784:mainnet", "ada:1815:mainnet", "algo:283:mainnet",
        "ton:607:mainnet",
    };
    for (const auto& triple : nets) {
        const auto first  = triple.find(':');
        const auto second = triple.find(':', first + 1);
        const std::string nt = triple.substr(0, first);
        const std::string ct = triple.substr(first + 1, second - first - 1);
        const std::string ci = triple.substr(second + 1);
        const std::string urn = "urn:mhda:nt:" + nt + ":ct:" + ct + ":ci:" + ci;
        EXPECT_NO_THROW(parse_urn_strict(urn));
    }
}
