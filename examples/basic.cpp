// Minimal smoke test exercising the public mhda API.

#include <iostream>

#include "mhda/mhda.hpp"

int main() {
    using namespace mhda;

    // Lenient parsing: structural validation only.
    auto addr = parse_urn("urn:mhda:nt:evm:ct:60:ci:1");
    std::cout << "network    : " << addr.get_chain().network().str() << "\n";
    std::cout << "algorithm  : " << addr.resolved_algorithm().str() << "\n";
    std::cout << "format     : " << addr.resolved_format().str() << "\n";

    // Strict parsing rejects nonsensical (network, algorithm, format) combos.
    try {
        parse_urn_strict("urn:mhda:nt:evm:ct:60:ci:1:aa:ed25519");
    } catch (const parse_error& e) {
        if (e.code() == error_code::incompatible) {
            std::cout << "evm + ed25519 rejected, as expected\n";
        }
    }

    // Type-agnostic level-by-level path inspection.
    auto bip = parse_urn("urn:mhda:nt:evm:ct:60:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0");
    int i = 0;
    for (const auto& lvl : bip.path()->levels()) {
        std::cout << "level[" << i++ << "] = " << lvl.index
                  << (lvl.is_hardened ? " (hardened)" : "") << "\n";
    }

    // Hashing for content-addressing or deduplication.
    std::cout << "sha256     : " << bip.hash256() << "\n";

    // The chain-domain triple is itself a parseable key.
    auto key = bip.get_chain().key();
    auto parsed = chain::from_key(key);
    std::cout << "chain key  : " << parsed.str() << "\n";

    return 0;
}
