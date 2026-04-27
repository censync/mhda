#include <string>
#include <vector>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

TEST_CASE("strict mode accepts valid combos") {
    const std::vector<std::string> ok = {
        "urn:mhda:nt:evm:ct:60:ci:1",
        "urn:mhda:nt:evm:ct:60:ci:1:aa:secp256k1:af:hex",
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip44:dp:m/44'/0'/0'/0/0:af:p2pkh",
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip84:dp:m/84'/0'/0'/0/0:af:bech32",
        "urn:mhda:nt:sol:ct:501:ci:mainnet",
        "urn:mhda:nt:sol:ct:501:ci:mainnet:aa:ed25519:af:base58",
        "urn:mhda:nt:cosmos:ct:118:ci:cosmoshub:dt:cip11:dp:m/44'/118'/0'/0/0",
    };
    for (const auto& urn : ok) EXPECT_NO_THROW(parse_urn_strict(urn));
}

TEST_CASE("strict mode rejects invalid combos") {
    const std::vector<std::string> bad = {
        "urn:mhda:nt:evm:ct:60:ci:1:aa:ed25519",
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:aa:ed25519",
        "urn:mhda:nt:evm:ct:60:ci:1:af:bech32",
        "urn:mhda:nt:sol:ct:501:ci:mainnet:aa:secp256k1",
        "urn:mhda:nt:cosmos:ct:118:ci:cosmoshub:af:hex",
    };
    for (const auto& urn : bad) {
        EXPECT_THROW_CODE(parse_urn_strict(urn), error_code::incompatible);
    }
}

TEST_CASE("non-strict parser stays permissive") {
    const std::string bad_for_strict = "urn:mhda:nt:evm:ct:60:ci:1:aa:ed25519";
    EXPECT_NO_THROW(parse_urn(bad_for_strict));
    EXPECT_THROW_CODE(parse_urn_strict(bad_for_strict), error_code::incompatible);
}

TEST_CASE("derivation compatibility leg of strict mode") {
    struct row { std::string urn; bool ok; };
    std::vector<row> cases = {
        {"urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0", true},
        {"urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip86:dp:m/86'/0'/0'/0/0:af:bech32m", true},
        {"urn:mhda:nt:cosmos:ct:118:ci:cosmoshub:dt:cip11:dp:m/44'/118'/0'/0/0", true},
        {"urn:mhda:nt:sol:ct:501:ci:mainnet:dt:slip10:dp:m/44'/501'/0'/0'", true},
        {"urn:mhda:nt:ada:ct:1815:ci:mainnet:dt:cip1852:dp:m/1852'/1815'/0'/0/0", true},
        {"urn:mhda:nt:algo:ct:283:ci:mainnet", true},
        {"urn:mhda:nt:ton:ct:607:ci:mainnet", true},
        {"urn:mhda:nt:sui:ct:784:ci:mainnet:dt:bip54:dp:m/54'/784'/0'/0/0:aa:secp256k1", true},
        {"urn:mhda:nt:sui:ct:784:ci:mainnet:dt:bip74:dp:m/74'/784'/0'/0/0:aa:secp256r1", true},
        // disallowed combos
        {"urn:mhda:nt:evm:ct:1815:ci:1:dt:cip1852:dp:m/1852'/1815'/0'/0/0", false},
        {"urn:mhda:nt:sol:ct:501:ci:mainnet:dt:bip44:dp:m/44'/501'/0'/0/0", false},
        {"urn:mhda:nt:ada:ct:1815:ci:mainnet:dt:bip44:dp:m/44'/1815'/0'/0/0", false},
        {"urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:cip11:dp:m/44'/118'/0'/0/0", false},
        {"urn:mhda:nt:apt:ct:637:ci:mainnet:dt:bip54:dp:m/54'/637'/0'/0/0:aa:secp256k1", false},
        {"urn:mhda:nt:xlm:ct:148:ci:mainnet:dt:bip44:dp:m/44'/148'/0'/0/0", false},
    };
    for (const auto& c : cases) {
        if (c.ok) {
            EXPECT_NO_THROW(parse_urn_strict(c.urn));
        } else {
            EXPECT_THROW_CODE(parse_urn_strict(c.urn), error_code::incompatible);
        }
    }
}

