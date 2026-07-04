#include <string>
#include <vector>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

namespace {

// Mirrors nssChainKey from the Go reference chain tests.
const std::vector<std::string> kChainKeys = {
    "nt:bitcoin:ci:bitcoin",  // Bitcoin
    "nt:tron:ci:mainnet",     // Tron
    "nt:evm:ci:0x1",          // Ethereum
    "nt:evm:ci:0xa86a",       // Avalanche
};

}  // namespace

TEST_CASE("chain.from_nss round-trip") {
    for (const auto& k : kChainKeys) {
        auto c = chain::from_nss(k);
        EXPECT_EQ(c.str(), k);
    }
}

// The strict chain-key parser: a chain key is the bare identity
// "nt:<network>:ci:<chain_id>" and round-trips through key().
TEST_CASE("chain.from_key round-trip") {
    for (const auto& k : kChainKeys) {
        auto c = chain::from_key(k);
        EXPECT_EQ(c.key(), k);
        EXPECT_FALSE(c.coin().has_value());  // keys never carry coin-type metadata
    }
}

// The pre-1.1 key format (with an embedded "ct" component) must fail loudly
// with the dedicated sentinel instead of being silently reinterpreted.
TEST_CASE("chain.from_key rejects coin type") {
    const std::vector<std::string> keys = {
        "nt:evm:ct:60:ci:1",  // pre-1.1 canonical order
        "nt:evm:ci:1:ct:60",  // ct trailing
        "nt:bitcoin:ct:0:ci:bitcoin",
    };
    for (const auto& k : keys) {
        EXPECT_THROW_CODE(chain::from_key(k), error_code::coin_type_in_chain_key);
    }
}

// A chain key may not carry derivation, address-format or wallet components.
TEST_CASE("chain.from_key rejects extra components") {
    const std::vector<std::string> keys = {
        "nt:evm:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0",
        "nt:evm:ci:1:aa:secp256k1",
        "nt:evm:ci:1:wt:web3",
        "nt:evm:ci:1:wi:5f2a8c31",
    };
    for (const auto& k : keys) {
        EXPECT_THROW_CODE(chain::from_key(k), error_code::invalid_chain_key);
    }
}

// The lenient NSS extractor accepts the optional ct metadata (in any position)
// and keeps it off the key.
TEST_CASE("chain.from_nss accepts optional coin type") {
    const std::vector<std::string> inputs = {
        "nt:evm:ci:1:ct:60",
        "nt:evm:ct:60:ci:1",  // pre-1.1 component order still parses as NSS
        "nt:evm:ci:1:ct:60:dt:bip44:dp:m/44'/60'/0'/0/0",
    };
    for (const auto& nss : inputs) {
        auto c = chain::from_nss(nss);
        EXPECT_TRUE(c.coin().has_value());
        if (c.coin()) EXPECT_EQ(*c.coin(), 60u);
        EXPECT_EQ(c.str(), std::string{"nt:evm:ci:1"});
    }
}

TEST_CASE("chain getters chainable from address") {
    auto addr = parse_urn("urn:mhda:nt:evm:ci:1:ct:60");
    EXPECT_EQ(addr.get_chain().network(), network_type::ethereum_vm);
    EXPECT_TRUE(addr.get_chain().coin().has_value());
    EXPECT_EQ(*addr.get_chain().coin(), coins::eth);
    EXPECT_EQ(addr.get_chain().id(), std::string{"1"});
    // The coin-type metadata is excluded from the chain key.
    EXPECT_EQ(addr.get_chain().key(), std::string{"nt:evm:ci:1"});
}

TEST_CASE("chain setters mutate in place") {
    chain c{network_type::ethereum_vm, "1"};
    c.set_network(network_type::bitcoin);
    c.set_coin(coins::btc);
    c.set_chain_id("bitcoin");
    EXPECT_EQ(c.network(), network_type::bitcoin);
    EXPECT_TRUE(c.coin().has_value());
    EXPECT_EQ(*c.coin(), coins::btc);
    EXPECT_EQ(c.id(), std::string{"bitcoin"});
    c.clear_coin();
    EXPECT_FALSE(c.coin().has_value());
}

TEST_CASE("chain.str canonical form") {
    chain c{network_type::ethereum_vm, "0x1"};
    const std::string want = "nt:evm:ci:0x1";
    EXPECT_EQ(c.str(), want);
    EXPECT_EQ(c.key(), want);
    // Attaching coin-type metadata must not change the key.
    c.set_coin(coins::eth);
    EXPECT_EQ(c.str(), want);
    EXPECT_EQ(c.key(), want);
}

TEST_CASE("chain equality is the (nt, ci) identity") {
    chain a{network_type::ethereum_vm, "1"};
    chain b{network_type::ethereum_vm, "1"};
    b.set_coin(coins::eth);  // metadata never participates in equality
    EXPECT_TRUE(a == b);
    chain other{network_type::ethereum_vm, "10"};
    EXPECT_TRUE(a != other);
}
