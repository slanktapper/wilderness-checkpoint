#ifndef ESP_UART_PROTOCOL_H
#define ESP_UART_PROTOCOL_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#include <cstddef>
#endif

// Maximum UART message length including newline (matches Nano side)
static const uint8_t UART_MAX_MSG_LEN = 64;

// Parsed command types from Nano
enum class NanoCommand : uint8_t {
    SCAN,
    STOP,
    CAPTURE,
    SAVE,       // Followed by :user_id:device_id:timestamp
    SHUTDOWN,
    UNKNOWN
};

struct ParsedCommand {
    NanoCommand command;
    char userId[5];      // "0001"-"1000", null-terminated
    char deviceId[3];    // "01"-"31", null-terminated
    char timestamp[20];  // "YYYY-MM-DDTHH:MM:SS", null-terminated
};

// ── Pure logic (compiles on native host and Arduino) ────────────

// Parse a raw command line into a ParsedCommand struct.
// Handles "CMD:SCAN\n", "CMD:STOP\n", "CMD:CAPTURE\n",
// "CMD:SAVE:<userId>:<deviceId>:<timestamp>\n", "CMD:SHUTDOWN\n".
// Anything else returns NanoCommand::UNKNOWN.
ParsedCommand parseCommand(const char* line);

// Validate a user_id string: exactly 4 ASCII digits, value 0001-1000.
bool isValidUserId(const char* userId);

// ── Hardware-dependent (Arduino only) ───────────────────────────

#ifdef ARDUINO
// Initialize UART at 115200 baud (Serial on GPIO1/GPIO3).
void uartInit();

// Send a response line. Appends newline automatically.
// Example: sendResponse("RSP:INIT_OK")
void sendResponse(const char* response);

// Read a complete command line (blocking with timeout).
// Returns true if a line was read, false on timeout.
bool readCommand(char* buffer, uint8_t bufferSize, uint16_t timeoutMs);
#endif

#endif // ESP_UART_PROTOCOL_H
