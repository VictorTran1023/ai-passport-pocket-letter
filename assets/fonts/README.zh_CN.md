<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 字库资源（Fonts）

本目录存放项目可复用的字库资源。每个字库子目录或单个字库文件，应附说明。

## 如何使用

- 字库文件（如 `.ttf`、`.otf`、LVGL 使用的 C 数组字库等）复制到本目录，并在本项目 `README.md` 记录字名、字号、支持字符集与版权信息。
- 若需集成到 ESP-IDF 固件，参考 [`components/bsp/include/bsp_display.h`](../../components/bsp/include/bsp_display.h) 与 LVGL 字体接口，将字库转换为对应格式并放入正确资源目录。
- 字库占用 Flash 与内存，需在集成前评估 ESP32-C3 无 PSRAM 的限制（详见 `docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.md`）。

## 擦肩

- `sp_font_16.c`：Noto Sans SC，字重 500，16 像素，压缩 1 bpp；包含 ASCII、CJK 基本汉字、中日韩／全角标点及界面分隔符。
- `sp_font_ui_16.c`：字重 500，16 像素，压缩 2 bpp；覆盖 3,755 个 GB2312 一级汉字、ASCII 和标点，使用 `sp_font_16` 回退到完整基本汉字区。
- `sp_font_24.c`：字重 600，24 像素，压缩 2 bpp；标题及昵称采用相同常用字覆盖，使用正文字体回退。罕见的标题汉字可能以 16 像素显示。
- `--only-polish` 仅重新生成两个 2 bpp 字体，保留完整回退字库。字体保存在 Flash，不会整体复制进 RAM。
- 来源：Noto Sans SC 可变字体，SHA-256 `763146584cf0710223441356b4395e279021b0806c196614377a7a0174ae074a`。许可为 [SIL OFL 1.1](NotoSansSC-LICENSE.txt)，来自 [Noto CJK 项目](https://github.com/notofonts/noto-cjk)。
- 使用 [generate_streetpass_fonts.py](../../tools/generate_streetpass_fonts.py) 重建，需要 `fonttools==4.59.2`、`lv_font_conv@1.5.3` 和 `--font /path/to/NotoSansSC-VF.ttf`。生成的 C 链接到 `main`，需启用 `CONFIG_LV_USE_FONT_COMPRESSED=y`。转换依赖与中间静态字体放在被忽略的 `build/`。
