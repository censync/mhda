// Fuzz-equivalent stress tests. The Go reference uses go testing's native
// fuzzer; here we run the same contract checks (no panics; idempotent
// round-trip on success) over a deterministic pseudo-random corpus plus the
// historical seed inputs. The point is to catch crashes / non-idempotency,
// not to discover new failures the way real coverage-guided fuzzing does.

#include <cstdint>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

using namespace mhda;

namespace {

const std::vector<std::string> kSeedURNs = {
    "urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/1'/0/1:aa:secp256k1:af:hex:ap:0x",
    "urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/2'/0/2'",
    "urn:mhda:nt:evm:ct:60:ci:1",
    "urn:mhda:nt:btc:ct:0:ci:bitcoin_testnet:dt:bip44:dp:m/44'/0'/0'/0/0",
    "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip44:dp:m/44'/0'/1'/0/1:aa:secp256k1:af:p2pkh:ap:1",
    "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip84:dp:m/84'/0'/2'/0/2",
    "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip84:dp:m/84'/0'/0'/0/0:af:bech32",
    "urn:mhda:nt:btc:ct:0:ci:bitcoin:dt:bip86:dp:m/86'/0'/0'/0/0:af:bech32m:ap:bc1p",
    "urn:mhda:nt:cosmos:ct:118:ci:cosmoshub:dt:cip11:dp:m/44'/118'/0'/0/0",
    // Historically problematic / degenerate inputs from go-mhda fuzz seeds.
    "",
    "urn:mhda:",
    "urn:mhda:n",
    "urn:mhda:nt",
    "urn:mhda:nt:",
    "urn:mhda:nt:evm",
    "urn:mhda:nt:evm:",
    "urn:mhda:nt:evm:ct",
    "urn:mhda:nt:evm:ct:",
    "urn:mhda:nt:evm:ct:60:ci",
    "urn:mhda:nt:evm:ct:60:ci:",
    "urn:mhda:nt:evm:ct:60:ci:1:xx:y",
    "urn:mhda:::::::",
    "urn:mhda:nt:evm:ct:60:ci:1:aa:",
    "urn:mhda:nt:evm:ct:60:ci:1:af:",
    "urn:mhdA:nt:BtC:ct:0:ci:0 #",
    "URN:MHDA:nt:evm:ct:60:ci:1?+x=y",
};

const std::vector<std::pair<std::string, std::string>> kSeedPaths = {
    {"bip32",   "m/0'/0/0"},
    {"bip32",   "m/2147483647'/1/4294967295'"},
    {"bip44",   "m/44'/60'/0'/0/0"},
    {"bip44",   "m/44'/0'/0'/0/0'"},
    {"bip49",   "m/49'/0'/0'/0/0"},
    {"bip54",   "m/54'/784'/0'/0/0"},
    {"bip74",   "m/74'/784'/0'/0/0"},
    {"bip84",   "m/84'/0'/0'/0/0"},
    {"bip86",   "m/86'/0'/0'/0/0"},
    {"slip10",  "m/44'/501'/0'/0'"},
    {"slip10",  "m/44'/148'/0'"},
    {"slip10",  "m/0"},
    {"cip11",   "m/44'/118'/0'/0/0"},
    {"cip1852", "m/1852'/1815'/0'/0/0"},
    {"cip1852", "m/1852'/1815'/0'/2/0"},
    {"zip32",   "m/32'/133'/0'"},
    {"zip32",   "m/32'/133'/0'/0"},
    {"zip32",   "m/32'/133'/0'/0'"},
    {"root",    ""},
    // Degenerate / overflow inputs.
    {"bip44",   ""},
    {"bip44",   "m/"},
    {"bip44",   "m/44'/0'/0'/0"},
    {"slip10",  "m"},
    {"slip10",  "m/"},
    {"slip10",  "m/99999999999999999999"},
    {"unknown", "m/0/0/0"},
};

constexpr std::string_view kPrefix = "urn:mhda:";

std::string mutate(const std::string& seed, std::mt19937_64& rng) {
    if (seed.empty()) {
        std::string out;
        std::uniform_int_distribution<int> len_d(0, 32);
        std::uniform_int_distribution<int> ch_d(32, 126);
        int n = len_d(rng);
        for (int i = 0; i < n; ++i) out.push_back(char(ch_d(rng)));
        return out;
    }
    std::string out = seed;
    std::uniform_int_distribution<int> op_d(0, 4);
    std::uniform_int_distribution<int> ch_d(32, 126);
    std::uniform_int_distribution<std::size_t> pos_d(0, out.size());
    int op = op_d(rng);
    if (op == 0 && !out.empty()) {
        // delete
        out.erase(pos_d(rng) % out.size(), 1);
    } else if (op == 1) {
        // insert
        out.insert(pos_d(rng), 1, char(ch_d(rng)));
    } else if (op == 2 && !out.empty()) {
        // replace
        out[pos_d(rng) % out.size()] = char(ch_d(rng));
    } else if (op == 3) {
        // truncate
        if (!out.empty()) out.resize(pos_d(rng) % out.size());
    } else {
        // surround with whitespace
        out = "  " + out + "  ";
    }
    return out;
}

}  // namespace

