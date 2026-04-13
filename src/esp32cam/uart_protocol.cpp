#include "uart_protocol.h"

#include <string.h>

// ── Pure logic (compiles on native host and Arduino) ────────────

ParsedCommand parseCommand(const char* line) {
    ParsedCommand cmd;
    cmd.command = NanoCommand::UNKNOWN;
    cmd.userId[0] = '\0';
    cmd.deviceId[0] = '\0';
    cmd.timestamp[0] = '\0';

    if (line == nullptr) {
        return cmd;
    }

    // Commands must start with "CMD:"
    if (strncmp(line, "CMD:", 4) != 0) {
        return cmd;
    }

    const char* body = line + 4; // skip "CMD:"

    // Work with a trimmed copy to handle trailing \n and \r
    char trimmed[UART_MAX_MSG_LEN];
    size_t len = strlen(body);
    if (len >= sizeof(trimmed)) {
        len = sizeof(trimmed) - 1;
    }
    memcpy(trimmed, body, len);
    trimmed[len] = '\0';

    // Trim trailing \n and \r
    while (len > 0 && (trimmed[len - 1] == '\n' || trimmed[len - 1] == '\r')) {
        trimmed[--len] = '\0';
    }

    // Match simple commands
    if (strcmp(trimmed, "SCAN") == 0) {
        cmd.command = NanoCommand::SCAN;
        return cmd;
    }
    if (strcmp(trimmed, "STOP") == 0) {
        cmd.command = NanoCommand::STOP;
        return cmd;
    }
    if (strcmp(trimmed, "CAPTURE") == 0) {
        cmd.command = NanoCommand::CAPTURE;
        return cmd;
    }
    if (strcmp(trimmed, "SHUTDOWN") == 0) {
        cmd.command = NanoCommand::SHUTDOWN;
        return cmd;
    }

    // Check for SAVE:<userId>:<deviceId>:<timestamp>
    if (strncmp(trimmed, "SAVE:", 5) != 0) {
        return cmd; // UNKNOWN
    }

    const char* saveBody = trimmed + 5; // skip "SAVE:"

    // Parse userId (up to first ':')
    const char* sep1 = strchr(saveBody, ':');
    if (sep1 == nullptr) {
        return cmd; // UNKNOWN — missing deviceId
    }
    size_t uidLen = (size_t)(sep1 - saveBody);
    if (uidLen == 0 || uidLen >= sizeof(cmd.userId)) {
        return cmd; // UNKNOWN — userId too long or empty
    }
    memcpy(cmd.userId, saveBody, uidLen);
    cmd.userId[uidLen] = '\0';

    // Parse deviceId (between first and second ':')
    const char* afterSep1 = sep1 + 1;
    const char* sep2 = strchr(afterSep1, ':');
    if (sep2 == nullptr) {
        cmd.userId[0] = '\0';
        return cmd; // UNKNOWN — missing timestamp
    }
    size_t didLen = (size_t)(sep2 - afterSep1);
    if (didLen == 0 || didLen >= sizeof(cmd.deviceId)) {
        cmd.userId[0] = '\0';
        return cmd; // UNKNOWN — deviceId too long or empty
    }
    memcpy(cmd.deviceId, afterSep1, didLen);
    cmd.deviceId[didLen] = '\0';

    // Parse timestamp (everything after second ':')
    const char* tsStart = sep2 + 1;
    size_t tsLen = strlen(tsStart);
    if (tsLen == 0 || tsLen >= sizeof(cmd.timestamp)) {
        cmd.userId[0] = '\0';
        cmd.deviceId[0] = '\0';
        return cmd; // UNKNOWN — timestamp too long or empty
    }
    memcpy(cmd.timestamp, tsStart, tsLen);
    cmd.timestamp[tsLen] = '\0';

    cmd.command = NanoCommand::SAVE;
    return cmd;
}

bool isValidUserId(const char* userId) {
    if (userId == nullptr) {
        return false;
    }

    // Must be exactly 4 characters
    size_t len = strlen(userId);
    if (len != 4) {
        return false;
    }

    // All characters must be ASCII digits
    for (size_t i = 0; i < 4; ++i) {
        if (userId[i] < '0' || userId[i] > '9') {
            return false;
        }
    }

    // Numeric value must be 1–1000
    // Manual conversion to avoid atoi portability concerns
    int value = (userId[0] - '0') * 1000
              + (userId[1] - '0') * 100
              + (userId[2] - '0') * 10
              + (userId[3] - '0');

    return (value >= 1 && value <= 1000);
}


// ── Hardware-dependent (Arduino only) ───────────────────────────

#ifdef ARDUINO

void uartInit() {
    Serial.begin(115200);
    while (!Serial) {
        ; // wait for serial port
    }
}

void sendResponse(const char* response) {
    if (response == nullptr) {
        return;
    }
    Serial.print(response);
    Serial.print('\n');
    Serial.flush();
}

bool readCommand(char* buffer, uint8_t bufferSize, uint16_t timeoutMs) {
    if (buffer == nullptr || bufferSize == 0) {
        return false;
    }

    unsigned long start = millis();
    uint8_t idx = 0;

    while ((millis() - start) < timeoutMs) {
        if (Serial.available()) {
            char c = Serial.read();
            if (c == '\n') {
                buffer[idx] = '\0';
                return true;
            }
            if (idx < bufferSize - 1) {
                buffer[idx++] = c;
            }
        }
    }

    buffer[idx] = '\0';
    return false;
}

#endif // ARDUINO
