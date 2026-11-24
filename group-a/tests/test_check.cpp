// An example unit test. Copy this file's shape for your own tests:
// one TEST(SuiteName, TestName) per behaviour, with EXPECT_/ASSERT_ checks.
#include <fcntl.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <system_error>

#include "check.hpp"

TEST(Check, ReturnsTheResultWhenTheCallSucceeds) { EXPECT_EQ(check(3, "test"), 3); }

TEST(Check, ThrowsWithErrnoWhenTheCallFails) {
    try {
        check(::open("/this/file/does/not/exist", O_RDONLY), "open");
        FAIL() << "check() should have thrown";
    } catch (const std::system_error& e) {
        EXPECT_EQ(e.code().value(), ENOENT);
        EXPECT_NE(std::string(e.what()).find("open"), std::string::npos);
    }
}
