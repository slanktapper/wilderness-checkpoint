// Unit tests for file path helper functions
// Validates: Requirements 5.2, 6.2, 6.3

#include <gtest/gtest.h>
#include "esp32cam/file_paths.h"
#include <cstring>

// ── buildPhotoPath ──────────────────────────────────────────────

TEST(FilePathsTest, BuildPhotoPathTypicalInput) {
    char buf[64];
    uint8_t len = buildPhotoPath(buf, sizeof(buf), "07", "0042");
    EXPECT_STREQ(buf, "/DEVICE07/DEVICE07_USER0042.jpg");
    EXPECT_EQ(len, std::strlen("/DEVICE07/DEVICE07_USER0042.jpg"));
}

TEST(FilePathsTest, BuildPhotoPathDeviceId01UserId0001) {
    char buf[64];
    uint8_t len = buildPhotoPath(buf, sizeof(buf), "01", "0001");
    EXPECT_STREQ(buf, "/DEVICE01/DEVICE01_USER0001.jpg");
    EXPECT_EQ(len, 31u);
}

TEST(FilePathsTest, BuildPhotoPathDeviceId31UserId1000) {
    char buf[64];
    uint8_t len = buildPhotoPath(buf, sizeof(buf), "31", "1000");
    EXPECT_STREQ(buf, "/DEVICE31/DEVICE31_USER1000.jpg");
    EXPECT_EQ(len, 31u);
}

TEST(FilePathsTest, BuildPhotoPathBufferTooSmall) {
    char buf[10];
    uint8_t len = buildPhotoPath(buf, sizeof(buf), "07", "0042");
    EXPECT_EQ(len, 0u);
    EXPECT_EQ(buf[0], '\0');
}

TEST(FilePathsTest, BuildPhotoPathExactFit) {
    // "/DEVICE07/DEVICE07_USER0042.jpg" = 31 chars + null = 32 bytes
    char buf[32];
    uint8_t len = buildPhotoPath(buf, sizeof(buf), "07", "0042");
    EXPECT_EQ(len, 31u);
    EXPECT_STREQ(buf, "/DEVICE07/DEVICE07_USER0042.jpg");
}

TEST(FilePathsTest, BuildPhotoPathOneByteShort) {
    char buf[31]; // needs 32
    uint8_t len = buildPhotoPath(buf, sizeof(buf), "07", "0042");
    EXPECT_EQ(len, 0u);
    EXPECT_EQ(buf[0], '\0');
}

TEST(FilePathsTest, BuildPhotoPathNullBuffer) {
    uint8_t len = buildPhotoPath(nullptr, 64, "07", "0042");
    EXPECT_EQ(len, 0u);
}

TEST(FilePathsTest, BuildPhotoPathNullDeviceId) {
    char buf[64];
    uint8_t len = buildPhotoPath(buf, sizeof(buf), nullptr, "0042");
    EXPECT_EQ(len, 0u);
}

TEST(FilePathsTest, BuildPhotoPathNullUserId) {
    char buf[64];
    uint8_t len = buildPhotoPath(buf, sizeof(buf), "07", nullptr);
    EXPECT_EQ(len, 0u);
}

// ── buildLogPath ────────────────────────────────────────────────

TEST(FilePathsTest, BuildLogPathTypicalInput) {
    char buf[64];
    uint8_t len = buildLogPath(buf, sizeof(buf), "07");
    EXPECT_STREQ(buf, "/DEVICE07/scan_log.csv");
    EXPECT_EQ(len, std::strlen("/DEVICE07/scan_log.csv"));
}

TEST(FilePathsTest, BuildLogPathDeviceId01) {
    char buf[64];
    uint8_t len = buildLogPath(buf, sizeof(buf), "01");
    EXPECT_STREQ(buf, "/DEVICE01/scan_log.csv");
    EXPECT_EQ(len, 22u);
}

TEST(FilePathsTest, BuildLogPathDeviceId31) {
    char buf[64];
    uint8_t len = buildLogPath(buf, sizeof(buf), "31");
    EXPECT_STREQ(buf, "/DEVICE31/scan_log.csv");
    EXPECT_EQ(len, 22u);
}

