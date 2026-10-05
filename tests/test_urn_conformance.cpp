// URN-level conformance table shared verbatim with go-mhda
// (testdata/urn_conformance.txt there): an input one implementation refuses,
// the other refuses too, with the same error.

#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

namespace {

const std::map<std::string, error_code>& conformance_errors() {
    static const std::map<std::string, error_code> m = {
        {"invalid_urn",             error_code::invalid_urn},
        {"invalid_nss",             error_code::invalid_nss},
        {"missing_network_type",    error_code::missing_network_type},
        {"invalid_network_type",    error_code::invalid_network_type},
        {"invalid_coin_type",       error_code::invalid_coin_type},
        {"missing_chain_id",        error_code::missing_chain_id},
        {"invalid_derivation_type", error_code::invalid_derivation_type},
        {"invalid_derivation_path", error_code::invalid_derivation_path},
        {"invalid_algorithm",       error_code::invalid_algorithm},
        {"invalid_format",          error_code::invalid_format},
        {"invalid_value",           error_code::invalid_value},
    };
    return m;
}

}  // namespace

TEST_CASE("URN conformance table shared with go-mhda") {
    std::ifstream in(MHDA_TEST_DATA_DIR "/urn_conformance.txt");
    if (!in) {
        mhda_failures.push_back({__FILE__, __LINE__,
            "cannot open " MHDA_TEST_DATA_DIR "/urn_conformance.txt"});
        return;
    }

    int rows = 0;
    std::string text;
    for (int line = 1; std::getline(in, text); ++line) {
        if (text.empty() || text[0] == '#') continue;
        std::istringstream fields(text);
        std::string verdict, kind, a, b, extra;
        fields >> verdict >> kind >> a >> b >> extra;
        const std::string where = "urn_conformance.txt:" + std::to_string(line) + ": " + text;
        if (!extra.empty() || b.empty() || (kind != "urn" && kind != "nss") ||
            (verdict != "accept" && verdict != "refuse")) {
            mhda_failures.push_back({__FILE__, __LINE__, "malformed row, " + where});
            return;
        }
        ++rows;
        const bool accept = verdict == "accept";
        const std::string& input = accept ? a : b;
        error_code want{};
        if (!accept) {
            auto it = conformance_errors().find(a);
            if (it == conformance_errors().end()) {
                mhda_failures.push_back({__FILE__, __LINE__, "unknown error name, " + where});
                return;
            }
            want = it->second;
        }

        try {
            const address addr = kind == "urn" ? parse_urn(input) : parse_nss(input);
            if (!accept) {
                mhda_failures.push_back({__FILE__, __LINE__, "accepted, " + where});
            } else if (addr.str() != b) {
                mhda_failures.push_back({__FILE__, __LINE__,
                    "str() = " + addr.str() + ", " + where});
            }
        } catch (const parse_error& e) {
            if (accept || e.code() != want) {
                mhda_failures.push_back({__FILE__, __LINE__,
                    std::string{"threw "} + e.what() + ", " + where});
            }
        }
    }
    if (rows == 0) {
        mhda_failures.push_back({__FILE__, __LINE__, "urn_conformance.txt: no rows"});
    }
}

// The setter refuses a path while the type is root, and accepts an empty one.
TEST_CASE("set_derivation_path under root") {
    auto addr = parse_urn("urn:mhda:nt:evm:ci:1");
    EXPECT_THROW_CODE(addr.set_derivation_path("m/44'/60'/0'/0/0"),
                      error_code::invalid_derivation_path);
    EXPECT_NO_THROW(addr.set_derivation_path(""));
    EXPECT_NO_THROW(addr.set_derivation_path("  "));
    EXPECT_EQ(addr.str(), std::string{"urn:mhda:nt:evm:ci:1"});
}