TEST_CASE("Bitcoin all script formats") {
    const std::vector<std::string> urns = {
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip44:dp:m/44'/0'/0'/0/0:af:p2pkh",
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip44:dp:m/44'/0'/0'/0/0:af:p2sh",
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip84:dp:m/84'/0'/0'/0/0:af:p2wpkh",
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip84:dp:m/84'/0'/0'/0/0:af:p2wsh",
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip84:dp:m/84'/0'/0'/0/0:af:bech32",
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip86:dp:m/86'/0'/0'/0/0:af:p2tr",
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip86:dp:m/86'/0'/0'/0/0:af:bech32m",
    };
    for (const auto& urn : urns) {
        auto addr = parse_urn_strict(urn);
        EXPECT_EQ(addr.str(), urn);
    }
}

TEST_CASE("Algorithm and format defaults") {
    auto evm = parse_urn("urn:mhda:nt:evm:ct:60:ci:1");
    EXPECT_EQ(evm.resolved_algorithm(), algorithm::secp256k1);
    EXPECT_EQ(evm.resolved_format(), format::hex);

    auto sol = parse_urn("urn:mhda:nt:sol:ct:501:ci:mainnet");
    EXPECT_EQ(sol.resolved_algorithm(), algorithm::ed25519);

    auto ton = parse_urn("urn:mhda:nt:ton:ct:607:ci:mainnet");
    EXPECT_EQ(ton.resolved_algorithm(), algorithm::ed25519);
    EXPECT_EQ(ton.resolved_format(), format::base64url);

    auto algo = parse_urn("urn:mhda:nt:algo:ct:283:ci:mainnet");
    EXPECT_EQ(algo.resolved_format(), format::base32);
}

TEST_CASE("defaults do not leak into NSS") {
    auto evm = parse_urn("urn:mhda:nt:evm:ct:60:ci:1");
    EXPECT_EQ(evm.str(), std::string{"urn:mhda:nt:evm:ct:60:ci:1"});
}

