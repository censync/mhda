// Consumer used by the packaging checks in tests/cmake: it only needs to
// compile against the public headers, link mhda::mhda and run.
#include <mhda/mhda.hpp>

int main() {
    const auto a = mhda::parse_urn_strict("urn:mhda:nt:evm:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0");
    return a.str() == "urn:mhda:nt:evm:ci:1:dt:bip44:dp:m/44'/60'/0'/0/0" ? 0 : 1;
}
