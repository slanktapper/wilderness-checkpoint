# Requirements Document

## Introduction

A battery-powered wilderness QR checkpoint device for outdoor events. Participants carry QR codes encoding their User ID. At each checkpoint, pressing a wake button activates the device, which scans the QR code, captures a photo, and stores both to an SD card. The device operates offline with no internet connectivity, supports up to 1000 users across an 8-hour event window, and enters deep sleep after inactivity to conserve power.

The system uses a two-board architecture: an Arduino Nano (ATMEGA328P) as the main controller handling the LED, wake button, and DS3231 RTC timekeeping, and an ESP32-CAM (AI-Thinker) handling camera, QR decoding, and SD card storage. The boards communicate over serial UART with appropriate voltage level shifting. The Nano controls power to the ESP32-CAM via a P-channel MOSFET, cutting power entirely during sleep for minimal battery drain.

## Glossary

- **Nano**: The Arduino Nano (ATMEGA328P) microcontroller board serving as the main controller for LED management, wake button handling, DS3231_RTC timekeeping, and inter-board serial communication
- **ESP32_CAM**: The AI-Thinker ESP32-CAM module with OV2640 camera, SD card slot, and WiFi/BT (WiFi/BT disabled in this application)
- **KY016_LED**: The KY-016 RGB LED module (common cathode, built-in resistors) connected to the Nano for visual status feedback
- **Wake_Button**: A momentary push button connected to the Nano that wakes the system from deep sleep
- **DS3231_RTC**: The DS3231 real-time clock module connected to the Nano via I2C (A4/SDA, A5/SCL), providing accurate timestamps. The Nano reads the time and sends it to the ESP32_CAM over the UART_Link
- **SD_Card**: A FAT32-formatted MicroSD card inserted into the ESP32_CAM's built-in slot for photo and log storage
- **OV2640_Camera**: The camera sensor on the ESP32_CAM used for QR code scanning and photo capture
- **Quirc_Library**: The quirc QR code decoding library used on the ESP32_CAM to decode QR images
- **UART_Link**: The serial UART communication channel between the Nano (5V logic) and ESP32_CAM (3.3V logic) using a voltage divider on the Nano TX line
- **User_ID**: A 4-digit zero-padded identifier (0001–1000) encoded in participant QR codes
- **Device_ID**: A 2-digit zero-padded identifier (01–32) determined at boot time by reading a 5-position DIP switch on the Nano
- **Scan_Log**: A CSV file on the SD_Card recording all scan events with columns: user_id, device_id, timestamp
- **Device_Directory**: The SD_Card directory named /DEVICEXX/ where XX is the Device_ID, containing all photos and the Scan_Log for that device
- **Inactivity_Timeout**: A 60-second period of no successful scans after which the system enters deep sleep
- **Deep_Sleep**: A low-power state where the Nano minimizes current draw and cuts power to the ESP32_CAM via the Power_MOSFET until the Wake_Button is pressed
- **Power_MOSFET**: A P-channel MOSFET (FQP27P06) controlled by a Nano GPIO pin that switches power to the ESP32_CAM. A 10kΩ pull-up resistor on the gate ensures the ESP32_CAM stays unpowered when the Nano is sleeping or resetting
- **VGA_Resolution**: 640×480 pixel resolution used for photo capture
- **ISO_8601_Timestamp**: A date-time string in the format YYYY-MM-DDTHH:MM:SS used in the Scan_Log

## Requirements

### Requirement 1: Two-Board Architecture and Communication

**User Story:** As a device builder, I want the system split across an Arduino Nano and ESP32-CAM communicating over serial UART, so that each board handles the tasks it is best suited for.

#### Acceptance Criteria

1. THE Nano SHALL serve as the main controller, managing the KY016_LED, Wake_Button, DS3231_RTC timekeeping, and sending commands to the ESP32_CAM over the UART_Link
2. THE ESP32_CAM SHALL handle QR code scanning via the OV2640_Camera and Quirc_Library, photo capture, and SD_Card storage
3. THE UART_Link SHALL operate at 115200 baud using serial communication between the Nano TX pin and ESP32_CAM RX pin (with a voltage divider from 5V to 3.3V) and ESP32_CAM TX pin to Nano RX pin (direct 3.3V connection)
4. WHEN the Nano sends a command over the UART_Link, THE ESP32_CAM SHALL acknowledge the command with a status response within 500 milliseconds
5. IF the ESP32_CAM does not respond to a UART_Link command within 500 milliseconds, THEN THE Nano SHALL retry the command up to 2 additional times before signaling an error state on the KY016_LED

