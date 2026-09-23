[简体中文](streetpass-decisions.zh_CN.md) · **English**

# Pocket Letter confirmed design decisions

Recorded: 2026-09-13; implementation update: 2026-09-20. The first software implementation is available in this branch; physical-device acceptance is pending. See the [current guide](streetpass-guide.md).

## Offline editing and QR onboarding

- No hosted service, account, or dedicated phone application is required. The device serves the complete editor locally over its temporary Wi-Fi access point.
- Entering Edit Card pauses BLE encounters and shows a Wi-Fi QR containing the temporary network credentials. The phone scans with its system camera and confirms joining where required by the operating system.
- A captive portal attempts to open the local editor after joining. Automatic opening is best effort; it may use a system login window rather than the full browser.
- If the page does not appear, pressing OK shows a second QR for the local editor URL, with the address also printed. The phone must already be connected to the device hotspot.
- Keep the local hotspot active after saving until the success response is delivered. Exiting editing closes the hotspot and resumes encounters.
- Use unique hotspot names and temporary credentials; expose editing only during the physical editing session. Verify joining, portal behavior, manual fallback, saving, and exit on both iOS and Android.

## Visual direction

- Dark, low-glare, flat, restrained, readable, and slightly cute. Previous sage-green light and dark mockups are exploratory and are not approved visual targets.
- Selected direction: combine A (iOS-inspired minimal hierarchy and grouped lists) with D (a small cute envelope mascot; its original warm palette is superseded by the confirmed palette below). Preserve the 240 x 320 display and UP/DOWN/OK operation; avoid phone-only controls and expensive glass effects. The combined review image is [warm minimal overview](../../assets/images/streetpass-ad-ui-overview-v1.png); selection of the direction is confirmed, while this newly generated revision remains a visual proposal, not implemented firmware.
- Conserve generation budget by comparing a small number of key screens before expanding the selected style to all screens.

## References

- [ESP-IDF captive portal example](https://github.com/espressif/esp-idf/blob/master/examples/protocols/http_server/captive_portal/README.md)
- [Apple captive network behavior](https://support.apple.com/en-ie/102554)
- [Apple dark appearance guidance](https://developer.apple.com/design/human-interface-guidelines/dark-mode)

## Confirmed palette update

The user supplied a tech-dark palette, superseding the warm A+D colors while retaining the selected layout and small mascot. Background: `#0D0D11`; surfaces: `#141418`; primary accent: `#00AEEF`; sparse highlight: `#00F0FF`; primary text: `#D0D0D0`; secondary text: `#A0A0A0`. Bright accents remain small; use a subdued dark selection background and preserve conventional dark-on-light QR rendering. These hex values are the implementation targets, not a claim of exact pixels in a generated concept.

- [User palette reference](../../assets/images/streetpass-techdark-palette-reference.png)
- [Updated concept overview](../../assets/images/streetpass-techdark-ui-overview-v1.png)

## Approved first-version visual baseline

The user approved `streetpass-techdark-ui-overview-v1.png` after reviewing the palette update. Use this image as the first-version visual baseline: A-style hierarchy, D-style small friendly elements, and the exact tech-dark palette above. Earlier images remain historical alternatives. Do not generate more style alternatives unless requested. Approval covers the visual direction; the QR examples remain illustrative and firmware behavior still requires implementation and validation.

## Repository presentation update (2026-09-21)

The earlier repository cover, four-screen UI study and illustrated encounter flow are documented in the [image index](../../assets/images/README.md#repository-presentation-revision-2). They refined presentation within the approved charcoal and blue palette. Those images are historical concepts, not device test evidence; current Pocket Letter renders are linked from the README.
