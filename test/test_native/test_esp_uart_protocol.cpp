#include <gtest/gtest.h>
#include "esp32cam/uart_protocol.h"

// ── parseCommand tests ──────────────────────────────────────────

TEST(EspParseCommand, ScanCommand) {
    ParsedCommand cmd = parseCommand("CMD:SCAN\n");
    EXPECT_EQ(cmd.command, NanoCommand::SCAN);
}

TEST(EspParseCommand, ScanCommandNoNewline) {
    ParsedCommand cmd = parseCommand("CMD:SCAN");
    EXPECT_EQ(cmd.command, NanoCommand::SCAN);
}

TEST(EspParseCommand, StopCommand) {
    ParsedCommand cmd = parseCommand("CMD:STOP\n");
    EXPECT_EQ(cmd.command, NanoCommand::STOP);
}

TEST(EspParseCommand, CaptureCommand) {
    ParsedCommand cmd = parseCommand("CMD:CAPTURE\n");
    EXPECT_EQ(cmd.command, NanoCommand::CAPTURE);
}

TEST(EspParseCommand, ShutdownCommand) {
    ParsedCommand cmd = parseCommand("CMD:SHUTDOWN\n");
    EXPECT_EQ(cmd.command, NanoCommand::SHUTDOWN);
}

TEST(EspParseCommand, SaveCommandFull) {
    ParsedCommand cmd = parseCommand("CMD:SAVE:0042:07:2025-06-15T14:30:22\n");
    EXPECT_EQ(cmd.command, NanoCommand::SAVE);
    EXPECT_STREQ(cmd.userId, "0042");
    EXPECT_STREQ(cmd.deviceId, "07");
    EXPECT_STREQ(cmd.timestamp, "2025-06-15T14:30:22");
}

TEST(EspParseCommand, SaveCommandNoNewline) {
    ParsedCommand cmd = parseCommand("CMD:SAVE:0001:31:2025-01-01T00:00:00");
    EXPECT_EQ(cmd.command, NanoCommand::SAVE);
    EXPECT_STREQ(cmd.userId, "0001");
    EXPECT_STREQ(cmd.deviceId, "31");
    EXPECT_STREQ(cmd.timestamp, "2025-01-01T00:00:00");
}

TEST(EspParseCommand, SaveCommandBoundaryUserId1000) {
    ParsedCommand cmd = parseCommand("CMD:SAVE:1000:01:2025-12-31T23:59:59\n");
    EXPECT_EQ(cmd.command, NanoCommand::SAVE);
    EXPECT_STREQ(cmd.userId, "1000");
    EXPECT_STREQ(cmd.deviceId, "01");
    EXPECT_STREQ(cmd.timestamp, "2025-12-31T23:59:59");
}

TEST(EspParseCommand, NullInput) {
    ParsedCommand cmd = parseCommand(nullptr);
    EXPECT_EQ(cmd.command, NanoCommand::UNKNOWN);
}

TEST(EspParseCommand, EmptyString) {
    ParsedCommand cmd = parseCommand("");
    EXPECT_EQ(cmd.command, NanoCommand::UNKNOWN);
}

TEST(EspParseCommand, GarbageInput) {
    ParsedCommand cmd = parseCommand("HELLO WORLD");
    EXPECT_EQ(cmd.command, NanoCommand::UNKNOWN);
}

TEST(EspParseCommand, ResponseNotCommand) {
    ParsedCommand cmd = parseCommand("RSP:INIT_OK\n");
    EXPECT_EQ(cmd.command, NanoCommand::UNKNOWN);
}

TEST(EspParseCommand, SaveMissingDeviceId) {
    ParsedCommand cmd = parseCommand("CMD:SAVE:0042\n");
    EXPECT_EQ(cmd.command, NanoCommand::UNKNOWN);
}

TEST(EspParseCommand, SaveMissingTimestamp) {
    ParsedCommand cmd = parseCommand("CMD:SAVE:0042:07\n");
    EXPECT_EQ(cmd.command, NanoCommand::UNKNOWN);
}

TEST(EspParseCommand, SaveEmptyUserId) {
    ParsedCommand cmd = parseCommand("CMD:SAVE::07:2025-06-15T14:30:22\n");
    EXPECT_EQ(cmd.command, NanoCommand::UNKNOWN);
}

TEST(EspParseCommand, CarriageReturnHandling) {
    ParsedCommand cmd = parseCommand("CMD:SCAN\r\n");
    EXPECT_EQ(cmd.command, NanoCommand::SCAN);
}

// ── isValidUserId tests ─────────────────────────────────────────

TEST(IsValidUserId, ValidMin) {
    EXPECT_TRUE(isValidUserId("0001"));
}

TEST(IsValidUserId, ValidMax) {
    EXPECT_TRUE(isValidUserId("1000"));
}

TEST(IsValidUserId, ValidMid) {
    EXPECT_TRUE(isValidUserId("0042"));
    EXPECT_TRUE(isValidUserId("0500"));
    EXPECT_TRUE(isValidUserId("0999"));
}

TEST(IsValidUserId, InvalidZero) {
    EXPECT_FALSE(isValidUserId("0000"));
}

TEST(IsValidUserId, InvalidAboveMax) {
    EXPECT_FALSE(isValidUserId("1001"));
    EXPECT_FALSE(isValidUserId("9999"));
}

TEST(IsValidUserId, TooShort) {
    EXPECT_FALSE(isValidUserId("42"));
    EXPECT_FALSE(isValidUserId("001"));
}

TEST(IsValidUserId, TooLong) {
    EXPECT_FALSE(isValidUserId("00042"));
}

TEST(IsValidUserId, NonDigits) {
    EXPECT_FALSE(isValidUserId("abcd"));
    EXPECT_FALSE(isValidUserId("00a1"));
    EXPECT_FALSE(isValidUserId("12 4"));
}

TEST(IsValidUserId, EmptyString) {
    EXPECT_FALSE(isValidUserId(""));
}

TEST(IsValidUserId, NullInput) {
    EXPECT_FALSE(isValidUserId(nullptr));
}
