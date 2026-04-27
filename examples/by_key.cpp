// by_key — a small command-line tool that takes an MHDA URN as its argument
// (or reads one per line from stdin) and prints a structured breakdown of
// the descriptor: chain triple, derivation path with level-by-level view,
// resolved algorithm/format defaults, prefix/suffix metadata and the four
// hash flavours. It also runs strict validation and reports the result.
//
// Build:
//   cmake --build build --target mhda_by_key
//
// Examples:
//   ./build/examples/mhda_by_key  'urn:mhda:nt:evm:ct:60:ci:1'
//   echo 'urn:mhda:nt:sol:ct:501:ci:mainnet' | ./build/examples/mhda_by_key

#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "mhda/mhda.hpp"

namespace {

constexpr const char* kUsage =
    "Usage: mhda_by_key <urn> [<urn>...]\n"
    "       mhda_by_key                  (one URN per line on stdin)\n";

// describe writes a human-readable, single-line-per-field rendering of an
// MHDA address to out. Returns 0 on success, 1 if strict validation rejects
// the descriptor (the lenient parse already succeeded by the time we get
// here, so the only remaining failure is the network-compat matrix).
int describe(std::ostream& out, const std::string& urn) {
    using namespace mhda;

    address addr;
    try {
        addr = parse_urn(urn);
    } catch (const parse_error& e) {
        out << "input  : " << urn << "\n"
            << "ERROR  : " << e.what() << " (code=" << static_cast<int>(e.code()) << ")\n\n";
        return 2;
    }

    out << "input  : " << urn << "\n"
        << "canon. : " << addr.str() << "\n"
        << "nss    : " << addr.nss() << "\n"
        << "------- chain --------\n"
        << "  network    : " << addr.get_chain().network().str() << "\n"
        << "  coin (slip): " << addr.get_chain().coin() << "\n"
        << "  chain id   : " << addr.get_chain().id() << "\n"
        << "  key        : " << addr.get_chain().key() << "\n";

    out << "------- derivation --\n"
        << "  type       : " << addr.get_derivation_type().str() << "\n";
    if (addr.path()) {
        const auto& dp = *addr.path();
        out << "  path       : " << (dp.str().empty() ? "(root)" : dp.str()) << "\n";
        if (!dp.levels().empty()) {
            out << "  levels     :\n";
            std::size_t i = 0;
            for (const auto& lvl : dp.levels()) {
                out << "    [" << i++ << "] " << lvl.index
                    << (lvl.is_hardened ? "'  (hardened)" : "  (soft)") << "\n";
            }
        }
        if (dp.has_index()) {
            out << "  account    : " << dp.account() << "\n";
            out << "  charge     : " << static_cast<unsigned>(dp.charge()) << "\n";
            out << "  index      : " << dp.index().index
                << (dp.index().is_hardened ? "'" : "") << "\n";
        }
    }

    out << "------- format ------\n"
        << "  algorithm  : " << addr.resolved_algorithm().str();
    if (addr.explicit_algorithm().empty()) out << "  (default)";
    out << "\n"
        << "  format     : " << (addr.resolved_format().empty()
                                     ? "(unset)"
                                     : addr.resolved_format().str());
    if (addr.explicit_format().empty() && !addr.resolved_format().empty()) {
        out << "  (default)";
    }
    out << "\n";
    if (!addr.prefix().empty()) out << "  prefix     : " << addr.prefix() << "\n";
    if (!addr.suffix().empty()) out << "  suffix     : " << addr.suffix() << "\n";

    out << "------- hashes ------\n"
        << "  sha1(urn)  : " << addr.hash() << "\n"
        << "  sha1(nss)  : " << addr.nss_hash() << "\n"
        << "  sha256(urn): " << addr.hash256() << "\n"
        << "  sha256(nss): " << addr.nss_hash256() << "\n";

    out << "------- validate ----\n";
    int rc = 0;
    try {
        addr.validate();
        out << "  strict     : OK\n\n";
    } catch (const parse_error& e) {
        out << "  strict     : REJECTED (" << e.what() << ")\n\n";
        rc = 1;
    }
    return rc;
}

}  // namespace

int main(int argc, char** argv) {
    int worst_rc = 0;

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            const std::string urn = argv[i];
            if (urn == "-h" || urn == "--help") {
                std::cout << kUsage;
                return 0;
            }
            int rc = describe(std::cout, urn);
            if (rc > worst_rc) worst_rc = rc;
        }
        return worst_rc;
    }

    // No CLI args — read URNs line by line from stdin until EOF.
    std::string line;
    bool any = false;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        any = true;
        int rc = describe(std::cout, line);
        if (rc > worst_rc) worst_rc = rc;
    }
    if (!any) {
        std::cerr << kUsage;
        return EXIT_FAILURE;
    }
    return worst_rc;
}
