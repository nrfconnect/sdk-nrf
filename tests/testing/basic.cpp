#include <gtest/gtest.h>

#include "test.h"

TEST(Testing, Basic)
{
    EXPECT_EQ(TestClass::Value, 10);
}