TEST_CASE("README examples round-trip strictly") {
    const std::vector<std::string> examples = {
        "urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0",
        "urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0:aa:secp256k1:af:hex:ap:0x",
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip44:dp:m/44'/0'/0'/0/0:af:p2pkh:ap:1",
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip49:dp:m/49'/0'/0'/0/0:af:p2sh:ap:3",
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip84:dp:m/84'/0'/0'/0/0:af:bech32:ap:bc1q",
        "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip86:dp:m/86'/0'/0'/0/0:af:bech32m:ap:bc1p",
        "urn:mhda:nt:evm:ct:60:ci:0xa86a:dt:bip44:dp:m/44'/60'/0'/0/0",
        "urn:mhda:nt:avm:ct:9000:ci:1:dt:bip44:dp:m/44'/9000'/0'/0/0:af:bech32:ap:X-avax",
        "urn:mhda:nt:sol:ct:501:ci:mainnet:dt:slip10:dp:m/44'/501'/0'/0'",
        "urn:mhda:nt:xrp:ct:144:ci:mainnet:dt:bip44:dp:m/44'/144'/0'/0/0",
        "urn:mhda:nt:xrp:ct:144:ci:mainnet:dt:bip44:dp:m/44'/144'/0'/0/0:aa:ed25519",
        "urn:mhda:nt:xlm:ct:148:ci:mainnet:dt:slip10:dp:m/44'/148'/0'",
        "urn:mhda:nt:near:ct:397:ci:mainnet:dt:slip10:dp:m/44'/397'/0'",
        "urn:mhda:nt:near:ct:397:ci:mainnet:dt:bip44:dp:m/44'/397'/0'/0/0:aa:secp256k1",
        "urn:mhda:nt:apt:ct:637:ci:mainnet:dt:slip10:dp:m/44'/637'/0'/0'/0'",
        "urn:mhda:nt:apt:ct:637:ci:mainnet:dt:bip44:dp:m/44'/637'/0'/0/0:aa:secp256k1",
        "urn:mhda:nt:sui:ct:784:ci:mainnet:dt:slip10:dp:m/44'/784'/0'/0'/0'",
        "urn:mhda:nt:sui:ct:784:ci:mainnet:dt:bip54:dp:m/54'/784'/0'/0/0:aa:secp256k1",
        "urn:mhda:nt:sui:ct:784:ci:mainnet:dt:bip74:dp:m/74'/784'/0'/0/0:aa:secp256r1",
        "urn:mhda:nt:ada:ct:1815:ci:mainnet:dt:cip1852:dp:m/1852'/1815'/0'/0/0",
        "urn:mhda:nt:ada:ct:1815:ci:mainnet:dt:cip1852:dp:m/1852'/1815'/0'/2/0",
        "urn:mhda:nt:ada:ct:1815:ci:mainnet:dt:cip1852:dp:m/1852'/1815'/0'/0/0:af:base58",
        "urn:mhda:nt:algo:ct:283:ci:mainnet",
        "urn:mhda:nt:algo:ct:283:ci:mainnet:dt:slip10:dp:m/44'/283'/0'/0'/0'",
        "urn:mhda:nt:ton:ct:607:ci:mainnet",
        "urn:mhda:nt:ton:ct:607:ci:mainnet:af:hex",
        "urn:mhda:nt:ton:ct:607:ci:mainnet:dt:slip10:dp:m/44'/607'/0'",
        "urn:mhda:nt:cosmos:ct:118:ci:cosmoshub:dt:cip11:dp:m/44'/118'/0'/0/0",
        "urn:mhda:nt:evm:ct:60:ci:1",
    };
    for (const auto& urn : examples) {
        auto addr = parse_urn_strict(urn);
        EXPECT_EQ(addr.str(), urn);
    }
}

TEST_CASE("TON strict-mode rejection") {
    const std::vector<std::string> bad = {
        "urn:mhda:nt:ton:ct:607:ci:mainnet:aa:secp256k1",
        "urn:mhda:nt:ton:ct:607:ci:mainnet:aa:sr25519",
        "urn:mhda:nt:ton:ct:607:ci:mainnet:af:base58",
        "urn:mhda:nt:ton:ct:607:ci:mainnet:af:bech32",
        "urn:mhda:nt:ton:ct:607:ci:mainnet:af:strkey",
    };
    for (const auto& urn : bad) {
        EXPECT_THROW_CODE(parse_urn_strict(urn), error_code::incompatible);
    }
}

TEST_CASE("Stellar strict-mode rejection") {
    const std::vector<std::string> bad = {
        "urn:mhda:nt:xlm:ct:148:ci:mainnet:aa:secp256k1",
        "urn:mhda:nt:xlm:ct:148:ci:mainnet:af:bech32",
        "urn:mhda:nt:xlm:ct:148:ci:mainnet:af:base58",
    };
    for (const auto& urn : bad) {
        EXPECT_THROW_CODE(parse_urn_strict(urn), error_code::incompatible);
    }
}

TEST_CASE("Sui purpose mapping") {
    struct row { std::string urn; std::uint32_t want; };
    std::vector<row> cases = {
        {"urn:mhda:nt:sui:ct:784:ci:mainnet:dt:slip10:dp:m/44'/784'/0'/0'/0'", 44},
        {"urn:mhda:nt:sui:ct:784:ci:mainnet:dt:bip54:dp:m/54'/784'/0'/0/0:aa:secp256k1", 54},
        {"urn:mhda:nt:sui:ct:784:ci:mainnet:dt:bip74:dp:m/74'/784'/0'/0/0:aa:secp256r1", 74},
    };
    for (const auto& c : cases) {
        auto addr = parse_urn(c.urn);
        const auto& levels = addr.path()->levels();
        EXPECT_EQ(levels[0].index, c.want);
        EXPECT_TRUE(levels[0].is_hardened);
    }
}
