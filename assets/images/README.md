<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Images

Store reusable source images and generated display assets here.

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## StreetPass references and actual rendering

- [Approved color reference](streetpass-techdark-palette-reference.png): supplied by the user, retained as a design reference; no additional redistribution rights are asserted.
- [Approved design overview](streetpass-techdark-ui-overview-v1.png): AI-generated visual proposal. Its QR patterns are illustrative.
- [Actual LVGL preview](streetpass-lvgl-preview-v1.png): 12 pages rendered from `main/sp_ui.c` with synthetic data. The hotspot credential shown is a fixture, not a real device password. Contact, Wi-Fi and editor QR payloads were independently decoded on the host. This is not a device photograph.
- Other `streetpass-*.png` concepts remain historical alternatives; the approved dark-tech design supersedes their palettes.
- Reproduce page images with [render_streetpass.py](../../tools/render_streetpass.py) and a host compiler. These previews do not verify LCD brightness, radio behavior or phone interoperability.

- [Phone editor preview](streetpass-phone-editor-v1.png): real HTML rendered in local headless Chrome with synthetic API fixtures. Preview, comma normalization, private/public contacts and successive save feedback were tested; no physical hotspot or device backend was involved.

## Repository presentation, revision 2

Added on 2026-09-21 for the StreetPass repository landing page. These are AI-generated presentation assets, separate from the actual LVGL and phone-editor previews above.

- [Product cover](streetpass-hero-v2.png): generated using the [official front reference](../../docs/assets/brand/ai-passport-front.png) for the enclosure and the v1 UI for functionality. The screen art is conceptual; the image is not a device photograph.
- [Four-screen UI study](streetpass-ui-study-v2.png): a visual proposal with calmer typography, grouping and spacing. Target hardware remains 240 x 320 with physical buttons. The illustrated QR is not an onboarding credential. This image does not change the firmware UI or its validation status.
- [Encounter story](streetpass-encounter-story-v2.png): an editorial doodle explaining local editing, BLE encounters and conversation. It is a scenario illustration, not evidence of real-world exchange reliability.

Keep these images versioned. Do not replace actual implementation previews with generated art without an explicit concept caption. The referenced hardware belongs to FoloToy; the presentation does not imply an official release or endorsement.

## Implemented UI, revision 2

- [Four-page preview](streetpass-lvgl-focus-v2.png) and [all 12 pages](streetpass-lvgl-preview-v2.png): rendered from the revised `main/sp_ui.c` on 2026-09-22 using the host renderer and synthetic profiles. PNG montages preserve each 240 x 320 screen at native resolution. The UI study informed typography, spacing, blue accents and the envelope mascot; physical-button navigation is retained. QR payloads are real test fixtures and decode successfully. These are software renders, not LCD photographs.
