#include "uart_protocol.h"

#include <string.h>

// ── Pure logic (compiles on native host and Arduino) ────────────

ParsedResponse parseResponse(const char* line) {
    ParsedResponse resp;
    resp.status = EspResponse::UNKNOWN;
    resp.data[0] = '\0';

    if (line == nullptr) {
        return resp;
    }

    // Responses must start with "RSP:"
    if (strncmp(line, "RSP:", 4) != 0) {
        return resp;
    }

    const char* body = line + 4; // skip "RSP:"

    // Strip trailing newline/carriage-return for comparison
    // Work with a local copy so we can trim safely
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

    // Match known response bodies
    if (strcmp(trimmed, "INIT_OK") == 0) {
        resp.status = EspResponse::INIT_OK;
    } else if (strncmp(trimmed, "QR:", 3) == 0) {
        // RSP:QR:<userId>
        resp.status = EspResponse::QR_FOUND;
        const char* payload = trimmed + 3;
        size_t plen = strlen(payload);
        if (plen >= sizeof(resp.data)) {
            plen = sizeof(resp.data) - 1;
        }
        memcpy(resp.data, payload, plen);
        resp.data[plen] = '\0';
    } else if (strcmp(trimmed, "QR_INVALID") == 0) {
        resp.status = EspResponse::QR_INVALID;
    } else if (strcmp(trimmed, "PHOTO_READY") == 0) {
        resp.status = EspResponse::PHOTO_READY;
    } else if (strcmp(trimmed, "SAVE_OK") == 0) {
        resp.status = EspResponse::SAVE_OK;
    } else if (strcmp(trimmed, "SHUTDOWN_READY") == 0) {
        resp.status = EspResponse::SHUTDOWN_READY;
    } else if (strcmp(trimmed, "SD_ERR") == 0) {
        resp.status = EspResponse::SD_ERR;
    } else if (strcmp(trimmed, "CAM_ERR") == 0) {
        resp.status = EspResponse::CAM_ERR;
    } else if (strcmp(trimmed, "STORAGE_ERR") == 0) {
        resp.status = EspResponse::STORAGE_ERR;
    } else if (strcmp(trimmed, "ERR:PARSE") == 0) {
        resp.status = EspResponse::PARSE_ERR;
    }
    // else stays UNKNOWN

    return resp;
}

uint8_t formatSaveCommand(char* buf, uint8_t bufSize,
                          const char* userId, const char* deviceId,
                          const char* timestamp) {
    if (buf == nullptr || userId == nullptr ||
        deviceId == nullptr || timestamp == nullptr) {
        return 0;
    }

    // Build: CMD:SAVE:<userId>:<deviceId>:<timestamp>\n
    // Calculate required length first
    size_t needed = 4 + 4 + 1       // "CMD:" + "SAVE" + ":"
                  + strlen(userId)   // userId
                  + 1                // ":"
                  + strlen(deviceId) // deviceId
                  + 1                // ":"
                  + strlen(timestamp)// timestamp
                  + 1                // "\n"
                  + 1;               // null terminator

    if (needed > bufSize || needed > UART_MAX_MSG_LEN + 1) {
        return 0;
    }

    // Use a simple manual approach to avoid snprintf portability issues
    char* p = buf;
    const char* prefix = "CMD:SAVE:";
    size_t prefixLen = strlen(prefix);
    memcpy(p, prefix, prefixLen);
    p += prefixLen;

    size_t uidLen = strlen(userId);
    memcpy(p, userId, uidLen);
    p += uidLen;

    *p++ = ':';

    size_t didLen = strlen(deviceId);
    memcpy(p, deviceId, didLen);
    p += didLen;

    *p++ = ':';

    size_t tsLen = strlen(timestamp);
    memcpy(p, timestamp, tsLen);
    p += tsLen;

    *p++ = '\n';
    *p = '\0';

    return (uint8_t)(p - buf);
}


// ── Hardware-dependent (Arduino only) ───────────────────────────

#ifdef ARDUINO

void uartInit() {
    Serial.begin(115200);
    while (!Serial) {
        ; // wait for serial port (needed on some boards)
    }
}

static bool readLine(char* buf, uint8_t bufSize, uint16_t timeoutMs) {
    unsigned long start = millis();
    uint8_t idx = 0;
    while ((millis() - start) < timeoutMs) {
        if (Serial.available()) {
            char c = Serial.read();
            if (c == '\n') {
                buf[idx] = '\0';
                return true;
            }
            if (idx < bufSize - 1) {
                buf[idx++] = c;
            }
        }
    }
    buf[idx] = '\0';
    return false;
}

ParsedResponse sendCommand(const char* command) {
    ParsedResponse resp;
    resp.status = EspResponse::TIMEOUT;
    resp.data[0] = '\0';

    for (uint8_t attempt = 0; attempt <= UART_MAX_RETRIES; ++attempt) {
        // Send command with newline
        Serial.print(command);
        Serial.print('\n');
        Serial.flush();

        char lineBuf[UART_MAX_MSG_LEN];
        if (readLine(lineBuf, sizeof(lineBuf), UART_TIMEOUT_MS)) {
            resp = parseResponse(lineBuf);
            if (resp.status != EspResponse::UNKNOWN) {
                return resp;
            }
        }
        // Timeout or unknown — retry
    }

    resp.status = EspResponse::TIMEOUT;
    resp.data[0] = '\0';
    return resp;
}

void sendCommandNoWait(const char* command) {
    Serial.print(command);
    Serial.print('\n');
    Serial.flush();
}

#endif // ARDUINO
