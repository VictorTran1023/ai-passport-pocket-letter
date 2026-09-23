[简体中文](streetpass-guide.zh_CN.md) · **English**

# Pocket Letter first-version guide

Pocket Letter replaces the demo menu with the approved dark social-card interface. Its BLE and Wi-Fi behavior still needs physical-device acceptance; a successful build is not a passing hardware test. No account, hosted page, or Internet access is required for editing and card exchange.

## Use

1. Open **Edit by QR**. BLE stops and a temporary WPA2 hotspot starts. Scan with the phone system camera and confirm joining. This network intentionally has no Internet access.
2. The phone may open a network-login window. If it does not, press OK on the device to show the second QR, or open `http://192.168.4.1` after joining the hotspot.
3. Fill in a nickname, introduction, message and comma-separated interests. Email, Telegram username and WeChat ID are optional. Enable the contact-sharing checkbox only for contact details you want included in the public card. Preview and confirm saving.
4. Wait for the saved confirmation on the phone, then hold device OK to leave editing. BLE resumes unless it was paused in Settings. The hotspot closes automatically after ten minutes in edit mode.
5. Carry two compatible devices nearby. A received card appears only after validation and local storage. Receipt does not prove that both devices finished exchanging before separating.

The application stores one own card and up to 100 people. A matching or older revision creates no duplicate and performs no additional flash write. An updated revision becomes unread. At capacity, a new person replaces the oldest non-favorite record; if all records are favorites, the card is rejected. Stored ordering reflects the last new or updated card, not wall-clock encounter time. No encounter count is shown.

## Buttons

| Screen | UP / DOWN | OK | Hold OK |
| --- | --- | --- | --- |
| Home / lists / Settings | Select | Open / change | Back |
| Card | Previous / next page | Next page / open contacts | Back |
| Card, any page | Hold UP to toggle favorite; hold DOWN to request deletion | Confirm only on the separate deletion page | Back |
| Contact list | Select | Show the contact text as a QR | Back |
| Edit | No action | Switch Wi-Fi / editor QR | Stop hotspot and exit |

Double-press OK on any page to turn off the backlight. The first UP, DOWN, or OK action wakes the screen without activating an item; while dark, BLE encounters and local card storage continue. If 30-second automatic screen-off is enabled in Settings, idle use turns off the backlight, but phone editing does not automatically darken. An incoming card does not wake the screen. The separate hardware power button is not connected to the firmware button API; its short press cannot be assigned to screen-off on the documented board. This is a backlight-off mode, not deep sleep or a measured battery-life claim.

Settings for encounters, sound and automatic screen-off are saved locally. Unsupported characters outside the font coverage may display as placeholders; full UTF-8 text remains in the card. Contact QR codes contain the displayed text; WeChat does not auto-add a contact.

## Implementation boundaries

- Public cards are readable by nearby compatible BLE clients. The protocol provides framing, UTF-8 validation and a corruption checksum, not identity authentication or private messaging.
- A random application ID is separate from hardware `cardid`. GATT protocol v1 uses a custom 128-bit service and 14-byte chunks, supports MTU 23, and has a 20-second session timeout. Advertisement manufacturer ID `0xFFFF` is for this experimental prototype, not an assigned production company identifier.
- The lexicographically smaller application ID initiates. RSSI is a rough filter, not a distance measurement. Successful exchanges cool down for five minutes; failed connected attempts for ten seconds. Updates can be delayed by the cooldown.
- Editing uses one phone at a time, a random hotspot password and a separate session token. The server validates byte lengths and revision numbers; no web resources are fetched from external services.
- UTF-8 byte limits are 48 / 96 / 144 / 96 / 80 / 48 / 64 for nickname / introduction / message / interests / email / Telegram / WeChat. A typical Chinese character consumes three bytes.
- `spdata` is an NVS partition at `0x360000`, size `0x80000`. Initialization errors never trigger an automatic erase. The app partition remains 3 MB; protected `cardid`, Recovery and the five-second UP hook are retained.
- NFC is outside this version. No phone app, remote profile page, pairing, account or location history is used.

## Verification and next device checks

Run `./tools/validate.sh` in ESP-IDF 5.5.3. It checks documentation/workflows, original and Pocket Letter host tests, then builds and verifies the merged image. Storage host tests use a fake NVS backend; they do not validate flash wear or power-loss behavior.

`tests/render_sp_ui.c` renders the actual LVGL pages on a host with sample data. Its 40 KB LVGL pool matches the firmware setting; it does not model the ESP32's total RAM, DMA, Wi-Fi or BLE consumption.

Before normal use, test two boards: passing at walking speed, long profiles, simultaneous discovery, repeated encounters and updates, interrupted transfers, favorites at capacity, reboot persistence, idle battery life, sound, and repeated BLE-to-Wi-Fi transitions. Test QR scanning, captive-login behavior, fallback URL and interrupted saves on iOS and Android. Verify the permanent Recovery hook on the actual board. The refined UI passed USB flashing, boot and BLE-advertisement smoke checks. No hardware-accepted release has been published.

- [Actual UI preview](../../assets/images/pocket-letter-lvgl-preview-v1.png)
- [Host rendering tool](../../tools/render_streetpass.py)

Long introductions and messages automatically scroll within their display areas.

- [Phone editor preview](../../assets/images/pocket-letter-phone-editor-v1.png)

Removing contact details changes future card exchanges. Previously received copies cannot be deleted remotely; another device receives the revised card on a later successful encounter.

### Software verification record (2026-09-21)

