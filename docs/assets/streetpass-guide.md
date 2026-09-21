[简体中文](streetpass-guide.zh_CN.md) · **English**

# StreetPass first-version guide

StreetPass replaces the demo menu with the approved dark social-card interface. Its BLE and Wi-Fi behavior still needs physical-device acceptance; a successful build is not a passing hardware test. No account, hosted page, or Internet access is required for editing and card exchange.

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

The first key press after idle dimming restores brightness without activating an item. Settings for encounters, sound and dimming are saved locally. Unsupported characters outside the font coverage may display as placeholders; full UTF-8 text remains in the card. Contact QR codes contain the displayed text; WeChat does not auto-add a contact.

## Implementation boundaries

- Public cards are readable by nearby compatible BLE clients. The protocol provides framing, UTF-8 validation and a corruption checksum, not identity authentication or private messaging.
- A random application ID is separate from hardware `cardid`. GATT protocol v1 uses a custom 128-bit service and 14-byte chunks, supports MTU 23, and has a 20-second session timeout. Advertisement manufacturer ID `0xFFFF` is for this experimental prototype, not an assigned production company identifier.
- The lexicographically smaller application ID initiates. RSSI is a rough filter, not a distance measurement. Successful exchanges cool down for five minutes; failed connected attempts for ten seconds. Updates can be delayed by the cooldown.
- Editing uses one phone at a time, a random hotspot password and a separate session token. The server validates byte lengths and revision numbers; no web resources are fetched from external services.
- UTF-8 byte limits are 48 / 96 / 144 / 96 / 80 / 48 / 64 for nickname / introduction / message / interests / email / Telegram / WeChat. A typical Chinese character consumes three bytes.
- `spdata` is an NVS partition at `0x360000`, size `0x80000`. Initialization errors never trigger an automatic erase. The app partition remains 3 MB; protected `cardid`, Recovery and the five-second UP hook are retained.
- NFC is outside this version. No phone app, remote profile page, pairing, account or location history is used.

## Verification and next device checks

Run `./tools/validate.sh` in ESP-IDF 5.5.3. It checks documentation/workflows, original and StreetPass host tests, then builds and verifies the merged image. Storage host tests use a fake NVS backend; they do not validate flash wear or power-loss behavior.

`tests/render_sp_ui.c` renders the actual LVGL pages on a host with sample data. Its 40 KB LVGL pool matches the firmware setting; it does not model the ESP32's total RAM, DMA, Wi-Fi or BLE consumption.

Before normal use, test two boards: passing at walking speed, long profiles, simultaneous discovery, repeated encounters and updates, interrupted transfers, favorites at capacity, reboot persistence, idle battery life, sound, and repeated BLE-to-Wi-Fi transitions. Test QR scanning, captive-login behavior, fallback URL and interrupted saves on iOS and Android. Verify the permanent Recovery hook on the actual board. No device flashing or hardware-tested release has been performed. Source publication is separate from device acceptance.

- [Actual UI preview](../../assets/images/streetpass-lvgl-preview-v1.png)
- [Host rendering tool](../../tools/render_streetpass.py)

Long introductions and messages automatically scroll within their display areas.

- [Phone editor preview](../../assets/images/streetpass-phone-editor-v1.png)

Removing contact details changes future card exchanges. Previously received copies cannot be deleted remotely; another device receives the revised card on a later successful encounter.

### Software verification record (2026-09-21)

- Build: PASS. Complete validation gate finished on 2026-09-20 using ESP-IDF 5.5.3; the result and artifact were checked again on 2026-09-21.
- Host tests: PASS. Protocol, navigation, fake-NVS storage and repository checks passed. Actual LVGL pages rendered successfully, including long-text scrolling; contact, Wi-Fi and fallback QR payloads decoded correctly.
- Phone editor: PASS with a synthetic local backend in desktop Chrome. Preview, contact visibility, repeated saves and feedback were checked; this does not validate the device HTTP server or mobile hotspot behavior.
- Device tests: NOT RUN. Two-board exchange, mobile captive portals, radio memory use, battery life and physical Recovery remain unverified.
- Application: 2,383,008 bytes (3,145,728-byte limit). Merged image: 2,448,544 bytes, available locally at `build/FoloToy-AI-Passport-full.bin`.
- Merged image SHA-256: `348959836EEA599FA6942F0232997BD31211EFF516D63437A33F680650C5126F`.
