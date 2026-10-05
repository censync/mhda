#include <cstddef>
#include <string>
#include <vector>

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

// Digests at the SHA-1/SHA-256 block and padding boundaries: str() of 55,
// 56, 63, 64, 119, 120, 128 and 1000 bytes (the NSS is 9 bytes shorter, so
// it covers 55 too). Computed with Python's hashlib; go-mhda pins the same
// values, so both implementations produce the same identifiers across every
// padding case.
TEST_CASE("Hash reference values at block boundaries") {
    struct row {
        std::size_t size;
        const char* hash;
        const char* hash256;
        const char* nss_hash;
        const char* nss_hash256;
    };
    const std::vector<row> rows = {
        {55, "7230ec9cac1176541d5e454a5977547849806ef3",
              "4bcf74edcd630246d1cb1f03be5dc2deb7cc850270a2d3f279d68166778e2065",
              "3c1704aa5f179b7d21a9f726e20473ac2b5786e1",
              "c85613e0a6562581d353224545cbebc3710ddb496d4b03baddd435fa311b7bbb"},
        {56, "7c08382a5672e118ef014b54c8d1b42e7b6cb90b",
              "a176aa8ff4ccdcb7b05e3e39440cc40b8b17219910f3081c062045abbf331255",
              "8f17a0079072eb355e316ebdbb73aa4328762ff7",
              "242f19fedc19e36ab9c4378b45922e731269ea04c325b58c4a90d4389ffce482"},
        {63, "6b6034fd2e44f4ad8ea9da946a6915f017966ead",
              "0be4d717499d1b2a081c60cf64f0c59b5e09a1a0da1e65c497c83e839caef655",
              "4b8360e09c0db0f1c423342d03db990ee4795695",
              "0bd7a04f6ca2b30eca17273961fa78db6f345e654ed80ac78a85d780d73d0267"},
        {64, "c5b9fb4f88ee7f77e14566483838fe03eb5f722a",
              "be3e95721333ec68988eedb3b6c37c7c48ee30211c3bef96900609e8f1dd3546",
              "fba8bdfdfb761ca75bc1a38886a7219cba376adc",
              "713ea1b363b46219865aded589c56341e7d0e63c9278e7e11ae154f05883d1c5"},
        {119, "91f3a5706cdf32541f6c2d45ea4280399da43847",
              "d04d2a8a7f6df456cada8d263a746f0d21e4b600ef7bdcea4f6e645c17d94914",
              "94285bd9afc8f49eb2059992f3660aee22e76063",
              "d3398acc7f987d3d3c90acd2e4ef958bd7053a8a88e4b2d51b5774b0fb03cfdc"},
        {120, "8f7e2ab8a8a0c101e299154982f0e262bf43b7d0",
              "a4ed7ff3b82d6f7fa7a322ca72971a2b1e6474a5d08475599bc965282acb49c7",
              "7eb1dc21b8685d3b9dbc9c2cbb9240acc90a2660",
              "aba04d6e4a96a9f662bb9ac433ba50f2c34d7991aee8cb2787ab91bc4228e88a"},
        {128, "56e36e13d0581067945cd12955e3ce4b21967a48",
              "b6fb21cc83b07ab98648ec0b1bb04a3d79cd31d231cc07303d24ce73145a46ed",
              "c792663db1797615b2433d56b96bf73d859a3e6f",
              "2c04bb624a604bd96c5b0e4b9e1006bbe898f134b60a40a1ed81d63e03897ea7"},
        {1000, "f3acba196b25a975d6989beef6693337b5987f26",
              "7d57f53b9e4785b1735151906fd63ccb70547a8a2292986b613cf23db7b0f3d7",
              "d16eb7a3991c6ace27d58b3e80e617567cfd242b",
              "2e7dd698a37e3fb7660b19bcad20b60677bd1e7f01d79870daa09a8a1f7269d8"},
    };
    for (const auto& r : rows) {
        const std::string urn = "urn:mhda:nt:evm:ci:" + std::string(r.size - 19, 'x');
        const auto addr = parse_urn(urn);
        EXPECT_EQ(addr.str().size(), r.size);
        EXPECT_EQ(addr.hash(), std::string{r.hash});
        EXPECT_EQ(addr.hash256(), std::string{r.hash256});
        EXPECT_EQ(addr.nss_hash(), std::string{r.nss_hash});
        EXPECT_EQ(addr.nss_hash256(), std::string{r.nss_hash256});
    }
}