TEST(FilePathsTest, BuildLogPathBufferTooSmall) {
    char buf[10];
    uint8_t len = buildLogPath(buf, sizeof(buf), "07");
    EXPECT_EQ(len, 0u);
    EXPECT_EQ(buf[0], '\0');
}

TEST(FilePathsTest, BuildLogPathExactFit) {
    // "/DEVICE07/scan_log.csv" = 22 chars + null = 23 bytes
    char buf[23];
    uint8_t len = buildLogPath(buf, sizeof(buf), "07");
    EXPECT_EQ(len, 22u);
    EXPECT_STREQ(buf, "/DEVICE07/scan_log.csv");
}

TEST(FilePathsTest, BuildLogPathNullBuffer) {
    uint8_t len = buildLogPath(nullptr, 64, "07");
    EXPECT_EQ(len, 0u);
}

TEST(FilePathsTest, BuildLogPathNullDeviceId) {
    char buf[64];
    uint8_t len = buildLogPath(buf, sizeof(buf), nullptr);
    EXPECT_EQ(len, 0u);
}

// ── buildDirPath ────────────────────────────────────────────────

TEST(FilePathsTest, BuildDirPathTypicalInput) {
    char buf[64];
    uint8_t len = buildDirPath(buf, sizeof(buf), "07");
    EXPECT_STREQ(buf, "/DEVICE07");
    EXPECT_EQ(len, std::strlen("/DEVICE07"));
}

TEST(FilePathsTest, BuildDirPathDeviceId01) {
    char buf[64];
    uint8_t len = buildDirPath(buf, sizeof(buf), "01");
    EXPECT_STREQ(buf, "/DEVICE01");
    EXPECT_EQ(len, 9u);
}

TEST(FilePathsTest, BuildDirPathDeviceId31) {
    char buf[64];
    uint8_t len = buildDirPath(buf, sizeof(buf), "31");
    EXPECT_STREQ(buf, "/DEVICE31");
    EXPECT_EQ(len, 9u);
}

TEST(FilePathsTest, BuildDirPathBufferTooSmall) {
    char buf[5];
    uint8_t len = buildDirPath(buf, sizeof(buf), "07");
    EXPECT_EQ(len, 0u);
    EXPECT_EQ(buf[0], '\0');
}

TEST(FilePathsTest, BuildDirPathExactFit) {
    // "/DEVICE07" = 9 chars + null = 10 bytes
    char buf[10];
    uint8_t len = buildDirPath(buf, sizeof(buf), "07");
    EXPECT_EQ(len, 9u);
    EXPECT_STREQ(buf, "/DEVICE07");
}

TEST(FilePathsTest, BuildDirPathNullBuffer) {
    uint8_t len = buildDirPath(nullptr, 64, "07");
    EXPECT_EQ(len, 0u);
}

TEST(FilePathsTest, BuildDirPathNullDeviceId) {
    char buf[64];
    uint8_t len = buildDirPath(buf, sizeof(buf), nullptr);
    EXPECT_EQ(len, 0u);
}

// ── formatCsvRow ────────────────────────────────────────────────
// Validates: Requirements 5.1, 5.3, 5.4

TEST(CsvFormatTest, FormatCsvRowTypicalInput) {
    char buf[64];
    uint8_t len = formatCsvRow(buf, sizeof(buf), "0042", "07", "2025-06-15T14:30:22");
    EXPECT_STREQ(buf, "0042,07,2025-06-15T14:30:22\n");
    EXPECT_EQ(len, std::strlen("0042,07,2025-06-15T14:30:22\n"));
}

TEST(CsvFormatTest, FormatCsvRowBoundaryUserId0001) {
    char buf[64];
    uint8_t len = formatCsvRow(buf, sizeof(buf), "0001", "01", "2025-01-01T00:00:00");
    EXPECT_STREQ(buf, "0001,01,2025-01-01T00:00:00\n");
    EXPECT_EQ(len, 28u);
}

