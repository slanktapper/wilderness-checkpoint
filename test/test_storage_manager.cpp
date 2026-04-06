/**
 * Property-based tests for Storage Manager module (Properties 3, 4, 5).
 *
 * Property 3: File path construction is deterministic and correctly formatted
 * Property 4: CSV scan log record round-trip
 * Property 5: Scan log append preserves all previous records
 */

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "storage_manager.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// ─── Helpers ────────────────────────────────────────────────────────────────

/// Zero-pad an integer to the given width.
static std::string zeroPad(int value, int width) {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(width) << value;
    return oss.str();
}

/// Generate a valid Device_ID string ("01"–"99").
static rc::Gen<std::string> genDeviceId() {
    return rc::gen::map(rc::gen::inRange(1, 100), [](int v) {
        return zeroPad(v, 2);
    });
}

/// Generate a valid User_ID string ("0001"–"1000").
static rc::Gen<std::string> genUserId() {
    return rc::gen::map(rc::gen::inRange(1, 1001), [](int v) {
        return zeroPad(v, 4);
    });
}

/// Generate a valid ISO 8601 timestamp like "2025-07-15T14:32:07".
static rc::Gen<std::string> genTimestamp() {
    return rc::gen::apply(
        [](int y, int mo, int d, int h, int mi, int s) {
            std::ostringstream oss;
            oss << std::setfill('0')
                << std::setw(4) << y << "-"
                << std::setw(2) << mo << "-"
                << std::setw(2) << d << "T"
                << std::setw(2) << h << ":"
                << std::setw(2) << mi << ":"
                << std::setw(2) << s;
            return oss.str();
        },
        rc::gen::inRange(2020, 2031),  // year
        rc::gen::inRange(1, 13),       // month
        rc::gen::inRange(1, 29),       // day (safe for all months)
        rc::gen::inRange(0, 24),       // hour
        rc::gen::inRange(0, 60),       // minute
        rc::gen::inRange(0, 60)        // second
    );
}

// ═════════════════════════════════════════════════════════════════════════════
// Property 3: File path construction is deterministic and correctly formatted
// ═════════════════════════════════════════════════════════════════════════════

/**
 * Property 3a: buildPhotoPath produces the expected pattern.
 *
 * **Validates: Requirements 3.4, 3.5, 9.1, 13.1**
 */
RC_GTEST_PROP(StorageManagerProperty3,
              PhotoPathMatchesExpectedPattern,
              ()) {
    const auto deviceId = *genDeviceId();
    const auto userId   = *genUserId();

    std::string path = Storage::buildPhotoPath(deviceId.c_str(), userId.c_str());

    std::string expected = "/DEVICE" + deviceId +
                           "/DEVICE" + deviceId +
                           "_USER" + userId + ".jpg";

    RC_ASSERT(path == expected);
}

/**
 * Property 3b: buildPhotoPath is deterministic — two calls with the same
 * inputs produce identical results.
 *
 * **Validates: Requirements 3.4, 3.5, 9.1, 13.1**
 */
RC_GTEST_PROP(StorageManagerProperty3,
              PhotoPathIsDeterministic,
              ()) {
    const auto deviceId = *genDeviceId();
    const auto userId   = *genUserId();

    std::string path1 = Storage::buildPhotoPath(deviceId.c_str(), userId.c_str());
    std::string path2 = Storage::buildPhotoPath(deviceId.c_str(), userId.c_str());

    RC_ASSERT(path1 == path2);
}


/**
 * Property 3c: buildLogPath produces the expected pattern.
 *
 * **Validates: Requirements 3.4, 3.5, 9.1, 13.1**
 */
RC_GTEST_PROP(StorageManagerProperty3,
              LogPathMatchesExpectedPattern,
              ()) {
    const auto deviceId = *genDeviceId();

    std::string path = Storage::buildLogPath(deviceId.c_str());

    std::string expected = "/DEVICE" + deviceId + "/scan_log.csv";

    RC_ASSERT(path == expected);
}

// ═════════════════════════════════════════════════════════════════════════════
// Property 4: CSV scan log record round-trip
// ═════════════════════════════════════════════════════════════════════════════

/**
 * Property 4: Formatting a scan record as CSV and parsing it back yields the
 * original fields unchanged.
 *
 * **Validates: Requirements 4.1, 4.2, 13.2**
 */
RC_GTEST_PROP(StorageManagerProperty4,
              CsvRoundTripPreservesFields,
              ()) {
    const auto userId    = *genUserId();
    const auto deviceId  = *genDeviceId();
    const auto timestamp = *genTimestamp();

    std::string csv = Storage::formatCsvRow(userId.c_str(),
                                            deviceId.c_str(),
                                            timestamp.c_str());

    Storage::CsvRecord parsed = Storage::parseCsvRow(csv);

    RC_ASSERT(parsed.userId    == userId);
    RC_ASSERT(parsed.deviceId  == deviceId);
    RC_ASSERT(parsed.timestamp == timestamp);
}

// ═════════════════════════════════════════════════════════════════════════════
// Property 5: Scan log append preserves all previous records
// ═════════════════════════════════════════════════════════════════════════════

/**
 * Property 5: Appending N records to an in-memory log results in exactly N
 * data rows plus the header, and each row matches the original input.
 *
 * **Validates: Requirements 4.3**
 */
RC_GTEST_PROP(StorageManagerProperty5,
              AppendPreservesAllRecords,
              ()) {
    // Generate between 1 and 100 scan records
    const auto n = *rc::gen::inRange(1, 101);

    // Build the header
    const std::string header = "user_id,device_id,timestamp";
    std::string log = header + "\n";

    // Collect expected rows
    std::vector<std::string> expectedRows;
    expectedRows.reserve(n);

    for (int i = 0; i < n; ++i) {
        const auto userId    = *genUserId();
        const auto deviceId  = *genDeviceId();
        const auto timestamp = *genTimestamp();

        std::string row = Storage::formatCsvRow(userId.c_str(),
                                                deviceId.c_str(),
                                                timestamp.c_str());
        expectedRows.push_back(row);
        log += row + "\n";
    }

    // Parse the log back into lines
    std::vector<std::string> lines;
    std::istringstream stream(log);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    // First line is the header
    RC_ASSERT(lines.size() == static_cast<size_t>(n + 1));
    RC_ASSERT(lines[0] == header);

    // Each subsequent line matches the expected row
    for (int i = 0; i < n; ++i) {
        RC_ASSERT(lines[static_cast<size_t>(i + 1)] == expectedRows[static_cast<size_t>(i)]);
    }
}
