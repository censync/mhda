#include <string>
#include <vector>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

namespace {

const std::vector<std::string> kCorpus = {
    "urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/1'/0/1:aa:secp256k1:af:hex:ap:0x",
    "urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/2'/0/2'",
    "urn:mhda:nt:evm:ct:60:ci:1",
    "urn:mhda:nt:btc:ct:0:ci:bitcoin_testnet:dt:bip44:dp:m/44'/0'/0'/0/0",
    "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip44:dp:m/44'/0'/1'/0/1:aa:secp256k1:af:p2pkh:ap:1",
    "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip84:dp:m/84'/0'/2'/0/2",
    "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip84:dp:m/84'/0'/0'/0/0:af:bech32",
    "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip86:dp:m/86'/0'/0'/0/0:af:bech32m:ap:bc1p",
    "urn:mhda:nt:cosmos:ct:118:ci:cosmoshub:dt:cip11:dp:m/44'/118'/0'/0/0",
};

}  // namespace

TEST_CASE("parse the canonical corpus") {
    for (const auto& s : kCorpus) {
        EXPECT_NO_THROW(parse_urn(s));
    }
}

TEST_CASE("nss equals stripped canonical form") {
    for (const auto& s : kCorpus) {
        auto addr = parse_urn(s);
        EXPECT_EQ(addr.nss(), s.substr(std::string{"urn:mhda:"}.size()));
    }
}

TEST_CASE("round-trip canonical inputs") {
    for (const auto& s : kCorpus) {
        auto addr = parse_urn(s);
        EXPECT_EQ(addr.str(), s);
    }
}

TEST_CASE("idempotent re-parse") {
    for (const auto& s : kCorpus) {
        auto first  = parse_urn(s);
        auto second = parse_urn(first.str());
        EXPECT_EQ(first.str(), second.str());
    }
}

TEST_CASE("sentinel error codes") {
    struct row { std::string in; error_code want; };
    std::vector<row> cases = {
        {"mhda:nt:evm:ct:60:ci:1",                              error_code::invalid_urn},
        {"urn:mhda:ct:60:ci:1",                                 error_code::missing_network_type},
        {"urn:mhda:nt:notanetwork:ct:60:ci:1",                  error_code::invalid_network_type},
        {"urn:mhda:nt:evm:ci:1",                                error_code::missing_coin_type},
        {"urn:mhda:nt:evm:ct:notanumber:ci:1",                  error_code::invalid_coin_type},
        {"urn:mhda:nt:evm:ct:60",                               error_code::missing_chain_id},
        {"urn:mhda:nt:evm:ct:60:ci:1:dt:bipxx:dp:m/44'/60'/0'/0/0",
                                                                error_code::invalid_derivation_type},
        {"urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:not_a_path",   error_code::invalid_derivation_path},
        {"urn:mhda:nt:evm:ct:60:ci:1:aa:rsa",                   error_code::invalid_algorithm},
        {"urn:mhda:nt:evm:ct:60:ci:1:af:notaformat",            error_code::invalid_format},
    };
    for (const auto& c : cases) {
        EXPECT_THROW_CODE(parse_urn(c.in), c.want);
    }
}

TEST_CASE("RFC 8141 case-insensitive prefix") {
    const std::string canonical = "urn:mhda:nt:evm:ct:60:ci:1";
    const std::vector<std::string> variants = {
        "urn:mhda:nt:evm:ct:60:ci:1",
        "URN:MHDA:nt:evm:ct:60:ci:1",
        "Urn:Mhda:nt:evm:ct:60:ci:1",
        "URN:mhda:nt:evm:ct:60:ci:1",
        "urn:MHDA:nt:evm:ct:60:ci:1",
        "  urn:mhda:nt:evm:ct:60:ci:1  ",
    };
    for (const auto& v : variants) {
        auto addr = parse_urn(v);
        EXPECT_EQ(addr.str(), canonical);
    }
}

TEST_CASE("RFC 8141 strips rq/f components") {
    const std::string canonical = "urn:mhda:nt:evm:ct:60:ci:1";
    const std::vector<std::string> variants = {
        "urn:mhda:nt:evm:ct:60:ci:1?+resolver=example.com",
        "urn:mhda:nt:evm:ct:60:ci:1?=v=1",
        "urn:mhda:nt:evm:ct:60:ci:1#fragment",
        "urn:mhda:nt:evm:ct:60:ci:1?+a=b#frag",
    };
    for (const auto& v : variants) {
        auto addr = parse_urn(v);
        EXPECT_EQ(addr.str(), canonical);
    }
}

TEST_CASE("hardened marker normalisation") {
    auto a = parse_urn("urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44H/60H/0H/0/0");
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0"});
    auto b = parse_urn("urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44h/60h/0h/0/0");
    EXPECT_EQ(b.str(), std::string{"urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0"});
}

TEST_CASE("empty component values rejected") {
    const std::vector<std::string> bad = {
        "urn:mhda:nt:evm:dt::dp::ct:60:ci:1",
        "urn:mhda:nt::ct:60:ci:1",
        "urn:mhda:nt:evm:ct::ci:1",
        "urn:mhda:nt:evm:ct:60:ci:",
    };
    for (const auto& s : bad) {
        bool threw = false;
        try { parse_urn(s); } catch (const parse_error&) { threw = true; }
        EXPECT_TRUE(threw);
    }
}

TEST_CASE("derivation type accessor") {
    auto a = parse_urn("urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0");
    EXPECT_EQ(a.get_derivation_type(), derivation_type::bip44);
    auto root = parse_urn("urn:mhda:nt:evm:ct:60:ci:1");
    EXPECT_EQ(root.get_derivation_type(), derivation_type::root);
}

TEST_CASE("regression: whitespace before fragment") {
    const std::string in = "urn:mhdA:nt:BtC:ct:0:ci:0 #";
    auto once  = parse_urn(in);
    auto twice = parse_urn(once.str());
    EXPECT_EQ(once.str(), twice.str());
}

TEST_CASE("MarshalText on uninitialised address fails") {
    address a;
    EXPECT_THROW_CODE(a.marshal_text(), error_code::uninitialized_address);
}

TEST_CASE("UnmarshalText round-trip") {
    address a;
    EXPECT_THROW_CODE(a.unmarshal_text("not-a-urn"), error_code::invalid_urn);
    a.unmarshal_text("urn:mhda:nt:evm:ct:60:ci:1");
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ct:60:ci:1"});
}

TEST_CASE("set prefix/suffix reset") {
    address a{
        chain{network_type::ethereum_vm, coins::eth, "1"},
        std::nullopt,
        "", "", "0xPREFIX", "SUFFIX",
    };
    EXPECT_TRUE(a.str().find(":ap:0xPREFIX") != std::string::npos);
    EXPECT_TRUE(a.str().find(":as:SUFFIX") != std::string::npos);
    a.set_address_prefix("");
    a.set_address_suffix("");
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ct:60:ci:1"});
}

TEST_CASE("set_derivation_path is no-op for ROOT") {
    address a{chain{network_type::ethereum_vm, coins::eth, "1"}, std::nullopt};
    a.set_derivation_type("");
    a.set_derivation_path("m/44'/0'/0'/0/0");  // silently ignored
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ct:60:ci:1"});
}
