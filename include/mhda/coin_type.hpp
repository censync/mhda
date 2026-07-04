#pragma once

#include <cstdint>

namespace mhda {

// CoinType is a SLIP-44 coin type (32-bit unsigned integer).
using coin_type = std::uint32_t;

// SLIP-44 registered coin types, ascending by index
// (https://github.com/satoshilabs/slips/blob/master/slip-0044.md).
// Matches go-mhda/coin_type.go.
namespace coins {

constexpr coin_type btc   = 0;
constexpr coin_type ltc   = 2;
constexpr coin_type doge  = 3;
constexpr coin_type dash  = 5;
constexpr coin_type eth   = 60;
constexpr coin_type etc   = 61;
// Cosmos Hub. The registry assigns 118 to ATOM (168 belongs to
// Helleniccoin); 118 also matches the coin level of CIP-11 paths.
constexpr coin_type atom  = 118;
constexpr coin_type xmr   = 128;
constexpr coin_type zec   = 133;
constexpr coin_type xrp   = 144;
constexpr coin_type bch   = 145;
constexpr coin_type xlm   = 148;
constexpr coin_type eos   = 194;
constexpr coin_type trx   = 195;
constexpr coin_type icp   = 223;
constexpr coin_type algo  = 283;
constexpr coin_type ckb   = 309;
constexpr coin_type zil   = 313;
constexpr coin_type luna  = 330;
constexpr coin_type dot   = 354;
constexpr coin_type near  = 397;
constexpr coin_type ksm   = 434;
constexpr coin_type kava  = 459;
constexpr coin_type fil   = 461;
constexpr coin_type sol   = 501;
constexpr coin_type cspr  = 506;
constexpr coin_type egld  = 508;
constexpr coin_type scrt  = 529;
constexpr coin_type flow  = 539;
constexpr coin_type ton   = 607;
constexpr coin_type apt   = 637;
constexpr coin_type bnb   = 714;  // BNB Beacon Chain (old style); BSC uses 9006
constexpr coin_type sui   = 784;
constexpr coin_type vet   = 818;
constexpr coin_type rune  = 931;
constexpr coin_type matic = 966;
constexpr coin_type ftm   = 1007;
constexpr coin_type one   = 1023;
constexpr coin_type glmr  = 1284;
constexpr coin_type xtz   = 1729;
constexpr coin_type ada   = 1815;
constexpr coin_type hype  = 2457;
constexpr coin_type hbar  = 3030;
constexpr coin_type move  = 3073;
constexpr coin_type stx   = 5757;
constexpr coin_type bera  = 8008;
constexpr coin_type xch   = 8444;
// https://support.avax.network/en/articles/7004986-what-derivation-paths-does-avalanche-use
constexpr coin_type avax  = 9000;
constexpr coin_type strk  = 9004;
constexpr coin_type bsc   = 9006;
constexpr coin_type mina  = 12586;
constexpr coin_type wax   = 14001;
constexpr coin_type kas   = 111111;
constexpr coin_type osmo  = 10000118;
constexpr coin_type sei   = 19000118;
constexpr coin_type inj   = 22000119;
constexpr coin_type mon   = 268435779;

}  // namespace coins

}  // namespace mhda