TEST(CsvFormatTest, FormatCsvRowBoundaryUserId1000DeviceId31) {
    char buf[64];
    uint8_t len = formatCsvRow(buf, sizeof(buf), "1000", "31", "2099-12-31T23:59:59");
    EXPECT_STREQ(buf, "1000,31,2099-12-31T23:59:59\n");
    EXPECT_EQ(len, 28u);
}

TEST(CsvFormatTest, FormatCsvRowBufferTooSmall) {
    char buf[10];
    uint8_t len = formatCsvRow(buf, sizeof(buf), "0042", "07", "2025-06-15T14:30:22");
    EXPECT_EQ(len, 0u);
    EXPECT_EQ(buf[0], '\0');
}

TEST(CsvFormatTest, FormatCsvRowExactFit) {
    // "0042,07,2025-06-15T14:30:22\n" = 28 chars + null = 29 bytes
    char buf[29];
    uint8_t len = formatCsvRow(buf, sizeof(buf), "0042", "07", "2025-06-15T14:30:22");
    EXPECT_EQ(len, 28u);
    EXPECT_STREQ(buf, "0042,07,2025-06-15T14:30:22\n");
}

TEST(CsvFormatTest, FormatCsvRowOneByteShort) {
    char buf[28]; // needs 29
    uint8_t len = formatCsvRow(buf, sizeof(buf), "0042", "07", "2025-06-15T14:30:22");
    EXPECT_EQ(len, 0u);
    EXPECT_EQ(buf[0], '\0');
}

TEST(CsvFormatTest, FormatCsvRowNullBuffer) {
    uint8_t len = formatCsvRow(nullptr, 64, "0042", "07", "2025-06-15T14:30:22");
    EXPECT_EQ(len, 0u);
}

TEST(CsvFormatTest, FormatCsvRowNullUserId) {
    char buf[64];
    uint8_t len = formatCsvRow(buf, sizeof(buf), nullptr, "07", "2025-06-15T14:30:22");
    EXPECT_EQ(len, 0u);
}

TEST(CsvFormatTest, FormatCsvRowNullDeviceId) {
    char buf[64];
    uint8_t len = formatCsvRow(buf, sizeof(buf), "0042", nullptr, "2025-06-15T14:30:22");
    EXPECT_EQ(len, 0u);
}

TEST(CsvFormatTest, FormatCsvRowNullTimestamp) {
    char buf[64];
    uint8_t len = formatCsvRow(buf, sizeof(buf), "0042", "07", nullptr);
    EXPECT_EQ(len, 0u);
}

TEST(CsvFormatTest, FormatCsvRowColumnOrder) {
    // Verify column order: userId comes first, then deviceId, then timestamp
    char buf[64];
    formatCsvRow(buf, sizeof(buf), "AAAA", "BB", "CCCCCCCCCCCCCCCCCC");
    // Should be "AAAA,BB,CCCCCCCCCCCCCCCCCC\n"
    EXPECT_EQ(std::string(buf).find("AAAA"), 0u);
    EXPECT_TRUE(std::string(buf).find("AAAA") < std::string(buf).find("BB"));
    EXPECT_TRUE(std::string(buf).find("BB") < std::string(buf).find("CCCCCCCCCCCCCCCCCC"));
}

// ── csvHeader ───────────────────────────────────────────────────

TEST(CsvFormatTest, CsvHeaderContent) {
    const char* header = csvHeader();
    EXPECT_STREQ(header, "user_id,device_id,timestamp\n");
}

TEST(CsvFormatTest, CsvHeaderEndsWithNewline) {
    const char* header = csvHeader();
    size_t len = std::strlen(header);
    EXPECT_GT(len, 0u);
    EXPECT_EQ(header[len - 1], '\n');
}

TEST(CsvFormatTest, CsvHeaderColumnNames) {
    const char* header = csvHeader();
    std::string h(header);
    // Verify column order matches CSV format spec
    EXPECT_TRUE(h.find("user_id") < h.find("device_id"));
    EXPECT_TRUE(h.find("device_id") < h.find("timestamp"));
}
