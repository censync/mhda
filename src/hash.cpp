#include "hash.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace mhda {
namespace detail {

namespace {

constexpr char hex_chars[] = "0123456789abcdef";

void to_hex(const std::uint8_t* in, std::size_t len, std::string& out) {
    out.resize(len * 2);
    for (std::size_t i = 0; i < len; ++i) {
        out[2 * i]     = hex_chars[(in[i] >> 4) & 0xF];
        out[2 * i + 1] = hex_chars[in[i] & 0xF];
    }
}

inline std::uint32_t rotl32(std::uint32_t x, unsigned n) noexcept {
    return (x << n) | (x >> (32 - n));
}

inline std::uint32_t rotr32(std::uint32_t x, unsigned n) noexcept {
    return (x >> n) | (x << (32 - n));
}

// ---- SHA-1 ----------------------------------------------------------------

void sha1_compress(std::uint32_t state[5], const std::uint8_t block[64]) {
    std::uint32_t w[80];
    for (int i = 0; i < 16; ++i) {
        w[i] = (std::uint32_t(block[4 * i]) << 24) |
               (std::uint32_t(block[4 * i + 1]) << 16) |
               (std::uint32_t(block[4 * i + 2]) << 8)  |
                std::uint32_t(block[4 * i + 3]);
    }
    for (int i = 16; i < 80; ++i) {
        w[i] = rotl32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
    for (int i = 0; i < 80; ++i) {
        std::uint32_t f, k;
        if (i < 20)      { f = (b & c) | (~b & d);              k = 0x5A827999u; }
        else if (i < 40) { f = b ^ c ^ d;                       k = 0x6ED9EBA1u; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d);     k = 0x8F1BBCDCu; }
        else             { f = b ^ c ^ d;                       k = 0xCA62C1D6u; }
        std::uint32_t t = rotl32(a, 5) + f + e + k + w[i];
        e = d; d = c; c = rotl32(b, 30); b = a; a = t;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

std::array<std::uint8_t, 20> sha1(const void* data, std::size_t n) {
    std::uint32_t state[5] = {
        0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u,
    };
    const std::uint8_t* bytes = static_cast<const std::uint8_t*>(data);
    std::size_t full_blocks = n / 64;
    for (std::size_t i = 0; i < full_blocks; ++i) {
        sha1_compress(state, bytes + i * 64);
    }
    std::uint8_t tail[128] = {0};
    std::size_t rem = n - full_blocks * 64;
    if (rem) std::memcpy(tail, bytes + full_blocks * 64, rem);
    tail[rem] = 0x80;
    std::size_t pad_len = (rem < 56) ? 64 : 128;
    std::uint64_t bit_len = std::uint64_t(n) * 8;
    for (int i = 0; i < 8; ++i) {
        tail[pad_len - 1 - i] = std::uint8_t(bit_len >> (8 * i));
    }
    sha1_compress(state, tail);
    if (pad_len == 128) sha1_compress(state, tail + 64);

    std::array<std::uint8_t, 20> out{};
    for (int i = 0; i < 5; ++i) {
        out[4 * i]     = std::uint8_t(state[i] >> 24);
        out[4 * i + 1] = std::uint8_t(state[i] >> 16);
        out[4 * i + 2] = std::uint8_t(state[i] >> 8);
        out[4 * i + 3] = std::uint8_t(state[i]);
    }
    return out;
}

// ---- SHA-256 --------------------------------------------------------------

const std::uint32_t k256[64] = {
    0x428A2F98u,0x71374491u,0xB5C0FBCFu,0xE9B5DBA5u,0x3956C25Bu,0x59F111F1u,0x923F82A4u,0xAB1C5ED5u,
    0xD807AA98u,0x12835B01u,0x243185BEu,0x550C7DC3u,0x72BE5D74u,0x80DEB1FEu,0x9BDC06A7u,0xC19BF174u,
    0xE49B69C1u,0xEFBE4786u,0x0FC19DC6u,0x240CA1CCu,0x2DE92C6Fu,0x4A7484AAu,0x5CB0A9DCu,0x76F988DAu,
    0x983E5152u,0xA831C66Du,0xB00327C8u,0xBF597FC7u,0xC6E00BF3u,0xD5A79147u,0x06CA6351u,0x14292967u,
    0x27B70A85u,0x2E1B2138u,0x4D2C6DFCu,0x53380D13u,0x650A7354u,0x766A0ABBu,0x81C2C92Eu,0x92722C85u,
    0xA2BFE8A1u,0xA81A664Bu,0xC24B8B70u,0xC76C51A3u,0xD192E819u,0xD6990624u,0xF40E3585u,0x106AA070u,
    0x19A4C116u,0x1E376C08u,0x2748774Cu,0x34B0BCB5u,0x391C0CB3u,0x4ED8AA4Au,0x5B9CCA4Fu,0x682E6FF3u,
    0x748F82EEu,0x78A5636Fu,0x84C87814u,0x8CC70208u,0x90BEFFFAu,0xA4506CEBu,0xBEF9A3F7u,0xC67178F2u,
};

void sha256_compress(std::uint32_t state[8], const std::uint8_t block[64]) {
    std::uint32_t w[64];
    for (int i = 0; i < 16; ++i) {
        w[i] = (std::uint32_t(block[4 * i]) << 24) |
               (std::uint32_t(block[4 * i + 1]) << 16) |
               (std::uint32_t(block[4 * i + 2]) << 8)  |
                std::uint32_t(block[4 * i + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        std::uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        std::uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19)  ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    std::uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
    for (int i = 0; i < 64; ++i) {
        std::uint32_t S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        std::uint32_t ch = (e & f) ^ (~e & g);
        std::uint32_t t1 = h + S1 + ch + k256[i] + w[i];
        std::uint32_t S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        std::uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
        std::uint32_t t2 = S0 + mj;
        h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

std::array<std::uint8_t, 32> sha256(const void* data, std::size_t n) {
    std::uint32_t state[8] = {
        0x6A09E667u, 0xBB67AE85u, 0x3C6EF372u, 0xA54FF53Au,
        0x510E527Fu, 0x9B05688Cu, 0x1F83D9ABu, 0x5BE0CD19u,
    };
    const std::uint8_t* bytes = static_cast<const std::uint8_t*>(data);
    std::size_t full_blocks = n / 64;
    for (std::size_t i = 0; i < full_blocks; ++i) {
        sha256_compress(state, bytes + i * 64);
    }
    std::uint8_t tail[128] = {0};
    std::size_t rem = n - full_blocks * 64;
    if (rem) std::memcpy(tail, bytes + full_blocks * 64, rem);
    tail[rem] = 0x80;
    std::size_t pad_len = (rem < 56) ? 64 : 128;
    std::uint64_t bit_len = std::uint64_t(n) * 8;
    for (int i = 0; i < 8; ++i) {
        tail[pad_len - 1 - i] = std::uint8_t(bit_len >> (8 * i));
    }
    sha256_compress(state, tail);
    if (pad_len == 128) sha256_compress(state, tail + 64);

    std::array<std::uint8_t, 32> out{};
    for (int i = 0; i < 8; ++i) {
        out[4 * i]     = std::uint8_t(state[i] >> 24);
        out[4 * i + 1] = std::uint8_t(state[i] >> 16);
        out[4 * i + 2] = std::uint8_t(state[i] >> 8);
        out[4 * i + 3] = std::uint8_t(state[i]);
    }
    return out;
}

}  // namespace

std::string sha1_hex(std::string_view data) {
    auto digest = sha1(data.data(), data.size());
    std::string out;
    to_hex(digest.data(), digest.size(), out);
    return out;
}

std::string sha256_hex(std::string_view data) {
    auto digest = sha256(data.data(), data.size());
    std::string out;
    to_hex(digest.data(), digest.size(), out);
    return out;
}

}  // namespace detail
}  // namespace mhda
