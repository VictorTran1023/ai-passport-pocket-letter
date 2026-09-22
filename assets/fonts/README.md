<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Fonts

Store reusable font files and generated font sources here.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

## StreetPass

- `sp_font_16.c`: Noto Sans SC, weight 500, 16 px, compressed 1 bpp; ASCII, CJK basic characters, CJK/fullwidth punctuation and UI separators.
- `sp_font_ui_16.c`: weight 500, 16 px, compressed 2 bpp; 3,755 GB2312 level-one Han characters, ASCII and punctuation, with `sp_font_16` as the complete basic-CJK fallback.
- `sp_font_24.c`: weight 600, 24 px, compressed 2 bpp; the same common-character coverage for titles and names, with the body font as fallback. Rare title characters can appear at 16 px.
- Use `--only-polish` to regenerate the two 2 bpp fonts without rebuilding the unchanged full fallback. Fonts remain in Flash; they are not copied wholesale into RAM.
- Source: Noto Sans SC variable font, SHA-256 `763146584cf0710223441356b4395e279021b0806c196614377a7a0174ae074a`. Redistribution license: [SIL OFL 1.1](NotoSansSC-LICENSE.txt), from the [Noto CJK project](https://github.com/notofonts/noto-cjk).
- Rebuild with [generate_streetpass_fonts.py](../../tools/generate_streetpass_fonts.py) using `fonttools==4.59.2`, `lv_font_conv@1.5.3`, and `--font /path/to/NotoSansSC-VF.ttf`. Generated C is linked into `main`; enable `CONFIG_LV_USE_FONT_COMPRESSED=y`. Conversion tools and static intermediate fonts stay in ignored `build/`.
