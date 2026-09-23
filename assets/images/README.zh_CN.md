<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 图片资源（Images）

本目录存放项目可复用的图片资源，如 UI 图标、背景、RGB565 资源等。

## 如何使用

- 图片文件复制到本目录，并在本项目 `README.md` 记录分辨率、格式、用途与来源。
- 与固件集成时，参考 [`components/bsp/include/bsp_display.h`](../../components/bsp/include/bsp_display.h) 与相关示例分支的图片资源管线，转换为固件所需格式（如 RGB565 数组）。
- 图片资源占用 Flash 与内存，集成前请评估 ESP32-C3 无 PSRAM 的限制。

## 早期设计参考与实际渲染

- [确认的色卡参考](streetpass-techdark-palette-reference.png)：由用户提供，作为设计参考保存，不额外声明转载授权。
- [确认的设计总览](streetpass-techdark-ui-overview-v1.png)：AI 生成的视觉提案，二维码是示意图案。
- [实际 LVGL 预览](streetpass-lvgl-preview-v1.png)：由 `main/sp_ui.c` 与虚构资料渲染的 12 个页面。图中热点密码是测试资料，并非真实设备密码。联系、热点及编辑网址二维码已在电脑独立解码验证；这不是设备实拍。
- 其他 `streetpass-*.png` 保留为历史备选；确认的科技深色方案已替代旧配色。
- 使用 [render_streetpass.py](../../tools/render_streetpass.py) 和主机编译器可重现页面图片。这些预览不验证 LCD 亮度、无线行为或手机兼容性。

- [手机编辑页预览](streetpass-phone-editor-v1.png)：真实 HTML 在本机无头 Chrome 中渲染，接口使用虚构测试数据。已检查预览、逗号归一化、联系方式公开开关及连续保存反馈；未涉及真实热点或设备后端。

## 仓库展示第二版

2026-09-21 为擦肩仓库首页新增。这些是 AI 生成的展示素材，与上方真实 LVGL 和手机编辑页预览分别记录。

- [产品封面](streetpass-hero-v2.png)：外壳参考[官方正面图](../../docs/assets/brand/ai-passport-front.png)，功能参考第一版 UI。屏幕内容为概念设计，并非真机照片。
- [四页 UI 提案](streetpass-ui-study-v2.png)：优化字体、分组与留白的视觉提案，目标仍为 240 x 320 实体按键设备。图中二维码不是入网凭证；此图片不改变固件界面或验证状态。
- [擦肩故事插画](streetpass-encounter-story-v2.png)：用涂鸦说明本地编辑、蓝牙相遇与开始交谈。它是场景插画，不能证明实际擦肩成功率。

这些图片按版本保留。不要把生成图片作为真实实现预览使用而不标注概念性质。参考硬件属于 FoloToy；本展示不代表官方发行或背书。

## 实装界面第二版

- [四页预览](streetpass-lvgl-focus-v2.png)和[全部 12 页](streetpass-lvgl-preview-v2.png)：2026-09-22 使用主机渲染器、修改后的 `main/sp_ui.c` 与虚构名片生成。PNG 拼图保留每页 240 x 320 原始分辨率。根据精修稿调整字体、留白、蓝色点缀和信封小角色，沿用实体按键导航。二维码是可正确解码的真实测试资料。这些是软件渲染，并非 LCD 实拍。

## 口袋来信：当前仓库展示

- [口袋来信封面](pocket-letter-hero-v1.png)：在原擦肩封面基础上使用 AI 编辑，保留概念设备构图并替换名称。屏幕是示意图，不是设备实拍。
- [当前四页 LVGL 预览](pocket-letter-lvgl-focus-v1.png)及[全部 12 页](pocket-letter-lvgl-preview-v1.png)：更名后由实际固件以虚构资料和二维码测试数据渲染的 240 x 320 页面，画面中的测试密码不是设备秘密。
- [当前手机编辑页预览](pocket-letter-phone-editor-v1.png)：真实内置编辑页在桌面 Chrome 中使用本地模拟接口渲染。

此前 `streetpass-*` 图片保留为历史设计和实现记录。

## 口袋来信社区封面

- [口袋来信社区封面](pocket-letter-cover-v1.jpg)：AI 生成的涂鸦风插画，描绘虚构人物偶遇并交换名片。于 2026-09-23 为本项目制作，仅用于社区玩法封面，不集成进固件，也不作为真机照片或截图展示。
