#pragma once

#include <cstdint>

namespace mhda {

// CoinType is a SLIP-44 coin type (32-bit unsigned integer).
using coin_type = std::uint32_t;

// Pre-registered SLIP-44 constants matching go-mhda/coin_type.go.
namespace coins {

constexpr coin_type btc   = 0;
constexpr coin_type ltc   = 2;
constexpr coin_type doge  = 3;
constexpr coin_type dash  = 5;
constexpr coin_type eth   = 60;
constexpr coin_type xmr   = 128;
constexpr coin_type zec   = 133;
constexpr coin_type xrp   = 144;
constexpr coin_type xlm   = 148;
constexpr coin_type atom  = 168;
constexpr coin_type trx   = 195;
constexpr coin_type algo  = 283;
constexpr coin_type near  = 397;
constexpr coin_type sol   = 501;
constexpr coin_type ton   = 607;
constexpr coin_type apt   = 637;
constexpr coin_type bnb   = 714;
constexpr coin_type matic = 966;
constexpr coin_type sui   = 784;
constexpr coin_type ada   = 1815;
constexpr coin_type glmr  = 1284;
constexpr coin_type bsc   = 9006;
// https://support.avax.network/en/articles/7004986-what-derivation-paths-does-avalanche-use
constexpr coin_type avax  = 9000;

}  // namespace coins

}  // namespace mhda
