// Contract tests for KW-1: tokenize.   (owner: lead)
// Switched on when src/kw/stub_tokenizer.cpp is deleted. The rules are in issues/KW-1.md.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "forge/keyword_index.hpp"

using forge::tokenize;
using Words = std::vector<std::string>;

TEST(Tokenize, AFinancialSentence) {
    EXPECT_EQ(tokenize("Revenue rose to ₹1,45,000 crore in FY2025."),
              (Words{"revenue", "rose", "to", "1,45,000", "crore", "in", "fy2025"}));
}

TEST(Tokenize, NothingInNothingOut) {
    EXPECT_EQ(tokenize(""), Words{});
    EXPECT_EQ(tokenize("  .,;!? \t\n"), Words{});
    EXPECT_EQ(tokenize("₹"), Words{});
}

TEST(Tokenize, LowercasesAsciiOnly) {
    EXPECT_EQ(tokenize("Hello\tWORLD\n"), (Words{"hello", "world"}));
    EXPECT_EQ(tokenize("ÀB"), (Words{"Àb"})) << "bytes of non-ASCII letters are kept as they are";
}

TEST(Tokenize, CommasAndDotsInsideNumbers) {
    EXPECT_EQ(tokenize("3.5% growth"), (Words{"3.5", "growth"}));
    EXPECT_EQ(tokenize("1,2,3"), (Words{"1,2,3"}));
    EXPECT_EQ(tokenize("v2.0.1"), (Words{"v2.0.1"}));
    EXPECT_EQ(tokenize("a,b"), (Words{"a", "b"}));
    EXPECT_EQ(tokenize("1."), (Words{"1"}));
    EXPECT_EQ(tokenize(".5"), (Words{"5"}));
    EXPECT_EQ(tokenize("1..2"), (Words{"1", "2"}));
    EXPECT_EQ(tokenize("1,,000"), (Words{"1", "000"}));
    EXPECT_EQ(tokenize("year 2024, then 2025."), (Words{"year", "2024", "then", "2025"}));
}

TEST(Tokenize, EverythingElseSeparatesWords) {
    EXPECT_EQ(tokenize("end."), (Words{"end"}));
    EXPECT_EQ(tokenize("U.S.A."), (Words{"u", "s", "a"}));
    EXPECT_EQ(tokenize("e-mail x_y don't"), (Words{"e", "mail", "x", "y", "don", "t"}));
    EXPECT_EQ(tokenize("(profit)/[loss]"), (Words{"profit", "loss"}));
}

TEST(Tokenize, TheRupeeSignSeparates) {
    EXPECT_EQ(tokenize("₹500"), (Words{"500"}));
    EXPECT_EQ(tokenize("Rs₹500"), (Words{"rs", "500"}));
}

TEST(Tokenize, OtherScriptsAreWords) {
    EXPECT_EQ(tokenize("नमस्ते दुनिया"), (Words{"नमस्ते", "दुनिया"}));
}
