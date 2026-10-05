// mhda used from a consumer's namespace-scope objects: while the program is
// initialised (a built-in chain table, a URN validated up front, a lookup)
// and while it is torn down (a destructor that still parses). The named
// constants and the lookup tables must already exist in the first case and
// still exist in the second.

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

namespace {

constexpr const char* kURN = "urn:mhda:nt:evm:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0";

// Defined before g_init, so it is constructed before g_init builds the
// lookup tables and destroyed after them, unless the tables outlive static
// destruction. A failure here can only end the process: exit code 3.
struct exit_parser {
    ~exit_parser() {
        try {
            (void)parse_urn_strict(kURN);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "parse_urn_strict at static destruction threw: %s\n", e.what());
            std::_Exit(3);
        }
    }
};
exit_parser g_exit;

struct init_probe {
    std::string btc_key;
    std::string evm_lookup;
    std::string strict_error = "not run";

    init_probe() {
        try {
            btc_key = chain{network_type::bitcoin, "0"}.key();
            auto nt = network_type_from_string("evm");
            evm_lookup = nt ? nt->str() : std::string{};
            (void)parse_urn_strict(kURN);
            strict_error.clear();
        } catch (const std::exception& e) {
            strict_error = e.what();
        }
    }
};
const init_probe g_init;

}  // namespace

TEST_CASE("mhda works from a consumer's static initialisers") {
    EXPECT_EQ(g_init.btc_key, std::string{"nt:bitcoin:ci:0"});
    EXPECT_EQ(g_init.evm_lookup, std::string{"evm"});
    EXPECT_EQ(g_init.strict_error, std::string{});
    // Tables first built during static initialisation stay right for the
    // rest of the process.
    EXPECT_NO_THROW(parse_urn_strict(kURN));
    EXPECT_NO_THROW(parse_urn_strict("urn:mhda:nt:bitcoin:ci:0:dt:bip84:dp:m/84'/0'/0'/0/0:af:p2wpkh"));
}
