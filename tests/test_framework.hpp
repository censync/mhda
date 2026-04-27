#pragma once

// Tiny self-contained test framework — no external dependency. Each TEST_CASE
// registers itself via a static initializer; main() iterates the registry,
// runs every case, and exits non-zero if any expectation fails.

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mhda_test {

struct case_failure {
    std::string file;
    int         line;
    std::string detail;
};

struct test_case {
    const char* name;
    void (*fn)(std::vector<case_failure>&);
};

inline std::vector<test_case>& registry() {
    static std::vector<test_case> r;
    return r;
}

struct registrar {
    registrar(const char* name, void (*fn)(std::vector<case_failure>&)) {
        registry().push_back({name, fn});
    }
};

inline int run_all() {
    std::size_t passed = 0, failed = 0;
    for (const auto& c : registry()) {
        std::vector<case_failure> failures;
        try {
            c.fn(failures);
        } catch (const std::exception& e) {
            failures.push_back({"<unhandled exception>", 0, e.what()});
        } catch (...) {
            failures.push_back({"<unhandled exception>", 0, "<non-std exception>"});
        }
        if (failures.empty()) {
            std::cout << "[ OK ] " << c.name << "\n";
            ++passed;
        } else {
            std::cout << "[FAIL] " << c.name << "\n";
            for (const auto& f : failures) {
                std::cout << "       " << f.file << ":" << f.line << ": " << f.detail << "\n";
            }
            ++failed;
        }
    }
    std::cout << "\n" << passed << " passed, " << failed << " failed, "
              << registry().size() << " total\n";
    return failed == 0 ? 0 : 1;
}

}  // namespace mhda_test

#define MHDA_CONCAT_INNER(a, b) a##b
#define MHDA_CONCAT(a, b) MHDA_CONCAT_INNER(a, b)

#define TEST_CASE(name_literal)                                                 \
    static void MHDA_CONCAT(mhda_test_fn_, __LINE__)(                          \
        std::vector<::mhda_test::case_failure>& mhda_failures);                \
    static ::mhda_test::registrar MHDA_CONCAT(mhda_test_reg_, __LINE__){       \
        name_literal, &MHDA_CONCAT(mhda_test_fn_, __LINE__)};                  \
    static void MHDA_CONCAT(mhda_test_fn_, __LINE__)(                          \
        std::vector<::mhda_test::case_failure>& mhda_failures)

#define EXPECT_TRUE(cond)                                                       \
    do {                                                                        \
        if (!(cond)) {                                                          \
            mhda_failures.push_back({__FILE__, __LINE__,                        \
                std::string{"EXPECT_TRUE("} + #cond + ") failed"});             \
        }                                                                       \
    } while (0)

#define EXPECT_FALSE(cond)                                                      \
    do {                                                                        \
        if ((cond)) {                                                           \
            mhda_failures.push_back({__FILE__, __LINE__,                        \
                std::string{"EXPECT_FALSE("} + #cond + ") failed"});            \
        }                                                                       \
    } while (0)

#define EXPECT_EQ(a, b)                                                         \
    do {                                                                        \
        auto&& mhda_a = (a);                                                    \
        auto&& mhda_b = (b);                                                    \
        if (!(mhda_a == mhda_b)) {                                              \
            std::ostringstream oss;                                             \
            oss << "EXPECT_EQ(" << #a << ", " << #b << "): "                    \
                << mhda_a << " != " << mhda_b;                                  \
            mhda_failures.push_back({__FILE__, __LINE__, oss.str()});           \
        }                                                                       \
    } while (0)

#define EXPECT_NE(a, b)                                                         \
    do {                                                                        \
        auto&& mhda_a = (a);                                                    \
        auto&& mhda_b = (b);                                                    \
        if (mhda_a == mhda_b) {                                                 \
            std::ostringstream oss;                                             \
            oss << "EXPECT_NE(" << #a << ", " << #b << "): both are "           \
                << mhda_a;                                                      \
            mhda_failures.push_back({__FILE__, __LINE__, oss.str()});           \
        }                                                                       \
    } while (0)

#define EXPECT_THROW_CODE(stmt, expected_code)                                  \
    do {                                                                        \
        bool mhda_caught = false;                                               \
        ::mhda::error_code mhda_actual{};                                       \
        std::string mhda_what;                                                  \
        try { (void)(stmt); }                                                   \
        catch (const ::mhda::parse_error& e) {                                  \
            mhda_caught = true;                                                 \
            mhda_actual = e.code();                                             \
            mhda_what   = e.what();                                             \
        } catch (const std::exception& e) {                                     \
            mhda_failures.push_back({__FILE__, __LINE__,                        \
                std::string{"unexpected exception: "} + e.what()});             \
            break;                                                              \
        }                                                                       \
        if (!mhda_caught) {                                                     \
            mhda_failures.push_back({__FILE__, __LINE__,                        \
                std::string{"expected parse_error from `"} + #stmt + "`"});     \
        } else if (mhda_actual != (expected_code)) {                            \
            std::ostringstream oss;                                             \
            oss << "wrong error code from `" << #stmt << "`: got "              \
                << int(mhda_actual) << " (" << mhda_what << "), want "          \
                << int(expected_code);                                          \
            mhda_failures.push_back({__FILE__, __LINE__, oss.str()});           \
        }                                                                       \
    } while (0)

#define EXPECT_NO_THROW(stmt)                                                   \
    do {                                                                        \
        try { (void)(stmt); }                                                   \
        catch (const std::exception& e) {                                       \
            mhda_failures.push_back({__FILE__, __LINE__,                        \
                std::string{"unexpected exception from `"} + #stmt + "`: "      \
                    + e.what()});                                               \
        } catch (...) {                                                         \
            mhda_failures.push_back({__FILE__, __LINE__,                        \
                std::string{"unexpected non-std exception from `"} + #stmt + "`"});\
        }                                                                       \
    } while (0)
