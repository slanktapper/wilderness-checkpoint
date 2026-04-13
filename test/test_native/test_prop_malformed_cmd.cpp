// Feature: wilderness-qr-checkpoint, Property 10: Malformed Command Rejection
//
// **Validates: Requirements 11.5**
//
// For any string that does not match a valid command format
// (CMD:SCAN, CMD:STOP, CMD:CAPTURE, CMD:SAVE:<uid>:<did>:<ts>, CMD:SHUTDOWN),
// parseCommand() shall classify it as NanoCommand::UNKNOWN.

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "esp32cam/uart_protocol.h"

#include <cstring>
#include <string>

// ── Helpers ─────────────────────────────────────────────────────

// Returns true if the string is a valid command that parseCommand()
// should recognise (i.e. NOT UNKNOWN). We use this to filter out
// valid commands from the arbitrary string generator so that only
// genuinely malformed strings reach the property assertion.
//
// IMPORTANT: the filter must not be too aggressive or RapidCheck
// will give up generating values. We only exclude the exact
// well-formed patterns (with optional trailing \n / \r\n).
static bool isWellFormedCommand(const std::string& s) {
    // Work on a copy with trailing whitespace stripped
    std::string trimmed = s;
    while (!trimmed.empty() &&
           (trimmed.back() == '\n' || trimmed.back() == '\r')) {
        trimmed.pop_back();
    }

    // Simple commands
    if (trimmed == "CMD:SCAN")     return true;
    if (trimmed == "CMD:STOP")     return true;
    if (trimmed == "CMD:CAPTURE")  return true;
    if (trimmed == "CMD:SHUTDOWN") return true;

    // CMD:SAVE:<uid>:<did>:<ts>  — any non-empty fields accepted
    // by the parser as SAVE (the parser only checks structure, not
    // field validity for the SAVE command to return NanoCommand::SAVE).
    const std::string savePrefix = "CMD:SAVE:";
    if (trimmed.rfind(savePrefix, 0) == 0) {
        const std::string body = trimmed.substr(savePrefix.size());
        // Need exactly two ':' separators with non-empty segments
        // and each segment must fit in the ParsedCommand buffers:
        //   userId  < 5 chars
        //   deviceId < 3 chars
        //   timestamp < 20 chars
        auto sep1 = body.find(':');
        if (sep1 != std::string::npos && sep1 > 0 && sep1 < 5) {
            auto sep2 = body.find(':', sep1 + 1);
            if (sep2 != std::string::npos) {
                size_t didLen = sep2 - sep1 - 1;
                size_t tsLen  = body.size() - sep2 - 1;
                if (didLen > 0 && didLen < 3 && tsLen > 0 && tsLen < 20) {
                    return true;
                }
            }
        }
    }

    return false;
}

// ── Property Tests ──────────────────────────────────────────────

// Property 10a: Arbitrary strings that are NOT valid commands
// must be classified as UNKNOWN.
RC_GTEST_PROP(MalformedCmdProperty10, ArbitraryNonCommandIsUnknown,
              ()) {
    // Generate an arbitrary string
    auto input = *rc::gen::arbitrary<std::string>();

    // Discard strings that happen to be well-formed commands.
    RC_PRE(!isWellFormedCommand(input));

    ParsedCommand cmd = parseCommand(input.c_str());
    RC_ASSERT(cmd.command == NanoCommand::UNKNOWN);
}

// Property 10b: Strings starting with "RSP:" (responses) must
// never be accepted as commands — they must return UNKNOWN.
RC_GTEST_PROP(MalformedCmdProperty10, ResponsePrefixIsUnknown,
              ()) {
    // Generate an arbitrary suffix after "RSP:"
    auto suffix = *rc::gen::arbitrary<std::string>();
    std::string input = "RSP:" + suffix;

    ParsedCommand cmd = parseCommand(input.c_str());
    RC_ASSERT(cmd.command == NanoCommand::UNKNOWN);
}
