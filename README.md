[简体中文](README.zh_CN.md) · **English**

# StreetPass for AI Passport

**Carry a little about yourself. Meet someone along the way.**

An offline social-card firmware for the FoloToy AI Passport. Nearby devices exchange public cards over Bluetooth; a phone edits your card through the device's own Wi-Fi hotspot. No account, cloud server, Internet connection, or companion app is needed during use.

[![Checks](https://github.com/VictorTran1023/ai-passport-streetpass/actions/workflows/ci.yml/badge.svg)](https://github.com/VictorTran1023/ai-passport-streetpass/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

**Status: first software prototype.** Local firmware build and host tests pass. Two-device exchanges and phone interoperability still require hardware testing. This is an independent application built on [FoloToy AI Passport](https://github.com/FoloToy/ai-passport), not an official FoloToy release.

[User guide](docs/assets/streetpass-guide.md) · [Phone editor](assets/images/streetpass-phone-editor-v1.png) · [Build setup](docs/development/environment-setup.md) · [Report a bug](https://github.com/VictorTran1023/ai-passport-streetpass/issues)

## A quiet interface for small encounters

![Twelve actual StreetPass LVGL screens](assets/images/streetpass-lvgl-preview-v1.png)

Actual 240 x 320 LVGL pages rendered on a computer with fictional profiles. The palette uses charcoal surfaces, blue selection accents and a small envelope mascot. This preview is not a photograph of the device.

## What it does

| Feature | First-version behavior |
| --- | --- |
| Public profile | Nickname, introduction, message, interests, and optional email, Telegram and WeChat |
| Encounters | BLE discovery and card exchange between compatible devices; reception is saved locally |
| Inbox | Up to 100 people, revision deduplication, unread updates, favorites and deletion |
| Phone editing | Temporary WPA2 hotspot, Wi-Fi QR, local browser editor and a fallback URL QR |
| Contact sharing | Optional contact fields; displayed text can be scanned as a QR |
| Daily use | Physical UP/DOWN/OK controls, saved settings, optional sound and idle dimming |

At capacity, the oldest non-favorite card is replaced. An inbox containing only favorites rejects new cards. NFC, private chat, identity verification and cloud synchronization are outside this version.

## Try the flow

1. Open **Edit by QR** on the device. Scan its Wi-Fi QR with your phone and confirm joining the hotspot. Bluetooth encounters pause while editing.
2. Open the local editor. If the phone does not display a login page, press device OK for the second QR, or visit `http://192.168.4.1` while connected to the hotspot.
3. Write a short card. Choose whether to publish contact details, preview, save, and wait for confirmation. Hold device OK to exit editing and resume encounters unless paused in Settings.
4. Carry two compatible devices nearby. Open the inbox to read received cards, discover shared interests, or show a contact QR.

Use UP/DOWN to select, OK to open, and hold OK to return. On another person's card, hold UP to toggle favorite or hold DOWN to request deletion; deletion requires a separate confirmation. See the [full button table](docs/assets/streetpass-guide.md#buttons).

**Privacy:** cards are public to nearby compatible BLE clients. Share only what you are comfortable making public. Removing a field affects future exchanges and cannot remotely erase previously received copies. Contact QR codes contain text; they do not automatically add a WeChat contact.

## Build and verify

Target: **ESP32-C3, 8 MB Flash, no PSRAM, ESP-IDF 5.5.3**. Follow the [environment setup](docs/development/environment-setup.md) for prerequisites and Windows setup. In an ESP-IDF-enabled Bash shell:

```bash
git clone https://github.com/VictorTran1023/ai-passport-streetpass.git
cd ai-passport-streetpass
# Activate your installed ESP-IDF 5.5.3 environment first.
./tools/validate.sh --static
./tools/validate.sh --firmware
```

The firmware gate creates `build/FoloToy-AI-Passport-full.bin`, the verified merged image for flash offset `0x0`. Build outputs are not committed. Successful CI runs also retain the merged image as a downloadable Actions artifact for seven days; this does not constitute a hardware-tested release. Follow the [hardware and flashing guide](docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.md) before writing a device.

The 3 MB application limit, protected `cardid` at `0x356000`, permanent Recovery at `0x700000`, and five-second UP bootloader hook are retained. Public cards use a separate NVS partition. Host storage tests use fake NVS and cannot prove real flash or power-loss behavior.

| Verification | Recorded result |
| --- | --- |
| Build | PASS locally with ESP-IDF 5.5.3; application 2,383,008 / 3,145,728 bytes |
| Host tests | PASS: protocol, navigation, storage and repository checks |
| UI and editor | Actual LVGL rendering, QR decoding and desktop-browser editor checks passed |
| Device tests | NOT RUN |
| Unverified | Walking encounters, mobile captive portals, power use, radio memory and physical Recovery |

See the [verification record](docs/assets/streetpass-guide.md#software-verification-record-2026-09-21) for the dated artifact checksum and test boundaries. The workflow badge above reflects current remote checks rather than this recorded local result.

## Project map

| Path | Responsibility |
| --- | --- |
| `main/sp_core.*`, `main/sp_nav.*` | Bounded card format, BLE fragments and navigation logic |
| `main/sp_store.*` | Local profile and inbox persistence |
| `main/sp_ble.*` | BLE discovery and transfer state machine |
| `main/sp_web.*`, `main/sp_editor.html` | Temporary hotspot and embedded phone editor |
| `main/sp_ui.*` | Device pages and dark theme |
| `components/bsp/` | Upstream board support |
| `tests/`, `tools/` | Host checks, firmware verification and preview tooling |
| `docs/assets/` | StreetPass guide and design decisions |

This standalone repository develops StreetPass on `main`. Retained upstream hardware and demo documents describe the original platform; the root README and StreetPass guide describe this application. The inherited fork-sync workflow skips standalone repositories. Upstream history and attribution are preserved.

## Contribute and credits

Start with the [contribution guide](.github/CONTRIBUTING.md). Please include the board revision, firmware commit, reproduction steps and separate build/device results in bug reports. Hardware feedback is especially useful for the unverified behaviors above. Report exploitable details through the [security policy](.github/SECURITY.md).

Based on **FoloToy AI Passport**, with the original [MIT license](LICENSE) and copyright notice retained. Noto Sans SC is distributed under the [SIL Open Font License](assets/fonts/NotoSansSC-LICENSE.txt). Preview profiles are fictional; image provenance and reference limitations are listed in the [asset notes](assets/images/README.md).
