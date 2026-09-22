[简体中文](streetpass-development.zh_CN.md) · **English**

# StreetPass implementation plan

Scope: [product decisions](streetpass-decisions.md). The implementation was developed in an isolated worktree and is maintained as a standalone StreetPass repository. Hardware acceptance remains pending; no server setup is required.

1. Implement and test bounded UTF-8 cards, versioned wire encoding, ordered BLE fragments, deterministic connection roles, and inbox eviction protecting favorites.
2. Persist an own card and up to 100 peers in a dedicated NVS partition after protected cardid. Commit before reporting success; never erase identity or Recovery on initialization failure.
3. Build the approved 240 x 320 dark UI for physical UP/DOWN/OK input, with a small envelope mascot, real QR codes, battery status, unread/favorites/delete, and idle dimming.
4. Implement a temporary password-protected AP, bounded DNS captive portal and embedded phone editor. Only explicit device edit mode allows saves; protect writes with a session token. Stop HTTP/DNS before Wi-Fi, and resume BLE only after shutdown.
5. Exchange public cards through a versioned custom BLE GATT service. Validate and persist before acknowledging; distinguish local receipt from bilateral success. Enforce session timeouts, one connection, and retry cooldowns.
6. Run host checks and the ESP-IDF 5.5.3 build/merge verification. Device acceptance requires two boards plus iOS/Android: passing, updates, repeat encounters, interrupted transfer, full favorites, reboot, QR readability, captive portal fallback, power and Recovery.

## Resource and behavior budget

- Application stays within 3 MB. Use compressed 2 bpp Noto Sans SC fonts at 16/24 px for common characters and a 16 px 1 bpp full basic-CJK fallback in Flash. See the [font coverage](../../assets/fonts/README.md). The generated font is not loaded into a RAM buffer.
- Only a compact 100-person index stays in RAM; card payloads are loaded on demand. No audio or image decoding at boot. UI, radio, and storage use bounded queues/buffers.
- Public cards are intentionally readable by nearby compatible devices. The first version provides no identity authentication or private messaging; optional contact fields are explicitly published by their owner.
- QR joining requires the phone OS confirmation where applicable. Captive portal opening is best effort; the second QR always points to the device-local editor.
- No real profile information, hotspot credentials, or hardware identity enters source control.
