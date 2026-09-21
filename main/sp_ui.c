#include "sp_ui.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>
LV_FONT_DECLARE(sp_font_16);
LV_FONT_DECLARE(sp_font_24);
#define BG 0x0D0D11
#define SURFACE 0x141418
#define BLUE 0x00AEEF
#define CYAN 0x00F0FF
#define TEXT 0xD0D0D0
#define MUTED 0xA0A0A0
static lv_obj_t *screen;
static lv_font_t body_font, title_font;
static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, int radius) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}
static void scroll_body(void *obj, int32_t y) {
    lv_obj_set_y(obj, y);
}
static lv_obj_t *text(lv_obj_t *parent, int x, int y, int w, int h, const char *s, uint32_t color,
                      bool title) {
    bool paragraph = !title && h > 26;
    if (paragraph) {
        lv_obj_t *viewport = lv_obj_create(parent);
        lv_obj_remove_style_all(viewport);
        lv_obj_set_pos(viewport, x, y);
        lv_obj_set_size(viewport, w, h);
        lv_obj_remove_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);
        parent = viewport;
        x = y = 0;
    }
    lv_obj_t *o = lv_label_create(parent);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_text_font(o, title ? &title_font : &body_font, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(color), 0);
    lv_label_set_long_mode(o, h <= 26 ? LV_LABEL_LONG_SCROLL_CIRCULAR : LV_LABEL_LONG_WRAP);
    lv_label_set_text(o, s);
    if (paragraph) {
        lv_obj_set_height(o, LV_SIZE_CONTENT);
        lv_obj_update_layout(o);
        int32_t overflow = lv_obj_get_height(o) - h;
        if (overflow > 0) {
            lv_anim_t animation;
            lv_anim_init(&animation);
            lv_anim_set_var(&animation, o);
            lv_anim_set_exec_cb(&animation, scroll_body);
            lv_anim_set_values(&animation, 0, -overflow);
            lv_anim_set_duration(&animation, (uint32_t)overflow * 40 + 1000);
            lv_anim_set_delay(&animation, 1500);
            lv_anim_set_reverse_duration(&animation, (uint32_t)overflow * 40 + 1000);
            lv_anim_set_reverse_delay(&animation, 1500);
            lv_anim_set_repeat_delay(&animation, 1500);
            lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
            lv_anim_start(&animation);
        }
    }
    return o;
}
static void heading(const char *s) {
    text(screen, 14, 32, 212, 34, s, TEXT, true);
}
static void foot(const char *s) {
    lv_obj_t *o = text(screen, 8, 294, 224, 22, s, MUTED, false);
    lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_CENTER, 0);
}
static void row(int y, int h, const char *label, const char *sub, bool selected) {
    lv_obj_t *o = box(screen, 10, y, 220, h, selected ? 0x102B36 : SURFACE, 10);
    if (selected)
        box(o, 0, 10, 3, h - 20, BLUE, 2);
    text(o, 14, sub ? 6 : (h - 22) / 2, 185, 22, label, TEXT, false);
    if (sub)
        text(o, 14, 29, 184, 22, sub, MUTED, false);
    text(o, 202, (h - 22) / 2, 12, 22, ">", selected ? CYAN : MUTED, false);
}
static void envelope(int x, int y) {
    lv_obj_t *o = box(screen, x, y, 44, 30, BG, 6);
    lv_obj_set_style_border_width(o, 2, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(BLUE), 0);
    box(o, 12, 12, 4, 4, TEXT, 2);
    box(o, 28, 12, 4, 4, TEXT, 2);
    box(screen, x + 10, y + 30, 5, 4, BLUE, 1);
    box(screen, x + 30, y + 30, 5, 4, BLUE, 1);
}
static void qr_code(const char *data) {
    if (!data || !*data) {
        text(screen, 18, 132, 204, 64, "尚未填写这一项", MUTED, false);
        return;
    }
    lv_obj_t *q = lv_qrcode_create(screen);
    lv_obj_set_pos(q, 30, 82);
    lv_qrcode_set_size(q, 180);
    lv_qrcode_set_quiet_zone(q, true);
    lv_qrcode_set_dark_color(q, lv_color_hex(BG));
    lv_qrcode_set_light_color(q, lv_color_hex(0xFFFFFF));
    if (lv_qrcode_update(q, data, strlen(data)) != LV_RESULT_OK) {
        lv_obj_delete(q);
        text(screen, 18, 132, 204, 64, "内容太长，无法生成二维码", MUTED, false);
    }
}
static bool tag_match(const char *all, const char *word) {
    size_t len = strlen(word);
    const char *p = all;
    while (*p) {
        while (*p == ' ' || *p == ',')
            p++;
        const char *end = strchr(p, ',');
        if (!end)
            end = p + strlen(p);
        const char *trim = end;
        while (trim > p && trim[-1] == ' ')
            trim--;
        if ((size_t)(trim - p) == len && !memcmp(p, word, len))
            return true;
        p = end;
        if (*p)
            p++;
    }
    return false;
}
static void interests(const sp_view_t *v) {
    text(screen, 14, 80, 212, 22, "我们都喜欢", MUTED, false);
    char all[SP_TEXT_CAP], common[SP_TEXT_CAP] = "";
    strcpy(all, v->card.text[3]);
    char *save = NULL;
    for (char *p = strtok_r(all, ",", &save); p; p = strtok_r(NULL, ",", &save)) {
        while (*p == ' ')
            p++;
        size_t n = strlen(p);
        while (n && p[n - 1] == ' ')
            p[--n] = 0;
        if (n && tag_match(v->own.text[3], p) && strlen(common) + n + 3 < sizeof common) {
            if (*common)
                strcat(common, " / ");
            strcat(common, p);
        }
    }
    text(screen, 14, 108, 212, 68, *common ? common : "还没有相同标签", *common ? CYAN : TEXT,
         false);
    text(screen, 14, 188, 212, 22, "TA 的兴趣", MUTED, false);
    text(screen, 14, 218, 212, 68, *v->card.text[3] ? v->card.text[3] : "还没填写", TEXT, false);
}
void sp_ui_render(const sp_view_t *v) {
    if (!screen) {
        body_font = sp_font_16;
        body_font.line_height = 22;
        body_font.base_line = 5;
        body_font.fallback = &lv_font_montserrat_14;
        title_font = sp_font_24;
        title_font.fallback = &body_font;
        screen = lv_obj_create(NULL);
        lv_obj_remove_style_all(screen);
        lv_obj_set_size(screen, 240, 320);
        lv_obj_set_style_bg_color(screen, lv_color_hex(BG), 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
        lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_screen_load(screen);
    }
    lv_obj_clean(screen);
    char buf[96];
    snprintf(buf, sizeof buf, v->battery >= 0 ? "%d%%" : "--", v->battery);
    text(screen, 184, 4, 50, 22, buf, MUTED, false);
    const char *contact_names[] = {"邮箱", "Telegram", "微信"};
    switch (v->nav.page) {
    case SP_HOME:
        heading("擦肩");
        envelope(173, 43);
        text(screen, 15, 77, 210, 22,
             *v->status ? v->status
                        : (v->paused        ? "已暂停擦肩"
                           : v->radio_ready ? "擦肩中 · 好朋友正在路上"
                                            : "蓝牙启动中…"),
             v->paused ? MUTED : BLUE, false);
        snprintf(buf, sizeof buf, "名片列表  %d", v->nav.people);
        row(111, 40, buf, NULL, v->nav.selected == 0);
        row(157, 40, "我的名片", NULL, v->nav.selected == 1);
        row(203, 40, "扫码编辑", NULL, v->nav.selected == 2);
        row(249, 36, "设置", NULL, v->nav.selected == 3);
        foot("上下选择 · OK 打开");
        break;
    case SP_INBOX: {
        heading("名片列表");
        if (!v->nav.people) {
            envelope(98, 126);
            text(screen, 25, 189, 198, 66, "带上设备出门走走\n下一次相遇，正在路上", MUTED, false);
        } else {
            int start = (v->nav.selected / 3) * 3;
            for (int j = 0; j < 3 && start + j < v->nav.people; j++) {
                const sp_index_t *i = &v->index[v->order[start + j]];
                snprintf(buf, sizeof buf, "%s%s%s", i->favorite ? "* " : "", i->name,
                         i->unread ? "  · 新" : "");
                row(83 + j * 67, 59, buf, i->favorite ? "已收藏" : "公开名片",
                    v->nav.selected == start + j);
            }
        }
        foot("上下选择 · 长按 OK 返回");
        break;
    }
    case SP_OWN:
        heading("我的名片");
        text(screen, 16, 82, 208, 24, v->own.text[0], TEXT, false);
        text(screen, 16, 116, 208, 66, *v->own.text[1] ? v->own.text[1] : "写一点自己吧", MUTED,
             false);
        row(193, 40, "编辑名片", NULL, v->nav.selected == 0);
        row(241, 40, "预览公开内容", NULL, v->nav.selected == 1);
        foot("上下选择 · OK 打开");
        break;
    case SP_CARD:
        heading(v->nav.tab == 0 ? "名片简介" : v->nav.tab == 1 ? "兴趣" : "联系方式");
        if (v->nav.tab == 0) {
            text(screen, 14, 80, 212, 26, v->card.text[0], BLUE, false);
            text(screen, 14, 112, 212, 66, v->card.text[1], TEXT, false);
            text(screen, 14, 184, 212, 88,
                 *v->card.text[2] ? v->card.text[2] : "这个人有点神秘，还没留言。", TEXT, false);
        }
        if (v->nav.tab == 1)
            interests(v);
        if (v->nav.tab == 2) {
            text(screen, 14, 86, 212, 60, "OK 查看联系二维码", BLUE, false);
            text(screen, 14, 160, 212, 104,
                 v->nav.own ? "这些内容会公开交换。" : "长按上键：收藏 / 取消\n长按下键：删除名片",
                 MUTED, false);
        }
        if (v->nav.tab == 2 && !v->nav.own && v->active_slot >= 0)
            text(screen, 14, 267, 212, 22,
                 v->index[v->active_slot].favorite ? "* 已收藏" : "未收藏", BLUE, false);
        foot("上下翻页 · 长按 OK 返回");
        break;
    case SP_CONTACTS:
        heading("联系方式");
        for (int j = 0; j < 3; j++)
            row(83 + j * 67, 59, contact_names[j],
                *v->card.text[j + 4] ? v->card.text[j + 4] : "未公开", v->nav.selected == j);
        foot("上下选择 · OK 看二维码");
        break;
    case SP_QR:
        heading(contact_names[v->nav.contact]);
        qr_code(v->card.text[v->nav.contact + 4]);
        text(screen, 14, 264, 212, 22, v->card.text[v->nav.contact + 4], MUTED, false);
        foot("扫描获取文字 · OK 返回");
        break;
    case SP_EDIT_WIFI:
        heading("扫码编辑");
        qr_code(v->wifi_qr);
        text(screen, 14, 266, 212, 22, "手机相机扫码，确认入网", TEXT, false);
        foot("未弹出？按 OK · 长按退出");
        break;
    case SP_EDIT_URL:
        heading("打开编辑页");
        qr_code("http://192.168.4.1");
        text(screen, 14, 265, 212, 22, "先连接设备热点再扫描", TEXT, false);
        foot("192.168.4.1 · 长按 OK 退出");
        break;
    case SP_SETTINGS:
        heading("设置");
        snprintf(buf, sizeof buf, "擦肩模式          %s", v->paused ? "关" : "开");
        row(82, 43, buf, NULL, v->nav.selected == 0);
        snprintf(buf, sizeof buf, "提示音              %s", v->sound ? "开" : "关");
        row(132, 43, buf, NULL, v->nav.selected == 1);
        snprintf(buf, sizeof buf, "30 秒调暗          %s", v->dim ? "开" : "关");
        row(182, 43, buf, NULL, v->nav.selected == 2);
        row(232, 43, "管理名片", NULL, v->nav.selected == 3);
        foot(*v->status ? v->status : "上下选择 · OK 更改");
        break;
    case SP_DELETE:
        heading("删除名片");
        text(screen, 14, 92, 212, 78, "删除后，下一次相遇\n仍然可以重新收到。", MUTED, false);
        row(190, 40, "保留名片", NULL, v->nav.selected == 0);
        row(240, 40, "确认删除", NULL, v->nav.selected == 1);
        foot("上下选择 · OK 确认");
        break;
    case SP_NOTICE:
        heading("收到名片");
        envelope(98, 100);
        text(screen, 20, 168, 200, 28, v->notice, CYAN, false);
        text(screen, 20, 210, 200, 48, "已保存到名片列表", TEXT, false);
        foot("稍后自动收起 · OK 查看");
        break;
    case SP_ERROR:
        heading("设备异常");
        text(screen, 14, 100, 212, 154, v->status, TEXT, false);
        foot("请重启后重试");
        break;
    }
}