### Requirement 2: Wake and Sleep Lifecycle

**User Story:** As an event organizer, I want the device to sleep when idle and wake on button press, so that battery life lasts the full 8-hour event.

#### Acceptance Criteria

1. WHEN the Wake_Button is pressed, THE Nano SHALL exit Deep_Sleep, drive the Power_MOSFET gate LOW to supply power to the ESP32_CAM, and set the KY016_LED to solid green within 3 seconds of the ESP32_CAM completing its boot sequence
2. WHILE the system is awake and no scan has occurred for 60 seconds (Inactivity_Timeout), THE Nano SHALL send a shutdown command to the ESP32_CAM over the UART_Link
3. WHEN the ESP32_CAM receives a shutdown command, THE ESP32_CAM SHALL finish any in-progress operations and send a ready-to-shutdown response to the Nano over the UART_Link
4. WHEN the Nano receives the ready-to-shutdown response, THE Nano SHALL wait 5 seconds, then drive the Power_MOSFET gate HIGH to cut power to the ESP32_CAM, turn off the KY016_LED, and enter Deep_Sleep itself
5. WHEN a successful scan occurs, THE Nano SHALL reset the Inactivity_Timeout counter to 60 seconds
6. WHILE in Deep_Sleep, THE system SHALL draw no more than 100 microamps total (Nano in power-down mode plus MOSFET pull-up leakage; ESP32_CAM draws zero with power cut)
7. THE system SHALL support a minimum of 1000 wake-scan-sleep cycles on a fully charged 3000mAh 18650 battery within an 8-hour event window
8. THE Power_MOSFET gate SHALL have a 10kΩ pull-up resistor to the power supply rail, ensuring the ESP32_CAM remains unpowered when the Nano is in Deep_Sleep or during reset

### Requirement 3: QR Code Scanning

**User Story:** As a participant, I want the device to scan my QR code quickly and reliably, so that I can check in at the checkpoint without delay.

#### Acceptance Criteria

1. WHEN the Nano sends a scan command over the UART_Link, THE ESP32_CAM SHALL initialize the OV2640_Camera in QR scanning mode (QVGA grayscale) and continuously capture and decode frames using the Quirc_Library
2. WHEN a valid QR code containing a User_ID (4-digit zero-padded string matching the pattern 0001–1000) is decoded, THE ESP32_CAM SHALL send the decoded User_ID to the Nano over the UART_Link and stop scanning
3. WHEN an invalid QR code is decoded (content does not match a valid User_ID pattern), THE ESP32_CAM SHALL send an invalid-QR status to the Nano over the UART_Link and stop scanning
4. IF no QR code is detected in a captured frame, THEN THE ESP32_CAM SHALL continue capturing and decoding frames until a QR code is found or the Nano sends a stop command
5. THE ESP32_CAM SHALL decode a valid QR code within 2 seconds of the scan command under normal lighting conditions

### Requirement 4: Photo Capture and Storage

**User Story:** As an event organizer, I want a photo captured for each scanned participant, so that I have visual verification of checkpoint visits.

#### Acceptance Criteria

1. WHEN the Nano sends a capture command over the UART_Link, THE ESP32_CAM SHALL reinitialize the OV2640_Camera in photo mode (VGA JPEG) and capture a single photo
2. WHEN the photo is captured, THE ESP32_CAM SHALL send a photo-ready status to the Nano over the UART_Link and hold the photo in memory
3. THE ESP32_CAM SHALL produce JPEG photos no larger than 50 kilobytes each at VGA_Resolution
4. WHEN a photo already exists for the same User_ID on the same device, THE ESP32_CAM SHALL overwrite the existing photo file with the new capture
5. IF the OV2640_Camera fails to capture a photo, THEN THE ESP32_CAM SHALL send a camera-error status to the Nano over the UART_Link

