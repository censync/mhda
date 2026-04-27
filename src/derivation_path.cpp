#include "mhda/derivation_path.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

#include "detail.hpp"
#include "mhda/error.hpp"

namespace mhda {

namespace {

struct raw_level {
    std::uint32_t index;
    bool          is_hardened;
};

// is_hardening_marker reports whether c is one of the accepted hardening
// markers ('\'', 'h', 'H'). Mirrors the Go regex character class.
bool is_hardening_marker(char c) noexcept {
    return c == '\'' || c == 'h' || c == 'H';
}

// parse_segment decomposes a single path segment into (numeric_part, hardened
// flag). Returns false on empty/non-numeric input or 32-bit overflow.
bool parse_segment(std::string_view seg, raw_level& out) noexcept {
    bool hardened = false;
    if (!seg.empty() && is_hardening_marker(seg.back())) {
        hardened = true;
        seg.remove_suffix(1);
    }
    std::uint32_t value = 0;
    if (!detail::parse_uint32_dec(seg, value)) return false;
    out.index = value;
    out.is_hardened = hardened;
    return true;
}

// split_path splits "m/<seg>/<seg>/..." into raw segments after stripping the
// leading "m". Returns false if the path does not begin with "m" or contains
// any structurally bogus shape (consecutive slashes, missing leaf, missing m).
bool split_path(std::string_view path, std::vector<std::string_view>& out) {
    if (path.empty() || path[0] != 'm') return false;
    if (path.size() == 1) return true;  // bare "m" -> zero segments
    if (path[1] != '/')   return false;
    auto rest = path.substr(2);
    if (rest.empty()) return false;  // "m/" alone is invalid
    out = detail::split(rest, '/');
    for (auto seg : out) {
        if (seg.empty()) return false;
    }
    return true;
}

bool parse_levels(std::string_view path, std::vector<raw_level>& levels) {
    std::vector<std::string_view> segments;
    if (!split_path(path, segments)) return false;
    levels.clear();
    levels.reserve(segments.size());
    for (auto seg : segments) {
        raw_level lvl;
        if (!parse_segment(seg, lvl)) return false;
        levels.push_back(lvl);
    }
    return true;
}

// expect_purpose verifies that level[0] is the given purpose (hardened).
bool expect_purpose(const std::vector<raw_level>& lvls, std::uint32_t purpose) {
    return !lvls.empty() && lvls[0].index == purpose && lvls[0].is_hardened;
}

// expect_coin verifies that level[1] is the given coin (hardened).
bool expect_coin(const std::vector<raw_level>& lvls, std::uint32_t coin) {
    return lvls.size() >= 2 && lvls[1].index == coin && lvls[1].is_hardened;
}

// validate_bip44_family validates the 5-level BIP-44-shaped layout:
//   m/<purpose>'/<coin>'/account'/charge/index[']
// where charge ∈ {0,1}. Coin is constrained when required != 0.
bool validate_bip44_family(const std::vector<raw_level>& lvls,
                           std::uint32_t purpose,
                           std::uint32_t required_coin = 0,
                           bool require_coin_match = false) {
    if (lvls.size() != 5) return false;
    if (!expect_purpose(lvls, purpose)) return false;
    if (!lvls[1].is_hardened) return false;
    if (require_coin_match && lvls[1].index != required_coin) return false;
    if (!lvls[2].is_hardened) return false;            // account hardened
    if (lvls[3].is_hardened)  return false;            // charge soft
    if (lvls[3].index > 1)    return false;            // charge ∈ {0,1}
    return true;  // index leaf: arbitrary, hardening optional
}

bool validate_bip32(const std::vector<raw_level>& lvls) {
    // m/account'/charge/index[']
    if (lvls.size() != 3) return false;
    if (!lvls[0].is_hardened) return false;
    if (lvls[1].is_hardened)  return false;
    if (lvls[1].index > 1)    return false;
    return true;
}

bool validate_slip10(const std::vector<raw_level>& lvls) {
    return !lvls.empty();  // any number of levels >= 1
}

bool validate_cip11(const std::vector<raw_level>& lvls) {
    // m/44'/118'/account'/charge/index[']  (charge accepts any non-negative
    // integer per the Go regex — `([0-9]+)`, not the {0,1} of bip44).
    if (lvls.size() != 5) return false;
    if (!expect_purpose(lvls, 44)) return false;
    if (!expect_coin(lvls, 118))   return false;
    if (!lvls[2].is_hardened)      return false;
    if (lvls[3].is_hardened)       return false;
    return true;
}

bool validate_cip1852(const std::vector<raw_level>& lvls) {
    // m/1852'/1815'/account'/role/index[']  (role: any non-negative integer)
    if (lvls.size() != 5) return false;
    if (!expect_purpose(lvls, 1852)) return false;
    if (!expect_coin(lvls, 1815))    return false;
    if (!lvls[2].is_hardened)        return false;
    if (lvls[3].is_hardened)         return false;
    return true;
}

bool validate_zip32(const std::vector<raw_level>& lvls) {
    // m/32'/133'/account'  or  m/32'/133'/account'/index[']
    if (lvls.size() != 3 && lvls.size() != 4) return false;
    if (!expect_purpose(lvls, 32)) return false;
    if (!expect_coin(lvls, 133))   return false;
    if (!lvls[2].is_hardened)      return false;
    return true;
}

bool validate_levels_for(const derivation_type& dt, const std::vector<raw_level>& lvls) {
    if (dt == derivation_type::root) {
        return lvls.empty();
    }
    if (dt == derivation_type::bip32)   return validate_bip32(lvls);
    if (dt == derivation_type::bip44)   return validate_bip44_family(lvls, 44);
    if (dt == derivation_type::bip49)   return validate_bip44_family(lvls, 49);
    if (dt == derivation_type::bip54)   return validate_bip44_family(lvls, 54);
    if (dt == derivation_type::bip74)   return validate_bip44_family(lvls, 74);
    if (dt == derivation_type::bip84)   return validate_bip44_family(lvls, 84);
    if (dt == derivation_type::bip86)   return validate_bip44_family(lvls, 86);
    if (dt == derivation_type::slip10)  return validate_slip10(lvls);
    if (dt == derivation_type::cip11)   return validate_cip11(lvls);
    if (dt == derivation_type::cip1852) return validate_cip1852(lvls);
    if (dt == derivation_type::zip32)   return validate_zip32(lvls);
    return false;
}

}  // namespace

bool validate_derivation_path(const derivation_type& dt, std::string_view path) {
    if (!dt.is_valid()) return false;
    if (dt == derivation_type::root) return path.empty();
    std::vector<raw_level> lvls;
    if (!parse_levels(path, lvls)) return false;
    return validate_levels_for(dt, lvls);
}

derivation_path::derivation_path(derivation_type dt,
                                 coin_type coin,
                                 account_index account,
                                 charge_type charge,
                                 address_index index)
    : type_(std::move(dt)),
      coin_(coin),
      account_(account),
      charge_(charge),
      index_(index) {
    if (type_ == derivation_type::slip10) {
        throw std::invalid_argument(
            "mhda: NewDerivationPath cannot construct SLIP10 paths; use derivation_path::from_levels");
    }
    has_index_ = (type_ != derivation_type::root);
    rebuild_levels();
}

derivation_path derivation_path::from_levels(derivation_type dt,
                                             std::vector<address_index> levels) {
    derivation_path dp;
    dp.type_ = std::move(dt);
    dp.levels_ = std::move(levels);
    dp.populate_shortcuts_from_levels();
    return dp;
}

derivation_path derivation_path::parse(derivation_type dt, std::string_view path) {
    if (!dt.is_valid()) {
        throw parse_error(error_code::invalid_derivation_type,
                          std::string{"\""} + dt.str() + "\"");
    }
    derivation_path dp;
    dp.type_ = std::move(dt);
    dp.parse_path(path);
    return dp;
}

void derivation_path::set_type(const derivation_type& dt) {
    type_ = dt;
}

void derivation_path::parse_path(std::string_view path) {
    if (!type_.is_valid()) {
        throw parse_error(error_code::invalid_derivation_type,
                          std::string{"\""} + type_.str() + "\"");
    }
    if (type_ == derivation_type::root) {
        if (!path.empty()) {
            throw parse_error(error_code::invalid_derivation_path,
                              std::string{"root derivation must have empty path, got \""}
                                  + std::string{path} + "\"");
        }
        levels_.clear();
        has_index_ = false;
        return;
    }

    std::vector<raw_level> lvls;
    if (!parse_levels(path, lvls) || !validate_levels_for(type_, lvls)) {
        throw parse_error(error_code::invalid_derivation_path,
                          std::string{"\""} + std::string{path} + "\"");
    }

    if (type_ == derivation_type::bip32) {
        account_ = account_index(lvls[0].index);
        charge_  = charge_type(lvls[1].index);
        index_   = address_index{lvls[2].index, lvls[2].is_hardened};
        has_index_ = true;
    } else if (type_ == derivation_type::bip44 ||
               type_ == derivation_type::bip49 ||
               type_ == derivation_type::bip54 ||
               type_ == derivation_type::bip74 ||
               type_ == derivation_type::bip84 ||
               type_ == derivation_type::bip86) {
        coin_    = coin_type(lvls[1].index);
        account_ = account_index(lvls[2].index);
        charge_  = charge_type(lvls[3].index);
        index_   = address_index{lvls[4].index, lvls[4].is_hardened};
        has_index_ = true;
    } else if (type_ == derivation_type::cip11) {
        coin_    = 118;
        account_ = account_index(lvls[2].index);
        charge_  = charge_type(lvls[3].index);
        index_   = address_index{lvls[4].index, lvls[4].is_hardened};
        has_index_ = true;
    } else if (type_ == derivation_type::cip1852) {
        coin_    = 1815;
        account_ = account_index(lvls[2].index);
        charge_  = charge_type(lvls[3].index);
        index_   = address_index{lvls[4].index, lvls[4].is_hardened};
        has_index_ = true;
    } else if (type_ == derivation_type::slip10) {
        levels_.clear();
        levels_.reserve(lvls.size());
        for (auto& l : lvls) {
            levels_.push_back(address_index{l.index, l.is_hardened});
        }
        has_index_ = !levels_.empty();
        // slip10's levels[] is the source of truth — skip the rebuild that
        // the BIP-family branches rely on.
        return;
    } else if (type_ == derivation_type::zip32) {
        coin_    = 133;
        account_ = account_index(lvls[2].index);
        if (lvls.size() == 4) {
            index_ = address_index{lvls[3].index, lvls[3].is_hardened};
            has_index_ = true;
        } else {
            index_ = address_index{};
            has_index_ = false;
        }
    } else {
        throw parse_error(error_code::invalid_derivation_type,
                          std::string{"\""} + type_.str() + "\"");
    }

    rebuild_levels();
}

void derivation_path::populate_shortcuts_from_levels() {
    if (type_ == derivation_type::bip32) {
        if (levels_.size() >= 3) {
            account_ = account_index(levels_[0].index);
            charge_  = charge_type(levels_[1].index);
            index_   = levels_[2];
            has_index_ = true;
        }
    } else if (type_ == derivation_type::bip44 ||
               type_ == derivation_type::bip49 ||
               type_ == derivation_type::bip54 ||
               type_ == derivation_type::bip74 ||
               type_ == derivation_type::bip84 ||
               type_ == derivation_type::bip86 ||
               type_ == derivation_type::cip11 ||
               type_ == derivation_type::cip1852) {
        if (levels_.size() >= 5) {
            coin_    = coin_type(levels_[1].index);
            account_ = account_index(levels_[2].index);
            charge_  = charge_type(levels_[3].index);
            index_   = levels_[4];
            has_index_ = true;
        }
    } else if (type_ == derivation_type::zip32) {
        if (levels_.size() >= 3) {
            coin_    = coin_type(levels_[1].index);
            account_ = account_index(levels_[2].index);
        }
        if (levels_.size() >= 4) {
            index_     = levels_[3];
            has_index_ = true;
        }
    } else if (type_ == derivation_type::slip10) {
        has_index_ = !levels_.empty();
    }
}

bool derivation_path::fixed_prefix(std::uint32_t& purpose, std::uint32_t& coin) const {
    if (type_ == derivation_type::bip44)   { purpose = 44;   coin = coin_; return true; }
    if (type_ == derivation_type::bip49)   { purpose = 49;   coin = coin_; return true; }
    if (type_ == derivation_type::bip54)   { purpose = 54;   coin = coin_; return true; }
    if (type_ == derivation_type::bip74)   { purpose = 74;   coin = coin_; return true; }
    if (type_ == derivation_type::bip84)   { purpose = 84;   coin = coin_; return true; }
    if (type_ == derivation_type::bip86)   { purpose = 86;   coin = coin_; return true; }
    if (type_ == derivation_type::cip11)   { purpose = 44;   coin = 118;   return true; }
    if (type_ == derivation_type::cip1852) { purpose = 1852; coin = 1815;  return true; }
    if (type_ == derivation_type::zip32)   { purpose = 32;   coin = 133;   return true; }
    return false;
}

void derivation_path::rebuild_levels() {
    if (type_ == derivation_type::root) {
        levels_.clear();
        return;
    }
    if (type_ == derivation_type::slip10) {
        // levels_ is the source of truth for slip10; nothing to rebuild.
        return;
    }
    if (type_ == derivation_type::bip32) {
        levels_.clear();
        levels_.reserve(3);
        levels_.push_back(address_index{account_, true});
        levels_.push_back(address_index{charge_, false});
        levels_.push_back(index_);
        return;
    }

    std::uint32_t purpose = 0, coin = 0;
    if (!fixed_prefix(purpose, coin)) return;

    levels_.clear();
    levels_.reserve(5);
    levels_.push_back(address_index{purpose, true});
    levels_.push_back(address_index{coin, true});
    levels_.push_back(address_index{account_, true});

    if (type_ == derivation_type::zip32) {
        if (has_index_) levels_.push_back(index_);
        return;
    }
    levels_.push_back(address_index{charge_, false});
    levels_.push_back(index_);
}

namespace {

void append_uint32(std::string& out, std::uint32_t v) {
    char buf[16];
    int n = 0;
    if (v == 0) { buf[n++] = '0'; }
    else {
        char tmp[16];
        int t = 0;
        while (v) { tmp[t++] = char('0' + (v % 10)); v /= 10; }
        for (int i = 0; i < t; ++i) buf[n++] = tmp[t - i - 1];
    }
    out.append(buf, std::size_t(n));
}

std::string format_levels(const std::vector<address_index>& lvls) {
    std::string out;
    out.reserve(2 + lvls.size() * 8);
    out.push_back('m');
    for (const auto& l : lvls) {
        out.push_back('/');
        append_uint32(out, l.index);
        if (l.is_hardened) out.push_back('\'');
    }
    return out;
}

}  // namespace

std::string derivation_path::str() const {
    if (type_ == derivation_type::root) return "";
    if (type_ == derivation_type::slip10) return format_levels(levels_);
    if (type_ == derivation_type::bip32) {
        std::string out = "m/";
        append_uint32(out, account_);
        out += "'/";
        append_uint32(out, charge_);
        out += '/';
        append_uint32(out, index_.index);
        if (index_.is_hardened) out += '\'';
        return out;
    }

    std::uint32_t purpose = 0, coin = 0;
    if (!fixed_prefix(purpose, coin)) return "";

    if (type_ == derivation_type::zip32) {
        std::string out = "m/";
        append_uint32(out, purpose);
        out += "'/";
        append_uint32(out, coin);
        out += "'/";
        append_uint32(out, account_);
        out += '\'';
        if (!has_index_) return out;
        out += '/';
        append_uint32(out, index_.index);
        if (index_.is_hardened) out += '\'';
        return out;
    }

    std::string out = "m/";
    append_uint32(out, purpose);
    out += "'/";
    append_uint32(out, coin);
    out += "'/";
    append_uint32(out, account_);
    out += "'/";
    append_uint32(out, charge_);
    out += '/';
    append_uint32(out, index_.index);
    if (index_.is_hardened) out += '\'';
    return out;
}

}  // namespace mhda
