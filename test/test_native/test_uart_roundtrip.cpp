// UART protocol round-trip tests: Nano formatSaveCommand → ESP32-CAM parseCommand,
// and ESP32-CAM response strings → Nano parseResponse.
// Validates: Requirements 1.3, 1.4, 9.4, 11.1–11.4

#include <gtest/gtest.h>
#include <cstring>

// Include Nano-side header for formatSaveCommand and parseResponse.
#include "nano/uart_protocol.h"

// Forward-declare ESP32-CAM types and parseCommand to avoid including
// esp32cam/uart_protocol.h (which redefines UART_MAX_MSG_LEN).
// These definitions are identical to esp32cam/uart_protocol.h.
enum class NanoCommand : uint8_t {
    SCAN, STOP, CAPTURE, SAVE, SHUTDOWN, UNKNOWN
};

struct ParsedCommand {
    NanoCommand command;
    char userId[5];
    char deviceId[3];
    char timestamp[20];
};

extern ParsedCommand parseCommand(const char* line);

// ── Round-trip: Nano formatSaveCommand → ESP32-CAM parseCommand ─

TEST(UartRoundTrip, SaveCommandTypicalValues) {
    char buf[UART_MAX_MSG_LEN + 1];
    uint8_t len = formatSaveCommand(buf, sizeof(buf), "0042", "07", "2025-06-15T14:30:22");
    ASSERT_GT(len, 0u);

    ParsedCommand cmd = parseCommand(buf);
    EXPECT_EQ(cmd.command, NanoCommand::SAVE);
    EXPECT_STREQ(cmd.userId, "0042");
    EXPECT_STREQ(cmd.deviceId, "07");
    EXPECT_STREQ(cmd.timestamp, "2025-06-15T14:30:22");
}

TEST(UartRoundTrip, SaveCommandBoundaryUserIdMin) {
    char buf[UART_MAX_MSG_LEN + 1];
    uint8_t len = formatSaveCommand(buf, sizeof(buf), "0001", "01", "2025-01-01T00:00:00");
    ASSERT_GT(len, 0u);

    ParsedCommand cmd = parseCommand(buf);
    EXPECT_EQ(cmd.command, NanoCommand::SAVE);
    EXPECT_STREQ(cmd.userId, "0001");
    EXPECT_STREQ(cmd.deviceId, "01");
    EXPECT_STREQ(cmd.timestamp, "2025-01-01T00:00:00");
}

TEST(UartRoundTrip, SaveCommandBoundaryUserIdMax) {
    char buf[UART_MAX_MSG_LEN + 1];
    uint8_t len = formatSaveCommand(buf, sizeof(buf), "1000", "31", "2099-12-31T23:59:59");
    ASSERT_GT(len, 0u);

    ParsedCommand cmd = parseCommand(buf);
    EXPECT_EQ(cmd.command, NanoCommand::SAVE);
    EXPECT_STREQ(cmd.userId, "1000");
    EXPECT_STREQ(cmd.deviceId, "31");
    EXPECT_STREQ(cmd.timestamp, "2099-12-31T23:59:59");
}

TEST(UartRoundTrip, SaveCommandDeviceIdBoundaryMin) {
    char buf[UART_MAX_MSG_LEN + 1];
    uint8_t len = formatSaveCommand(buf, sizeof(buf), "0500", "01", "2025-06-01T12:00:00");
    ASSERT_GT(len, 0u);

    ParsedCommand cmd = parseCommand(buf);
    EXPECT_EQ(cmd.command, NanoCommand::SAVE);
    EXPECT_STREQ(cmd.deviceId, "01");
}

TEST(UartRoundTrip, SaveCommandDeviceIdBoundaryMax) {
    char buf[UART_MAX_MSG_LEN + 1];
    uint8_t len = formatSaveCommand(buf, sizeof(buf), "0500", "31", "2025-06-01T12:00:00");
    ASSERT_GT(len, 0u);

    ParsedCommand cmd = parseCommand(buf);
    EXPECT_EQ(cmd.command, NanoCommand::SAVE);
    EXPECT_STREQ(cmd.deviceId, "31");
}

TEST(UartRoundTrip, DeviceIdPreservedThroughRoundTrip) {
    // Verify device ID is faithfully transmitted from Nano to ESP32-CAM
    const char* deviceIds[] = {"01", "07", "16", "25", "31"};
    for (const char* did : deviceIds) {
        char buf[UART_MAX_MSG_LEN + 1];
        uint8_t len = formatSaveCommand(buf, sizeof(buf), "0042", did, "2025-06-15T14:30:22");
        ASSERT_GT(len, 0u) << "formatSaveCommand failed for deviceId=" << did;

        ParsedCommand cmd = parseCommand(buf);
        EXPECT_EQ(cmd.command, NanoCommand::SAVE) << "Parse failed for deviceId=" << did;
        EXPECT_STREQ(cmd.deviceId, did) << "Device ID mismatch for input=" << did;
    }
}

