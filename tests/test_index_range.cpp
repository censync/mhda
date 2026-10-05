// Derivation-path level index range, mirroring index_range_test.go of the Go
// reference: a level index of 2^31 or more is refused at every level of every
// derivation type, and the conformance table shared with go-mhda
// (tests/data/dp_conformance.txt) runs through both the standalone and the
// URN parser.

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

namespace {

constexpr std::uint32_t kMaxLevelIndex = 0x7FFFFFFFu;

}  // namespace

// An index of 2^31 or more collides with the hardened bit of a BIP-32 child
// number: m/44'/60'/2147483648'/0/0 would derive the key of m/44'/60'/0'/0/0,
// and an unhardened 2147483648 would ask for a hardened child through the
// public-key formula.
TEST_CASE("level index of 2^31 or more is refused") {
    const std::vector<std::string> urns = {
        "urn:mhda:nt:evm:ci:1:ct:60:dt:bip44:dp:m/44'/60'/2147483648'/0/0",
        "urn:mhda:nt:evm:ci:1:dt:bip44:dp:m/44'/60'/2147483648'/0/0",
        "urn:mhda:nt:evm:ci:1:dt:bip44:dp:m/44'/60'/0'/0/2147483648",
        "urn:mhda:nt:evm:ci:1:dt:bip44:dp:m/44'/60'/0'/0/4294967295",
        "urn:mhda:nt:evm:ci:1:dt:bip44:dp:m/44'/2147483708'/0'/0/0",
        "urn:mhda:nt:solana:ci:mainnet:dt:slip10:dp:m/44'/501'/2147483648'/0'",
    };
    for (const auto& urn : urns) {
        EXPECT_THROW_CODE(parse_urn(urn), error_code::invalid_derivation_path);
        EXPECT_THROW_CODE(parse_urn_strict(urn), error_code::invalid_derivation_path);
    }

    const std::string max = "urn:mhda:nt:evm:ci:1:dt:bip44:dp:m/44'/60'/2147483647'/0/2147483647'";
    auto a = parse_urn_strict(max);
    EXPECT_EQ(a.str(), max);
    EXPECT_EQ(a.path()->account(), kMaxLevelIndex);
    EXPECT_EQ(a.path()->index(), (address_index{kMaxLevelIndex, true}));
}

// The ct component is SLIP-44 metadata, not a path level, and keeps the full
// 32-bit range.
TEST_CASE("ct is not a path level and keeps 32 bits") {
    const std::vector<std::pair<std::string, coin_type>> rows = {
        {"urn:mhda:nt:evm:ci:1:ct:2147483648:dt:bip44:dp:m/44'/60'/0'/0/0", 2147483648u},
        {"urn:mhda:nt:evm:ci:1:ct:4294967295:dt:bip44:dp:m/44'/60'/0'/0/0", 4294967295u},
    };
    for (const auto& r : rows) {
        auto a = parse_urn(r.first);
        EXPECT_TRUE(a.get_chain().coin().has_value());
        EXPECT_EQ(*a.get_chain().coin(), r.second);
        EXPECT_EQ(a.str(), r.first);
    }
}

// Runs the derivation-path table shared verbatim with go-mhda
// (testdata/dp_conformance.txt there). Every row is parsed standalone, through
// validate_derivation_path and inside a URN; all three must agree with the row.
TEST_CASE("derivation-path conformance table shared with go-mhda") {
    std::ifstream in(MHDA_TEST_DATA_DIR "/dp_conformance.txt");
    if (!in) {
        mhda_failures.push_back({__FILE__, __LINE__,
            "cannot open " MHDA_TEST_DATA_DIR "/dp_conformance.txt"});
        return;
    }

    int rows = 0;
    std::string text;
    for (int line = 1; std::getline(in, text); ++line) {
        if (text.empty() || text[0] == '#') continue;
        std::istringstream fields(text);
        std::string verdict, dt_str, path, canonical, extra;
        fields >> verdict >> dt_str >> path >> canonical >> extra;
        const bool accept = verdict == "accept";
        if (!extra.empty() || (accept && canonical.empty()) ||
            (!accept && (verdict != "refuse" || !canonical.empty()))) {
            mhda_failures.push_back({__FILE__, __LINE__,
                "dp_conformance.txt:" + std::to_string(line) + ": malformed row " + text});
            return;
        }
        ++rows;
        const std::string where = "dp_conformance.txt:" + std::to_string(line) + ": " + text;
        const derivation_type dt{dt_str};

        if (validate_derivation_path(dt, path) != accept) {
            mhda_failures.push_back({__FILE__, __LINE__, "validate_derivation_path disagrees, " + where});
        }

        const std::string urn = "urn:mhda:nt:evm:ci:1:dt:" + dt_str + ":dp:" + path;
        try {
            auto dp = derivation_path::parse(dt, path);
            if (!accept) {
                mhda_failures.push_back({__FILE__, __LINE__, "parse accepted, " + where});
            } else {
                if (dp.str() != canonical) {
                    mhda_failures.push_back({__FILE__, __LINE__,
                        "parse str() = " + dp.str() + ", " + where});
                }
                for (const auto& lvl : dp.levels()) {
                    if (lvl.index > kMaxLevelIndex) {
                        mhda_failures.push_back({__FILE__, __LINE__,
                            "level above 2^31-1, " + where});
                    }
                }
            }
        } catch (const parse_error& e) {
            if (accept || e.code() != error_code::invalid_derivation_path) {
                mhda_failures.push_back({__FILE__, __LINE__,
                    std::string{"parse threw "} + e.what() + ", " + where});
            }
        }

        try {
            auto a = parse_urn(urn);
            if (!accept) {
                mhda_failures.push_back({__FILE__, __LINE__, "parse_urn accepted, " + where});
            } else if (a.str() != "urn:mhda:nt:evm:ci:1:dt:" + dt_str + ":dp:" + canonical) {
                mhda_failures.push_back({__FILE__, __LINE__,
                    "parse_urn str() = " + a.str() + ", " + where});
            }
        } catch (const parse_error& e) {
            if (accept || e.code() != error_code::invalid_derivation_path) {
                mhda_failures.push_back({__FILE__, __LINE__,
                    std::string{"parse_urn threw "} + e.what() + ", " + where});
            }
        }
    }
    if (rows < 300) {
        mhda_failures.push_back({__FILE__, __LINE__,
            "dp_conformance.txt: " + std::to_string(rows) + " rows, the table is truncated"});
    }
}
