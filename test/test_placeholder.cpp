/**
 * Placeholder test to verify the native test environment builds and runs.
 * Real tests are added in subsequent tasks.
 */

#include <gtest/gtest.h>

TEST(Setup, NativeTestEnvironmentWorks) {
  EXPECT_TRUE(true);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