TEST(UartRoundTrip, MaxLengthTimestamp) {
    // ISO 8601 timestamp is 19 chars — verify it survives the round-trip
    char buf[UART_MAX_MSG_LEN + 1];
    uint8_t len = formatSaveCommand(buf, sizeof(buf), "0999", "31", "2099-12-31T23:59:59");
    ASSERT_GT(len, 0u);

    ParsedCommand cmd = parseCommand(buf);
    EXPECT_EQ(cmd.command, NanoCommand::SAVE);
    EXPECT_STREQ(cmd.timestamp, "2099-12-31T23:59:59");
    EXPECT_EQ(strlen(cmd.timestamp), 19u);
}

// ── ESP32-CAM response → Nano parseResponse ─────────────────────

TEST(UartRoundTrip, ResponseInitOk) {
    ParsedResponse resp = parseResponse("RSP:INIT_OK\n");
    EXPECT_EQ(resp.status, EspResponse::INIT_OK);
}

TEST(UartRoundTrip, ResponseQrFound) {
    ParsedResponse resp = parseResponse("RSP:QR:0042\n");
    EXPECT_EQ(resp.status, EspResponse::QR_FOUND);
    EXPECT_STREQ(resp.data, "0042");
}

TEST(UartRoundTrip, ResponseQrFoundBoundaryMin) {
    ParsedResponse resp = parseResponse("RSP:QR:0001\n");
    EXPECT_EQ(resp.status, EspResponse::QR_FOUND);
    EXPECT_STREQ(resp.data, "0001");
}

TEST(UartRoundTrip, ResponseQrFoundBoundaryMax) {
    ParsedResponse resp = parseResponse("RSP:QR:1000\n");
    EXPECT_EQ(resp.status, EspResponse::QR_FOUND);
    EXPECT_STREQ(resp.data, "1000");
}

TEST(UartRoundTrip, ResponseQrInvalid) {
    ParsedResponse resp = parseResponse("RSP:QR_INVALID\n");
    EXPECT_EQ(resp.status, EspResponse::QR_INVALID);
}

TEST(UartRoundTrip, ResponsePhotoReady) {
    ParsedResponse resp = parseResponse("RSP:PHOTO_READY\n");
    EXPECT_EQ(resp.status, EspResponse::PHOTO_READY);
}

TEST(UartRoundTrip, ResponseSaveOk) {
    ParsedResponse resp = parseResponse("RSP:SAVE_OK\n");
    EXPECT_EQ(resp.status, EspResponse::SAVE_OK);
}

TEST(UartRoundTrip, ResponseShutdownReady) {
    ParsedResponse resp = parseResponse("RSP:SHUTDOWN_READY\n");
    EXPECT_EQ(resp.status, EspResponse::SHUTDOWN_READY);
}

TEST(UartRoundTrip, ResponseSdErr) {
    ParsedResponse resp = parseResponse("RSP:SD_ERR\n");
    EXPECT_EQ(resp.status, EspResponse::SD_ERR);
}

TEST(UartRoundTrip, ResponseCamErr) {
    ParsedResponse resp = parseResponse("RSP:CAM_ERR\n");
    EXPECT_EQ(resp.status, EspResponse::CAM_ERR);
}

TEST(UartRoundTrip, ResponseStorageErr) {
    ParsedResponse resp = parseResponse("RSP:STORAGE_ERR\n");
    EXPECT_EQ(resp.status, EspResponse::STORAGE_ERR);
}

TEST(UartRoundTrip, ResponseParseErr) {
    ParsedResponse resp = parseResponse("RSP:ERR:PARSE\n");
    EXPECT_EQ(resp.status, EspResponse::PARSE_ERR);
}

TEST(UartRoundTrip, ResponseWithoutNewline) {
    ParsedResponse resp = parseResponse("RSP:SAVE_OK");
    EXPECT_EQ(resp.status, EspResponse::SAVE_OK);
}

TEST(UartRoundTrip, ResponseUnknownString) {
    ParsedResponse resp = parseResponse("RSP:SOMETHING_ELSE\n");
    EXPECT_EQ(resp.status, EspResponse::UNKNOWN);
}
