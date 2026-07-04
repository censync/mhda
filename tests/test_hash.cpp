#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

TEST_CASE("Hash determinism and shape") {
    auto a1 = parse_urn("urn:mhda:nt:evm:ci:1:ct:60");
    auto a2 = parse_urn("urn:mhda:nt:evm:ci:1:ct:60");

    EXPECT_EQ(a1.hash(),       a2.hash());
    EXPECT_EQ(a1.hash256(),    a2.hash256());
    EXPECT_EQ(a1.nss_hash(),   a2.nss_hash());
    EXPECT_EQ(a1.nss_hash256(), a2.nss_hash256());

    EXPECT_EQ(a1.hash().size(), 40u);
    EXPECT_EQ(a1.hash256().size(), 64u);

    auto b = parse_urn("urn:mhda:nt:evm:ci:2:ct:60");
    EXPECT_NE(a1.hash256(), b.hash256());

    EXPECT_NE(a1.hash(), a1.nss_hash());
    EXPECT_NE(a1.hash256(), a1.nss_hash256());
}

// Reference values pre-computed against the canonical strings via OpenSSL
// (sha1sum, sha256sum on the exact byte content of str() / nss()).
TEST_CASE("Hash reference values") {
    auto addr = parse_urn("urn:mhda:nt:evm:ci:1:ct:60");
    EXPECT_EQ(addr.hash(),
              std::string{"25e531aaa6f4668cf5b42d30cffb25b4fd359c10"});
    EXPECT_EQ(addr.nss_hash(),
              std::string{"85f36c062d728ff1c5a6001efde906f4367cbcf7"});
    EXPECT_EQ(addr.hash256(),
              std::string{"a682965c81c59da0d8d8d32fdf168ac31e530b394a99ba2a357f3ffe52f74a11"});
    EXPECT_EQ(addr.nss_hash256(),
              std::string{"828f1579eb41e06dcf1a4b32a0223ce57f4be481d79e62735ca64c4a3696a5a2"});
}