- Build: PASS. Complete validation gate finished on 2026-09-20 using ESP-IDF 5.5.3; the result and artifact were checked again on 2026-09-21.
- Host tests: PASS. Protocol, navigation, fake-NVS storage and repository checks passed. Actual LVGL pages rendered successfully, including long-text scrolling; contact, Wi-Fi and fallback QR payloads decoded correctly.
- Phone editor: PASS with a synthetic local backend in desktop Chrome. Preview, contact visibility, repeated saves and feedback were checked; this does not validate the device HTTP server or mobile hotspot behavior.
- Device tests: NOT RUN. Two-board exchange, mobile captive portals, radio memory use, battery life and physical Recovery remain unverified.
- Application: 2,383,008 bytes (3,145,728-byte limit). Merged image: 2,448,544 bytes, available locally at `build/FoloToy-AI-Passport-full.bin`.
- Merged image SHA-256: `348959836EEA599FA6942F0232997BD31211EFF516D63437A33F680650C5126F`.

### Refined UI verification (2026-09-22)

- Build: PASS. Complete ESP-IDF 5.5.3 gate, merged image and mini-program Recovery contract verified. Application: 3,133,280 / 3,145,728 bytes; 12,448 bytes remain, so future features must recheck the size budget.
- Host tests: PASS. Protocol, navigation, fake-NVS storage and repository checks. All 12 LVGL pages render with the 40 KB pool; maximum text, empty states and 20 repeated page cycles pass with no continuing memory growth. Three QR payloads decode correctly.
- Device tests: PASS for segmented flashing, 40-second startup observation without panic/reset loops and external SP v1 BLE advertisement detection on ESP32-C3 revision 1.1 with 8 MB Flash. Existing profile/inbox, NVS/PHY, device identity and permanent Recovery digests matched before and after flashing. Startup free heap: 49,652 bytes. A verified original 8 MB backup remains local. This is a smoke test, not full device acceptance. The flashed local artifact was built before commit `c36fa49` and retains the embedded development label `f300c8b-dirty`; identify it by the checksum below.
- Unverified: refined-UI LCD appearance and physical controls, phone QR/hotspot editing, two-board exchanges, battery life, sound and physical Recovery entry.
- Refined merged image: 3,198,816 bytes. SHA-256: `e4fac9edf62f401e9a4c319fba18291f04bdaf934b4c6b11dd3862c413f3a886`. Local output: `build/FoloToy-AI-Passport-full.bin`; no device-specific data is included.

### Fixed top titles verification (2026-09-23)

- Build and host tests: PASS. Full ESP-IDF 5.5.3 gate passed; application 3,133,344 / 3,145,728 bytes. Long headings use LVGL's stationary ellipsis mode in the host renderer, while body paragraphs still scroll.
- Device smoke tests: PASS. Segmented flashing, 40-second startup without panic/reset loops and external SP v1 BLE advertisement detection. Existing profile/inbox, NVS/PHY, cardid and Recovery digests matched before and after flashing. Startup free heap: 49,524 bytes.
- Unverified: user confirmed the top display looks normal. Physical controls, phone hotspot/editor and two-board exchanges remain untested.
- Flashed merged image SHA-256: `a3d4ff0a02bc53eedc8c8b2d5341e48e4291053cf928a98dcaeea6b7ce5a002e`. The local image was built before this documentation commit, so its embedded development label may show the prior commit with `-dirty`.

### Pocket Letter verification (2026-09-23)

- Build: PASS. Complete ESP-IDF 5.5.3 gate and merged-image contract passed. Application: 3,133,376 / 3,145,728 bytes, leaving 12,352 bytes. The merged image is 3,198,912 bytes; SHA-256: `034e1538d7c9209223cd3c600c1c0de0fcdd92c8d296c9b9651365eb4596179b`.
- Host tests: PASS. Repository and protocol/navigation/storage tests, all 12 actual LVGL renders, 20 page cycles, three independently decoded QR payloads, and the desktop phone editor with fixture API all passed.
- Device smoke tests: PASS. Segmented USB flash, 40-second startup without panic/reset loops, and an external compatible SP v1 BLE advertisement check passed. Existing profile/inbox, NVS/PHY, `cardid` and Recovery digests matched before and after flashing. Startup free heap: 49,520 bytes. The user confirmed that the new product title displays correctly and stays stationary. The image was built before this documentation commit and embeds the prior development commit label with `-dirty`; identify it by the checksum above.
- Unverified: physical button flow, phone Wi-Fi QR/hotspot/editor on iOS and Android, two-device exchanges, battery use, sound and physical Recovery entry. The existing `streetpass` NVS namespace and SP v1 payload remain unchanged for compatibility.

### Screen-off verification (2026-09-23)

- Build: PASS. The complete ESP-IDF 5.5.3 gate passed, including the mini-program BLE and Recovery layout contract. Application: 3,133,456 / 3,145,728 bytes, leaving 12,272 bytes. Merged image: 3,198,992 bytes; SHA-256: `265cc81bec73d51a1797519abfb0570f33d0cc02a60c2aaedd01139efaee3308`.
- Host tests: PASS. The screen state test covers double-OK off/on, wake consuming the first key event, the 30-second boundary, the disabled setting and the phone-editing exemption. Repository, protocol, navigation and storage checks passed. All 12 actual LVGL pages rendered, including the updated Settings page.
- Device smoke tests: PASS. Segmented USB flashing preserved profile/inbox, NVS/PHY, `cardid` and Recovery digests. A 40-second startup showed no panic or reset loop; startup free heap was 49,520 bytes. External scanning detected a compatible SP v1 advertisement after the screen-off timeout window.
- Unverified: manual double-OK and wake behavior on the physical buttons, measured current or battery-life improvement, phone editing, two-device exchanges and physical Recovery entry. The image was built before this documentation commit and embeds a `-dirty` development label; identify it by the checksum above.
