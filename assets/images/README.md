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
