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

TEST_CASE("re-parsing leaves no stale state") {
    // ZIP-32: a 3-level path after a 4-level one has no index left over.
    auto dp = derivation_path::parse(derivation_type::zip32, "m/32'/133'/0'/5");
    dp.parse_path("m/32'/133'/1'");
    EXPECT_EQ(dp.str(), std::string{"m/32'/133'/1'"});
    EXPECT_EQ(dp.levels().size(), 3u);
    EXPECT_EQ(dp.index(), address_index{});

    // SLIP-10 after BIP-44: the type change drops the BIP-44 shortcuts.
    auto sl = derivation_path::parse(derivation_type::bip44, "m/44'/60'/5'/1/9");
    sl.set_type(derivation_type::slip10);
    EXPECT_TRUE(sl.levels().empty());
    EXPECT_EQ(sl.str(), std::string{});
    sl.parse_path("m/0'");
    EXPECT_EQ(sl.coin(), 0u);
    EXPECT_EQ(sl.account(), 0u);
    EXPECT_EQ(sl.charge(), 0u);
    EXPECT_EQ(sl.index(), address_index{});

    // The same type keeps the path; a failed parse changes nothing.
    auto keep = derivation_path::parse(derivation_type::bip44, "m/44'/60'/3'/1/9");
    keep.set_type(derivation_type::bip44);
    EXPECT_THROW_CODE(keep.parse_path("m/44'/60'/4'/0/2147483648"),
                      error_code::invalid_derivation_path);
    EXPECT_EQ(keep.str(), std::string{"m/44'/60'/3'/1/9"});
    EXPECT_EQ(keep.account(), 3u);
}

// A new derivation type invalidates the old path. Until a path is set the
// address names no path at all: its URN carries dt without dp (and so does
// not parse), and validate / marshal_text refuse it.
TEST_CASE("a derivation type change drops the path") {
    const std::string bip44 = "urn:mhda:nt:evm:ci:1:dt:bip44:dp:m/44'/60'/3'/1/9";
    auto a = parse_urn(bip44);
    a.set_derivation_type("zip32");
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ci:1:dt:zip32"});
    EXPECT_TRUE(a.path()->levels().empty());
    EXPECT_THROW_CODE(a.validate(), error_code::invalid_derivation_path);
    EXPECT_THROW_CODE(a.marshal_text(), error_code::invalid_derivation_path);
    EXPECT_THROW_CODE(parse_urn(a.str()), error_code::invalid_derivation_path);
    a.set_derivation_path("m/32'/133'/7'");
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ci:1:dt:zip32:dp:m/32'/133'/7'"});

    // The same type, in any spelling, keeps the path.
    auto b = parse_urn(bip44);
    b.set_derivation_type(" BIP44 ");
    EXPECT_EQ(b.str(), bip44);
}

// set_derivation validates the type and the path before changing anything.
TEST_CASE("set_derivation is atomic") {
    const std::string bip44 = "urn:mhda:nt:evm:ci:1:dt:bip44:dp:m/44'/60'/3'/1/9";
    auto a = parse_urn(bip44);
    EXPECT_THROW_CODE(a.set_derivation("slip10", "garbage"), error_code::invalid_derivation_path);
    EXPECT_THROW_CODE(a.set_derivation("bogus", "m/0'"), error_code::invalid_derivation_type);
    EXPECT_EQ(a.str(), bip44);
    a.set_derivation("slip10", "m/0'/1'");
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ci:1:dt:slip10:dp:m/0'/1'"});
    EXPECT_EQ(a.path()->account(), 0u);
    a.set_derivation("", "");
    EXPECT_EQ(a.str(), std::string{"urn:mhda:nt:evm:ci:1"});
}
