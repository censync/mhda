#include <string>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

TEST_CASE("network_type_from_string is case-insensitive") {
    auto a = network_type_from_string("xrp");
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(*a, network_type::xrp_ledger);

    auto b = network_type_from_string("  XRP  ");
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(*b, network_type::xrp_ledger);

    auto c = network_type_from_string("xxx");
    EXPECT_FALSE(c.has_value());
}

TEST_CASE("algorithm helpers") {
    EXPECT_TRUE(algorithm::secp256k1.is_valid());
    EXPECT_TRUE(algorithm::ed25519.is_valid());
    EXPECT_FALSE(algorithm{"rsa"}.is_valid());
    EXPECT_EQ(algorithm::secp256k1.str(), std::string{"secp256k1"});

    auto a = algorithm_from_string("  ED25519  ");
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(*a, algorithm::ed25519);
    EXPECT_FALSE(algorithm_from_string("nope").has_value());
}

TEST_CASE("format helpers") {
    EXPECT_TRUE(format::p2tr.is_valid());
    EXPECT_TRUE(format::bech32m.is_valid());
    EXPECT_FALSE(format{"zzz"}.is_valid());
    EXPECT_EQ(format::bech32m.str(), std::string{"bech32m"});

    auto f = format_from_string("  P2TR  ");
    EXPECT_TRUE(f.has_value());
    EXPECT_EQ(*f, format::p2tr);
}

TEST_CASE("derivation_type helpers") {
    EXPECT_TRUE(derivation_type::bip44.is_valid());
    EXPECT_TRUE(derivation_type::slip10.is_valid());
    EXPECT_TRUE(derivation_type::root.is_valid());
    EXPECT_FALSE(derivation_type{"zip999"}.is_valid());
    EXPECT_EQ(derivation_type::bip86.str(), std::string{"bip86"});

    auto dt = derivation_type_from_string("  CIP1852  ");
    EXPECT_EQ(dt, derivation_type::cip1852);

    EXPECT_THROW_CODE(derivation_type_from_string("nope"),
                      error_code::invalid_derivation_type);
}

TEST_CASE("set_derivation_path numeric overflow surfaces correct sentinel") {
    EXPECT_THROW_CODE(parse_urn(
        "urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/0'/0'/0/99999999999999999999"),
        error_code::invalid_derivation_path);
}

TEST_CASE("SLIP-10 programmatic construction round-trips through full URN") {
    auto dp = derivation_path::from_levels(derivation_type::slip10, {
        {44, true}, {501, true}, {0, true}, {0, true},
    });
    chain   c{network_type::solana, coins::sol, "mainnet"};
    address a{c, std::optional<derivation_path>{std::move(dp)}};
    const std::string want = "urn:mhda:nt:sol:ct:501:ci:mainnet:dt:slip10:dp:m/44'/501'/0'/0'";
    EXPECT_EQ(a.str(), want);

    auto back = parse_urn_strict(want);
    EXPECT_EQ(back.str(), want);
}

TEST_CASE("Cardano CIP-1852 role mapping") {
    struct row { std::string urn; charge_type want_role; };
    std::vector<row> cases = {
        {"urn:mhda:nt:evm:ct:1815:ci:mainnet:dt:cip1852:dp:m/1852'/1815'/0'/0/0", 0},
        {"urn:mhda:nt:evm:ct:1815:ci:mainnet:dt:cip1852:dp:m/1852'/1815'/0'/1/0", 1},
        {"urn:mhda:nt:evm:ct:1815:ci:mainnet:dt:cip1852:dp:m/1852'/1815'/0'/2/0", 2},
        {"urn:mhda:nt:evm:ct:1815:ci:mainnet:dt:cip1852:dp:m/1852'/1815'/0'/3/0", 3},
        {"urn:mhda:nt:evm:ct:1815:ci:mainnet:dt:cip1852:dp:m/1852'/1815'/3'/2/7'", 2},
    };
    for (const auto& c : cases) {
        auto addr = parse_urn(c.urn);
        EXPECT_EQ(addr.str(), c.urn);
        EXPECT_EQ(addr.path()->charge(), c.want_role);
    }
}
