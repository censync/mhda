// <windows.h> (minwindef.h) defines `near` and `far` as empty macros. The
// public headers must compile with them in effect, as they are in any Windows
// translation unit that includes <windows.h> first.
#define near
#define far

#include "mhda/mhda.hpp"
#include "ostream_helpers.hpp"
#include "test_framework.hpp"

TEST_CASE("headers compile under the <windows.h> near/far macros") {
    // coins::near itself is unusable while the macro is defined; its alias
    // near_protocol (after network_type::near_protocol) is not.
    EXPECT_EQ(mhda::coins::near_protocol, mhda::coin_type{397});
}

#undef near
#undef far
