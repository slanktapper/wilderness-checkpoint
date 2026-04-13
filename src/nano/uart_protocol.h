#ifndef NANO_UART_PROTOCOL_H
#define NANO_UART_PROTOCOL_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#include <cstddef>
#endif

// Maximum UART message length including newline
static const uint8_t UART_MAX_MSG_LEN = 64;

// Command timeout in milliseconds
static const uint16_t UART_TIMEOUT_MS = 500;

// Maximum retry count for failed commands
static const uint8_t UART_MAX_RETRIES = 2;

// Response status codes from ESP32-CAM
enum class EspResponse : uint8_t {
    INIT_OK,
    QR_FOUND,        // Followed by :YYYY user_id
    QR_INVALID,
    PHOTO_READY,
    SAVE_OK,
    SHUTDOWN_READY,
    SD_ERR,
    CAM_ERR,
    STORAGE_ERR,
    PARSE_ERR,
    TIMEOUT,         // Internal: no response received
    UNKNOWN          // Internal: unrecognized response
};

struct ParsedResponse {
    EspResponse status;
    char data[16];   // Optional payload (e.g., user_id "0042")
};

// Parse a raw response line into a ParsedResponse struct.
// Pure logic — works on native host without Arduino.
ParsedResponse parseResponse(const char* line);

// Build a CMD:SAVE:<userId>:<deviceId>:<timestamp>\n string.
// Returns the number of characters written (excluding null terminator),
// or 0 if the buffer is too small.
// Pure logic — works on native host without Arduino.
uint8_t formatSaveCommand(char* buf, uint8_t bufSize,
                          const char* userId, const char* deviceId,
                          const char* timestamp);

#ifdef ARDUINO
// Initialize UART at 115200 baud.
void uartInit();

// Send a command and wait for response with retries.
// Returns parsed response. On timeout after all retries, returns TIMEOUT.
// command: full command string without newline (e.g., "CMD:SCAN")
ParsedResponse sendCommand(const char* command);

// Send a command without waiting for response (used for CMD:STOP).
void sendCommandNoWait(const char* command);
#endif

#endif // NANO_UART_PROTOCOL_H
