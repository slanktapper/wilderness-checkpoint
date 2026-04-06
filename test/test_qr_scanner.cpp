/**
 * Property-based tests for QR Scanner User_ID validation (Property 1).
 *
 * Property 1: User_ID validation accepts only correctly formatted IDs
 * For any string, isValidUserId() returns true iff the string is exactly
 * 4 characters long, consists entirely of digits, and represents a number
 * in the range 0001–1000. All other strings are rejected.
 *
 * **Validates: Requirements 2.2**
 */

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "qr_scanner.h"

#include <string>
#include <sstream>
#include <iomanip>

class UserIdValidationPropertyTest : public ::testing::Test {};

/**
 * Property 1a: Valid IDs (numbers 1–1000, zero-padded to 4 digits) are accepted.
 *
 * **Validates: Requirements 2.2**
 */
RC_GTEST_FIXTURE_PROP(UserIdValidationPropertyTest,
                       ValidIdsAreAccepted,
                       ()) {
    // Generate a number in the valid range [1, 1000]
    const auto num = *rc::gen::inRange(1, 1001);

    // Zero-pad to 4 digits
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << num;
    String id = oss.str();

    RC_ASSERT(QRScanner::isValidUserId(id));
}

/**
 * Property 1b: Arbitrary strings are accepted iff they match the exact format.
 *
 * **Validates: Requirements 2.2**
 */
RC_GTEST_FIXTURE_PROP(UserIdValidationPropertyTest,
                       ArbitraryStringsMatchFormat,
                       ()) {
    const auto str = *rc::gen::arbitrary<std::string>();
    String id = str;

    // Compute expected result: exactly 4 digits, numeric value in [1, 1000]
    bool expected = false;
    if (str.length() == 4) {
        bool allDigits = true;
        for (char c : str) {
            if (c < '0' || c > '9') {
                allDigits = false;
                break;
            }
        }
        if (allDigits) {
            int val = std::stoi(str);
            expected = (val >= 1 && val <= 1000);
        }
    }

    RC_ASSERT(QRScanner::isValidUserId(id) == expected);
}


/**
 * Property 1c: Out-of-range numbers (0, 1001+) formatted as 4-digit strings
 * are rejected.
 *
 * **Validates: Requirements 2.2**
 */
RC_GTEST_FIXTURE_PROP(UserIdValidationPropertyTest,
                       OutOfRangeNumbersRejected,
                       ()) {
    // Generate a number that is either 0 or in [1001, 9999]
    const auto num = *rc::gen::oneOf(
        rc::gen::just(0),
        rc::gen::inRange(1001, 10000)
    );

    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << num;
    String id = oss.str();

    RC_ASSERT(!QRScanner::isValidUserId(id));
}

/**
 * Property 1d: Wrong-length strings (not exactly 4 characters) are rejected.
 *
 * **Validates: Requirements 2.2**
 */
RC_GTEST_FIXTURE_PROP(UserIdValidationPropertyTest,
                       WrongLengthStringsRejected,
                       ()) {
    // Generate a string whose length is NOT 4
    auto str = *rc::gen::arbitrary<std::string>();
    RC_PRE(str.length() != 4);

    String id = str;

    RC_ASSERT(!QRScanner::isValidUserId(id));
}

// ─── Unit Tests for User_ID Validation Edge Cases (Task 3.3) ────────────────
// **Validates: Requirements 2.2, 2.3**

// --- Known-good IDs ---

TEST(UserIdValidationEdgeCases, AcceptsLowerBound0001) {
    EXPECT_TRUE(QRScanner::isValidUserId("0001"));
}

TEST(UserIdValidationEdgeCases, AcceptsMidRange0500) {
    EXPECT_TRUE(QRScanner::isValidUserId("0500"));
}

TEST(UserIdValidationEdgeCases, AcceptsUpperBound1000) {
    EXPECT_TRUE(QRScanner::isValidUserId("1000"));
}

// --- Known-bad inputs ---

TEST(UserIdValidationEdgeCases, RejectsEmptyString) {
    EXPECT_FALSE(QRScanner::isValidUserId(""));
}

TEST(UserIdValidationEdgeCases, RejectsZero0000) {
    EXPECT_FALSE(QRScanner::isValidUserId("0000"));
}

TEST(UserIdValidationEdgeCases, RejectsAboveRange1001) {
    EXPECT_FALSE(QRScanner::isValidUserId("1001"));
}

TEST(UserIdValidationEdgeCases, RejectsAlphabeticAbcd) {
    EXPECT_FALSE(QRScanner::isValidUserId("abcd"));
}

TEST(UserIdValidationEdgeCases, RejectsTooLong00001) {
    EXPECT_FALSE(QRScanner::isValidUserId("00001"));
}

TEST(UserIdValidationEdgeCases, RejectsTooShort1) {
    EXPECT_FALSE(QRScanner::isValidUserId("1"));
}

TEST(UserIdValidationEdgeCases, RejectsThreeDigit999) {
    EXPECT_FALSE(QRScanner::isValidUserId("999"));
}

TEST(UserIdValidationEdgeCases, RejectsLeadingSpace042) {
    EXPECT_FALSE(QRScanner::isValidUserId(" 042"));
}
