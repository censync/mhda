#include <stdexcept>
#include <string>
#include <vector>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

TEST_CASE("ParseDerivationPath round-trip") {
    struct row { derivation_type dt; std::string in; std::string want; };
    std::vector<row> cases = {
        {derivation_type::bip44,   "m/44'/0'/0'/0/0",        "m/44'/0'/0'/0/0"},
        {derivation_type::bip44,   "m/44H/0h/0'/1/2'",       "m/44'/0'/0'/1/2'"},  // marker normalisation
        {derivation_type::slip10,  "m/44'/501'/0'/0'",       "m/44'/501'/0'/0'"},
        {derivation_type::zip32,   "m/32'/133'/0'",          "m/32'/133'/0'"},
        {derivation_type::root,    "",                       ""},
    };
    for (const auto& c : cases) {
        auto dp = derivation_path::parse(c.dt, c.in);
        EXPECT_EQ(dp.str(), c.want);
    }
}

TEST_CASE("ParseDerivationPath errors") {
    EXPECT_THROW_CODE(derivation_path::parse(derivation_type{"nope"}, "m/0"),
                      error_code::invalid_derivation_type);
    EXPECT_THROW_CODE(derivation_path::parse(derivation_type::bip44, "not_a_path"),
                      error_code::invalid_derivation_path);
    EXPECT_THROW_CODE(derivation_path::parse(derivation_type::root, "m/0"),
                      error_code::invalid_derivation_path);
    EXPECT_THROW_CODE(derivation_path::parse(derivation_type::bip44,
                                             "m/44'/0'/0'/0/99999999999999999999"),
                      error_code::invalid_derivation_path);
}

TEST_CASE("BIP44 levels view") {
    auto addr = parse_urn("urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/1'/0/2");
    std::vector<address_index> want = {
        {44, true}, {60, true}, {1, true}, {0, false}, {2, false},
    };
    EXPECT_EQ(addr.path()->levels().size(), want.size());
    for (std::size_t i = 0; i < want.size(); ++i) {
        EXPECT_EQ(addr.path()->levels()[i], want[i]);
    }
}

TEST_CASE("CIP-1852 levels exposed correctly") {
    auto addr = parse_urn("urn:mhda:nt:evm:ct:1815:ci:mainnet:dt:cip1852:dp:m/1852'/1815'/3'/2/7");
    std::vector<address_index> want = {
        {1852, true}, {1815, true}, {3, true}, {2, false}, {7, false},
    };
    const auto& got = addr.path()->levels();
    EXPECT_EQ(got.size(), want.size());
    for (std::size_t i = 0; i < want.size(); ++i) EXPECT_EQ(got[i], want[i]);
}

TEST_CASE("SLIP-10 mixed hardening round-trip") {
    const std::string urn = "urn:mhda:nt:evm:ct:0:ci:mainnet:dt:slip10:dp:m/44'/0'/0'/0/5";
    auto addr = parse_urn(urn);
    std::vector<address_index> want = {
        {44, true}, {0, true}, {0, true}, {0, false}, {5, false},
    };
    EXPECT_EQ(addr.path()->levels().size(), want.size());
    for (std::size_t i = 0; i < want.size(); ++i) {
        EXPECT_EQ(addr.path()->levels()[i], want[i]);
    }
    EXPECT_EQ(addr.str(), urn);
}

TEST_CASE("from_levels for SLIP10") {
    auto dp = derivation_path::from_levels(derivation_type::slip10, {
        {44, true}, {501, true}, {0, true}, {0, true},
    });
    EXPECT_EQ(dp.str(), std::string{"m/44'/501'/0'/0'"});
}

TEST_CASE("from_levels for BIP44 populates shortcuts") {
    auto dp = derivation_path::from_levels(derivation_type::bip44, {
        {44, true}, {60, true}, {3, true}, {1, false}, {7, false},
    });
    EXPECT_EQ(dp.str(), std::string{"m/44'/60'/3'/1/7"});
    EXPECT_EQ(dp.coin(), 60u);
    EXPECT_EQ(dp.account(), 3u);
    EXPECT_EQ(dp.charge(), charge_type{1});
    EXPECT_EQ(dp.index().index, 7u);
}

TEST_CASE("from_levels ZIP-32 variable length") {
    auto a = derivation_path::from_levels(derivation_type::zip32, {
        {32, true}, {133, true}, {5, true},
    });
    EXPECT_EQ(a.str(), std::string{"m/32'/133'/5'"});
    auto b = derivation_path::from_levels(derivation_type::zip32, {
        {32, true}, {133, true}, {5, true}, {9, false},
    });
    EXPECT_EQ(b.str(), std::string{"m/32'/133'/5'/9"});
}

TEST_CASE("BIP44 family round-trip") {
    const char* cases[] = {
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip49:dp:m/49'/0'/0'/0/0:af:p2sh",
        "urn:mhda:nt:evm:ct:784:ci:1:dt:bip54:dp:m/54'/784'/0'/0/0",
        "urn:mhda:nt:evm:ct:784:ci:1:dt:bip74:dp:m/74'/784'/0'/0/0",
    };
    for (auto* in : cases) {
        auto addr = parse_urn(in);
        EXPECT_EQ(addr.str(), std::string{in});
    }
}

TEST_CASE("CIP-11 emits coin 118 not 133") {
    const std::string in = "urn:mhda:nt:cosmos:ct:118:ci:cosmoshub:dt:cip11:dp:m/44'/118'/3'/0/7";
    auto addr = parse_urn(in);
    EXPECT_EQ(addr.str(), in);
}

TEST_CASE("BIP44 hardened leaf round-trip") {
    const std::string in = "urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/0'/0'/0/0'";
    auto addr = parse_urn(in);
    EXPECT_EQ(addr.str(), in);
    EXPECT_TRUE(addr.path()->is_hardened_address());
}

TEST_CASE("ZIP32 variable-length URN round-trip") {
    const char* urns[] = {
        "urn:mhda:nt:btc:ct:133:ci:zcash:dt:zip32:dp:m/32'/133'/0'",
        "urn:mhda:nt:btc:ct:133:ci:zcash:dt:zip32:dp:m/32'/133'/0'/0",
        "urn:mhda:nt:btc:ct:133:ci:zcash:dt:zip32:dp:m/32'/133'/0'/0'",
    };
    for (auto* in : urns) {
        auto addr = parse_urn(in);
        EXPECT_EQ(addr.str(), std::string{in});
    }
}

TEST_CASE("constructor rejects SLIP10") {
    bool threw = false;
    try {
        derivation_path dp{derivation_type::slip10, 0, 0, 0, address_index{}};
        (void)dp;
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

TEST_CASE("IsHardenedAddress predicate") {
    auto a = derivation_path::parse(derivation_type::bip44, "m/44'/0'/0'/0/0");
    EXPECT_FALSE(a.is_hardened_address());
    auto b = derivation_path::parse(derivation_type::bip44, "m/44'/0'/0'/0/0'");
    EXPECT_TRUE(b.is_hardened_address());
}
