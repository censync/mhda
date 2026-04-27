#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

TEST_CASE("chain.from_nss round-trip") {
    const char* keys[] = {
        "nt:btc:ct:0:ci:bitcoin",
        "nt:tvm:ct:195:ci:mainnet",
        "nt:evm:ct:60:ci:0x1",
        "nt:evm:ct:60:ci:0xa86a",
    };
    for (auto* k : keys) {
        auto c = chain::from_nss(k);
        EXPECT_EQ(c.str(), std::string{k});
    }
}

TEST_CASE("chain getters chainable from address") {
    auto addr = parse_urn("urn:mhda:nt:evm:ct:60:ci:1");
    EXPECT_EQ(addr.get_chain().network(), network_type::ethereum_vm);
    EXPECT_EQ(addr.get_chain().coin(), coins::eth);
    EXPECT_EQ(addr.get_chain().id(), std::string{"1"});
    EXPECT_EQ(addr.get_chain().key(), std::string{"nt:evm:ct:60:ci:1"});
}

TEST_CASE("chain setters mutate in place") {
    chain c{network_type::ethereum_vm, coins::eth, "1"};
    c.set_network(network_type::bitcoin);
    c.set_coin(coins::btc);
    c.set_chain_id("bitcoin");
    EXPECT_EQ(c.network(), network_type::bitcoin);
    EXPECT_EQ(c.coin(), coins::btc);
    EXPECT_EQ(c.id(), std::string{"bitcoin"});
}

TEST_CASE("chain.str canonical form") {
    chain c{network_type::ethereum_vm, coins::eth, "0x1"};
    EXPECT_EQ(c.str(), std::string{"nt:evm:ct:60:ci:0x1"});
    EXPECT_EQ(c.key(), std::string{"nt:evm:ct:60:ci:0x1"});
}
