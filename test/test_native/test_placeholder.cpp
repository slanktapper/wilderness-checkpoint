#include <gtest/gtest.h>

// Trivial test to verify the native Google Test infrastructure works.
TEST(PlaceholderTest, TrivialPass) {
    EXPECT_EQ(1 + 1, 2);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