### Requirement 5: Scan Log Recording

**User Story:** As an event organizer, I want an append-only CSV log of all scans, so that I can reconstruct the event timeline after collection.

#### Acceptance Criteria

1. WHEN the Nano sends a save command over the UART_Link containing the User_ID, Device_ID, and ISO_8601_Timestamp, THE ESP32_CAM SHALL deinitialize the OV2640_Camera, mount the SD_Card, save the photo, and append a row to the Scan_Log
2. THE ESP32_CAM SHALL store the photo on the SD_Card at the path /DEVICEXX/DEVICEXX_USERYYYY.jpg where XX is the Device_ID and YYYY is the User_ID
3. THE ESP32_CAM SHALL append a row to the Scan_Log file at /DEVICEXX/scan_log.csv using the CSV format with columns: user_id, device_id, timestamp
4. THE ESP32_CAM SHALL write the timestamp column using the ISO_8601_Timestamp value received from the Nano (which the Nano reads from the DS3231_RTC)
5. WHEN the same User_ID is scanned again on the same device, THE ESP32_CAM SHALL overwrite the photo and append a new row to the Scan_Log (photo overwrites; log is append-only)
6. WHEN the Scan_Log file does not exist on the SD_Card, THE ESP32_CAM SHALL create the file with a header row (user_id,device_id,timestamp) before appending the first data row
7. WHEN the save operation completes, THE ESP32_CAM SHALL send a save-ok status to the Nano over the UART_Link
8. IF the SD_Card is full or unwritable, THEN THE ESP32_CAM SHALL send a storage-error status to the Nano over the UART_Link

### Requirement 6: SD Card Directory Structure

**User Story:** As an event organizer, I want each device's data in its own directory, so that SD cards from multiple devices can be merged without conflicts.

#### Acceptance Criteria

1. WHEN the ESP32_CAM boots and the SD_Card is mounted, THE ESP32_CAM SHALL create the Device_Directory /DEVICEXX/ if the directory does not already exist
2. THE ESP32_CAM SHALL store all photos and the Scan_Log exclusively within the Device_Directory
3. THE Device_Directory name SHALL use the format DEVICEXX where XX is the 2-digit zero-padded Device_ID

### Requirement 7: DS3231 RTC Timekeeping

**User Story:** As an event organizer, I want accurate timestamps on all scan records, so that I can determine the order and timing of checkpoint visits.

#### Acceptance Criteria

1. WHEN the Nano boots, THE Nano SHALL initialize the DS3231_RTC over I2C (A4/SDA, A5/SCL) and verify communication
2. IF the DS3231_RTC fails to respond during initialization, THEN THE Nano SHALL set the KY016_LED to solid red for 3 seconds and use a fallback timestamp of "0000-00-00T00:00:00" for all subsequent scans
3. WHEN a timestamp is needed for the Scan_Log, THE Nano SHALL read the current time from the DS3231_RTC, format the value as an ISO_8601_Timestamp, and send it to the ESP32_CAM as part of the save command over the UART_Link
4. THE DS3231_RTC SHALL maintain time accuracy within 2 seconds per day using its internal temperature-compensated crystal oscillator

### Requirement 8: LED Status Feedback

**User Story:** As a participant, I want clear visual feedback from the device LED, so that I know whether my scan succeeded or failed.

#### Acceptance Criteria

1. WHEN the system is awake and ready to scan, THE Nano SHALL set the KY016_LED to solid green
2. WHEN the ESP32_CAM reports a valid QR code decoded, THE Nano SHALL set the KY016_LED to solid white for 1 second
3. WHEN the ESP32_CAM reports an invalid QR code, THE Nano SHALL set the KY016_LED to solid red for 2 seconds, then return to solid green
4. WHILE the ESP32_CAM is performing the photo capture countdown, THE Nano SHALL blink the KY016_LED in an accelerating white/green pattern over 3 seconds
5. WHILE the ESP32_CAM is saving the photo and log to the SD_Card, THE Nano SHALL set the KY016_LED to solid blue
6. WHEN the save operation completes successfully, THE Nano SHALL blink the KY016_LED in an alternating blue/green pattern for 3 seconds, then return to solid green
7. IF the ESP32_CAM reports any error (storage-error, rtc-error, or camera-error), THEN THE Nano SHALL set the KY016_LED to solid red for 3 seconds
8. WHEN the system enters Deep_Sleep, THE Nano SHALL turn off the KY016_LED