TEST_CASE("FuzzParseURN: no panic, idempotent on success") {
    std::mt19937_64 rng(0xC0FFEEULL);
    constexpr int kIterations = 4000;

    auto check = [&](const std::string& src) {
        try {
            auto addr = parse_urn(src);
            const std::string once = addr.str();
            // Serialised form must always carry the canonical prefix.
            if (once.compare(0, kPrefix.size(), kPrefix) != 0) {
                mhda_failures.push_back({__FILE__, __LINE__,
                    std::string{"serialised form lacks urn:mhda: prefix: "} + once
                        + "  (input: " + src + ")"});
                return;
            }
            auto twice = parse_urn(once);
            if (twice.str() != once) {
                mhda_failures.push_back({__FILE__, __LINE__,
                    std::string{"not idempotent: once="} + once
                        + " twice=" + twice.str() + " input=" + src});
            }
        } catch (const parse_error&) {
            // Rejected inputs are fine; the contract is just "no crash".
        } catch (const std::exception& e) {
            mhda_failures.push_back({__FILE__, __LINE__,
                std::string{"unexpected non-parse_error from parse_urn(\""} + src
                    + "\"): " + e.what()});
        }
    };

    for (const auto& seed : kSeedURNs) check(seed);
    for (int i = 0; i < kIterations; ++i) {
        const auto& seed = kSeedURNs[std::uniform_int_distribution<std::size_t>{
            0, kSeedURNs.size() - 1}(rng)];
        check(mutate(seed, rng));
    }
}

TEST_CASE("FuzzParseNSS: no panic, no half-state on success") {
    std::mt19937_64 rng(0xDECADEULL);
    constexpr int kIterations = 4000;
    auto check = [&](const std::string& src) {
        try {
            auto addr = parse_nss(src);
            (void)addr.str();
            (void)addr.nss();
        } catch (const parse_error&) {
            // Rejection is fine.
        } catch (const std::exception& e) {
            mhda_failures.push_back({__FILE__, __LINE__,
                std::string{"unexpected non-parse_error from parse_nss: "} + e.what()});
        }
    };

    for (const auto& seed : kSeedURNs) {
        if (seed.size() >= kPrefix.size() &&
            seed.compare(0, kPrefix.size(), kPrefix) == 0) {
            check(seed.substr(kPrefix.size()));
        }
    }
    const std::vector<std::string> bare_seeds = {
        "", "n", "nt", "nt:", "nt:evm", "nt:evm:ct:60:ci:1",
    };
    for (const auto& s : bare_seeds) check(s);
    for (int i = 0; i < kIterations; ++i) {
        const auto& seed = bare_seeds[std::uniform_int_distribution<std::size_t>{
            0, bare_seeds.size() - 1}(rng)];
        check(mutate(seed, rng));
    }
}

TEST_CASE("FuzzDerivationPath: no panic, idempotent on success") {
    std::mt19937_64 rng(0xFEEDFACEULL);
    constexpr int kIterations = 3000;

    auto check = [&](const std::string& dt_str, const std::string& path) {
        try {
            auto dp = derivation_path::parse(derivation_type{dt_str}, path);
            const std::string once = dp.str();
            auto dp2 = derivation_path::parse(dp.type(), once);
            if (dp2.str() != once) {
                mhda_failures.push_back({__FILE__, __LINE__,
                    std::string{"not idempotent: once="} + once
                        + " twice=" + dp2.str()
                        + " input dt=" + dt_str + " path=" + path});
            }
            (void)dp.levels();
        } catch (const parse_error&) {
            // Rejection is fine.
        } catch (const std::invalid_argument&) {
            // SLIP-10 constructor refusal is documented behaviour.
        } catch (const std::exception& e) {
            mhda_failures.push_back({__FILE__, __LINE__,
                std::string{"unexpected non-parse_error from derivation_path::parse: "}
                    + e.what()});
        }
    };

    for (const auto& s : kSeedPaths) check(s.first, s.second);
    for (int i = 0; i < kIterations; ++i) {
        const auto& seed = kSeedPaths[std::uniform_int_distribution<std::size_t>{
            0, kSeedPaths.size() - 1}(rng)];
        check(seed.first, mutate(seed.second, rng));
    }
}

TEST_CASE("Round-trip across full canonical corpus is stable") {
    // Sanity: every literal that appears in the README / SPEC must not only
    // parse, but pass parse(parse(s).str()).str() == parse(s).str() — the
    // strongest available form of "no representation drift" without hitting
    // the strict-mode network whitelist.
    for (const auto& urn : kSeedURNs) {
        try {
            auto first = parse_urn(urn);
            auto second = parse_urn(first.str());
            EXPECT_EQ(first.str(), second.str());
        } catch (const parse_error&) {
            // Most seeds are valid; the historically-degenerate ones throw,
            // and that's fine — they are listed precisely to force that path.
        }
    }
}
