// State of derivation_path and address across construction, re-parsing and
// type changes, mirroring state_test.go of the Go reference: a constructed
// path is exactly what the parser gives back for its own str(), parsing
// replaces a path as a whole, and a type change drops the old path.

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

TEST_CASE("constructors make only parseable paths") {
    const std::vector<std::pair<std::string, std::function<void()>>> refused = {
        {"bip44 purpose 49", [] { derivation_path::from_levels(derivation_type::bip44,
              {{49, true}, {60, true}, {0, true}, {0, false}, {0, false}}); }},
        {"bip44 soft account", [] { derivation_path::from_levels(derivation_type::bip44,
              {{44, true}, {60, true}, {0, false}, {0, false}, {0, false}}); }},
        {"bip44 two levels", [] { derivation_path::from_levels(derivation_type::bip44,
              {{44, true}, {60, true}}); }},
        {"slip10 no levels", [] { derivation_path::from_levels(derivation_type::slip10, {}); }},
        {"slip10 index 2^31", [] { derivation_path::from_levels(derivation_type::slip10,
              {{0x80000000u, false}}); }},
        {"root with a level", [] { derivation_path::from_levels(derivation_type::root,
              {{0, true}}); }},
        {"bip44 account 2^31", [] { derivation_path(derivation_type::bip44, 60, 0x80000000u, 0,
              address_index{}); }},
        {"bip44 charge 7", [] { derivation_path(derivation_type::bip44, 60, 0, 7, address_index{}); }},
        {"bip32 index 4294967295", [] { derivation_path(derivation_type::bip32, 0, 0, 0,
              address_index{4294967295u, false}); }},
    };
    for (const auto& r : refused) {
        bool code_ok = false;
        try {
            r.second();
        } catch (const parse_error& e) {
            code_ok = e.code() == error_code::invalid_derivation_path;
        }
        if (!code_ok) {
            mhda_failures.push_back({__FILE__, __LINE__,
                r.first + ": expected parse_error(invalid_derivation_path)"});
        }
    }

    const auto dp = derivation_path::from_levels(derivation_type::bip44,
        {{44, true}, {60, true}, {0, true}, {0, false}, {5, false}});
    const auto parsed = derivation_path::parse(derivation_type::bip44, "m/44'/60'/0'/0/5");
    EXPECT_EQ(dp.str(), parsed.str());
    EXPECT_TRUE(dp.levels() == parsed.levels());
    EXPECT_EQ(dp.coin(), parsed.coin());
    EXPECT_EQ(dp.account(), parsed.account());
    EXPECT_EQ(dp.charge(), parsed.charge());
    EXPECT_EQ(dp.index(), parsed.index());
    // BIP-32 has no coin level: the constructor drops the coin like the parser.
    EXPECT_EQ(derivation_path(derivation_type::bip32, 60, 0, 0, address_index{}).coin(), 0u);
}