### Requirement 9: Device Configuration

**User Story:** As a device builder, I want to set the Device ID using physical DIP switches on the Nano, so that each checkpoint device can be uniquely identified without recompiling firmware.

#### Acceptance Criteria

1. THE Device_ID SHALL be determined at boot time by reading a 5-position DIP switch connected to 5 digital pins on the Nano
2. THE Nano SHALL read the 5 DIP switch positions as binary digits and convert the result to a Device_ID in the range 01–32
3. THE Nano SHALL use internal pull-up resistors on the DIP switch pins so that an OFF position reads HIGH (1) and an ON position reads LOW (0)
4. THE Nano SHALL send the Device_ID to the ESP32_CAM over the UART_Link during the boot initialization sequence
5. THE Device_ID SHALL be formatted as a 2-digit zero-padded string (e.g., "01", "17", "32") for use in file paths and the Scan_Log
6. IF all DIP switches are OFF (binary 00000 = 0), THEN THE Nano SHALL treat this as an invalid configuration and set the KY016_LED to a blinking red pattern until the switches are corrected and the device is reset

### Requirement 10: Boot Initialization Sequence

**User Story:** As a device builder, I want a deterministic boot sequence with clear error reporting, so that I can diagnose hardware issues in the field.

#### Acceptance Criteria

1. WHEN the ESP32_CAM powers on, THE ESP32_CAM SHALL initialize the SD_Card to verify it is present and writable, then deinitialize the SD_Card to release shared GPIO pins
2. AFTER the SD_Card check, THE ESP32_CAM SHALL initialize the OV2640_Camera in QR scanning mode (QVGA grayscale) and verify it is operational
3. WHEN both subsystems initialize successfully, THE ESP32_CAM SHALL send init-ok status to the Nano over the UART_Link and wait for a scan command
4. IF the SD_Card fails to initialize, THEN THE ESP32_CAM SHALL send sd-error to the Nano over the UART_Link and halt further initialization
5. IF the OV2640_Camera fails to initialize, THEN THE ESP32_CAM SHALL send camera-error to the Nano over the UART_Link and halt further initialization
6. WHEN the Nano receives init-ok, THE Nano SHALL set the KY016_LED to solid green indicating the system is ready
7. IF the Nano receives any error status during boot, THEN THE Nano SHALL set the KY016_LED to solid red and remain in error state until the Wake_Button is pressed to retry

### Requirement 11: UART Protocol Format

**User Story:** As a firmware developer, I want a well-defined serial protocol between the two boards, so that communication is reliable and debuggable.

#### Acceptance Criteria

1. THE UART_Link protocol SHALL use newline-terminated ASCII text messages
2. THE Nano SHALL send commands in the format CMD:<command_name>[:<parameter>]\n
3. THE ESP32_CAM SHALL send responses in the format RSP:<status>[:<data>]\n
4. THE UART_Link messages SHALL not exceed 64 bytes in length including the newline terminator
5. WHEN the ESP32_CAM receives a malformed command, THE ESP32_CAM SHALL respond with RSP:ERR:PARSE\n

### Requirement 12: Voltage Level Shifting

**User Story:** As a device builder, I want safe voltage levels between the 5V Nano and 3.3V ESP32-CAM, so that the ESP32_CAM is not damaged by overvoltage.

#### Acceptance Criteria

1. THE UART_Link SHALL use a resistor voltage divider on the Nano TX line to reduce the 5V signal to 3.3V before reaching the ESP32_CAM RX pin
2. THE UART_Link SHALL connect the ESP32_CAM TX pin directly to the Nano RX pin (3.3V output exceeds the Nano's logic HIGH threshold)
3. THE voltage divider SHALL produce an output voltage between 3.0V and 3.3V when the Nano TX pin outputs 5V
