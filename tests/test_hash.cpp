#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

TEST_CASE("Hash determinism and shape") {
    auto a1 = parse_urn("urn:mhda:nt:evm:ct:60:ci:1");
    auto a2 = parse_urn("urn:mhda:nt:evm:ct:60:ci:1");

    EXPECT_EQ(a1.hash(),       a2.hash());
    EXPECT_EQ(a1.hash256(),    a2.hash256());
    EXPECT_EQ(a1.nss_hash(),   a2.nss_hash());
    EXPECT_EQ(a1.nss_hash256(), a2.nss_hash256());

    EXPECT_EQ(a1.hash().size(), 40u);
    EXPECT_EQ(a1.hash256().size(), 64u);

    auto b = parse_urn("urn:mhda:nt:evm:ct:60:ci:2");
    EXPECT_NE(a1.hash256(), b.hash256());

    EXPECT_NE(a1.hash(), a1.nss_hash());
    EXPECT_NE(a1.hash256(), a1.nss_hash256());
}

// Reference values pre-computed against the canonical strings via OpenSSL
// (sha1sum, sha256sum on the exact byte content of str() / nss()).
TEST_CASE("Hash reference values") {
    auto addr = parse_urn("urn:mhda:nt:evm:ct:60:ci:1");
    EXPECT_EQ(addr.hash(),
              std::string{"1b67879a4e427b4b26dbf1518569b8ddebb6b6ba"});
    EXPECT_EQ(addr.nss_hash(),
              std::string{"5f3e128a6968997f0b00f629296feb5d90678799"});
    EXPECT_EQ(addr.hash256(),
              std::string{"47ff599055bf943d1fca281f2177859709e2c2dfedb3f75a955dd8c0e65ed034"});
    EXPECT_EQ(addr.nss_hash256(),
              std::string{"88429e10123e1d49cf67d44145a5493c08bf599937541e0dee4fc00873eb8215"});
}
