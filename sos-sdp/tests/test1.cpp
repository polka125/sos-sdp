//
// Created by sergey on 29.05.23.
//

#include <gtest/gtest.h>
#include "testproj.h"

// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
// Expect two strings not to be equal.
EXPECT_STRNE("hello", "world");
// Expect equality.
EXPECT_EQ(7 * 6, 42);
}

TEST(HelloTest, BasicAssertions1) {
    EXPECT_EQ(test1(), "test1");
}

