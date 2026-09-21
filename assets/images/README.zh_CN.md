<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 图片资源（Images）

本目录存放项目可复用的图片资源，如 UI 图标、背景、RGB565 资源等。

## 如何使用

- 图片文件复制到本目录，并在本项目 `README.md` 记录分辨率、格式、用途与来源。
- 与固件集成时，参考 [`components/bsp/include/bsp_display.h`](../../components/bsp/include/bsp_display.h) 与相关示例分支的图片资源管线，转换为固件所需格式（如 RGB565 数组）。
- 图片资源占用 Flash 与内存，集成前请评估 ESP32-C3 无 PSRAM 的限制。

## 目录说明

> 当前为空骨架，用于存放后续加入的图片资源。加入资源时请同步更新本 `README.md` 的索引。

## 擦肩参考与实际渲染

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
