#include "sp_ui.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(sp_font_16);
LV_FONT_DECLARE(sp_font_ui_16);
LV_FONT_DECLARE(sp_font_24);
#define BG 0x0D0D11
#define SURFACE 0x141418
#define SELECTED 0x102B36
#define LINE 0x2A2A31
#define BLUE 0x00AEEF
#define CYAN 0x00F0FF
#define TEXT 0xD0D0D0
#define MUTED 0xA0A0A0
static lv_obj_t *screen;
static lv_font_t body_font, fallback_font, title_font;

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
static void border(lv_obj_t *o, uint32_t color, int width) {
    lv_obj_set_style_border_width(o, width, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(color), 0);
}
static void stroke(lv_obj_t *parent, int x, int y, const lv_point_precise_t *points, int count,
                   uint32_t color, int width) {
    lv_obj_t *o = lv_line_create(parent);
    lv_obj_set_pos(o, x, y);
    lv_line_set_points(o, points, count);
    lv_obj_set_style_line_width(o, width, 0);
    lv_obj_set_style_line_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_line_rounded(o, true, 0);
}
static void scroll_body(void *obj, int32_t y) { lv_obj_set_y(obj, y); }
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
    lv_label_set_long_mode(o, paragraph ? LV_LABEL_LONG_WRAP : LV_LABEL_LONG_SCROLL_CIRCULAR);
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
static void heading(const char *s) { text(screen, 16, 35, 208, 36, s, TEXT, true); }
static void foot(const char *s) {
    lv_obj_t *o = text(screen, 8, 294, 224, 22, s, MUTED, false);
    lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_CENTER, 0);
}
static void separator(int y) { box(screen, 16, y, 208, 1, LINE, 0); }
static void status_bar(const sp_view_t *v) {
    static const lv_point_precise_t bluetooth[] = {{0, 3}, {10, 13}, {5, 17},
                                                   {5, 0}, {10, 4},  {0, 14}};
    bool editing = v->nav.page == SP_EDIT_WIFI || v->nav.page == SP_EDIT_URL;
    if (editing) {
        lv_obj_t *o = text(screen, 16, 7, 46, 20, "Wi-Fi", BLUE, false);
        lv_obj_set_style_text_font(o, &lv_font_montserrat_14, 0);
    } else {
        stroke(screen, 17, 8, bluetooth, 6, v->radio_ready && !v->paused ? BLUE : MUTED, 1);
    }
    char buf[12];
    snprintf(buf, sizeof buf, v->battery >= 0 ? "%d%%" : "--", v->battery);
    lv_obj_t *o = text(screen, 162, 7, 40, 20, buf, MUTED, false);
    lv_obj_set_style_text_font(o, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_RIGHT, 0);
    o = box(screen, 210, 10, 16, 10, BG, 2);
    border(o, MUTED, 1);
    box(screen, 226, 13, 2, 4, MUTED, 1);
    if (v->battery > 0) {
        int amount = v->battery > 100 ? 100 : v->battery;
        int width = (10 * amount + 99) / 100;
        box(o, 3, 3, width, 4, TEXT, 1);
    }
}
/* A value occupies the trailing column; NULL draws a navigation chevron. */
static lv_obj_t *row(int y, int h, const char *label, const char *sub, bool selected,
                     const char *value) {
    static const lv_point_precise_t arrow[] = {{0, 0}, {5, 5}, {0, 10}};
    lv_obj_t *o = box(screen, 12, y, 216, h, selected ? SELECTED : SURFACE, 10);
    if (selected)
        box(o, 0, 10, 3, h - 20, BLUE, 2);
    text(o, 14, sub ? 7 : (h - 22) / 2, 160, 22, label, TEXT, false);
    if (sub)
        text(o, 14, 30, 162, 22, sub, MUTED, false);
    if (value) {
        lv_obj_t *t = text(o, 172, (h - 22) / 2, 28, 22, value, selected ? CYAN : MUTED, false);
        lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_RIGHT, 0);
    } else {
        stroke(o, 198, (h - 10) / 2, arrow, 3, selected ? CYAN : MUTED, 1);
    }
    return o;
}
static void setting(int y, const char *label, bool on, bool selected) {
    lv_obj_t *o = row(y, 43, label, NULL, selected, "");
    lv_obj_t *track = box(o, 171, 13, 30, 17, on ? BLUE : LINE, 9);
    box(track, on ? 16 : 3, 3, 11, 11, TEXT, 6);
}
static void envelope(int x, int y) {
    lv_obj_t *o = box(screen, x, y, 42, 30, BG, 7);
    border(o, BLUE, 2);
    box(o, 11, 12, 4, 4, TEXT, 2);
    box(o, 26, 12, 4, 4, TEXT, 2);
    box(screen, x + 10, y + 30, 4, 5, BLUE, 1);
    box(screen, x + 28, y + 30, 4, 5, BLUE, 1);
}
static void page_dots(int selected) {
    for (int i = 0; i < 3; i++)
        box(screen, 100 + i * 15, 281, 10, 3, i == selected ? BLUE : LINE, 2);
}
/* Chips are an overview only. The interest page retains the full scrolling
 * field. */
static void chips(int y, const char *tags, int limit) {
    char copy[SP_TEXT_CAP];
    snprintf(copy, sizeof copy, "%s", tags);
    char *save = NULL;
    int x = 16;
    for (char *p = strtok_r(copy, ",", &save); p && limit > 0; p = strtok_r(NULL, ",", &save)) {
        while (*p == ' ')
            p++;
        size_t len = strlen(p);
        while (len && p[len - 1] == ' ')
            p[--len] = 0;
        if (!len)
            continue;
        lv_point_t size;
        lv_text_get_size(&size, p, &body_font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        int w = size.x + 20;
        if (w > 100)
            w = 100;
        if (x + w > 224)
            break;
        lv_obj_t *o = box(screen, x, y, w, 28, BG, 14);
        border(o, 0x246077, 1);
        text(o, 10, 3, w - 20, 22, p, BLUE, false);
        x += w + 8;
        limit--;
    }
}
static void qr_code(const char *data, int y, int size) {
    if (!data || !*data) {
        text(screen, 18, 132, 204, 64, "尚未填写这一项", MUTED, false);
        return;
    }
    lv_obj_t *q = lv_qrcode_create(screen);
    lv_obj_set_pos(q, (240 - size) / 2, y);
    lv_qrcode_set_size(q, size);
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
    text(screen, 16, 83, 208, 22, "我们都喜欢", MUTED, false);
    char all[SP_TEXT_CAP], common[SP_TEXT_CAP] = "";
    strcpy(all, v->card.text[3]);
    char *save = NULL;
    for (char *p = strtok_r(all, ",", &save); p; p = strtok_r(NULL, ",", &save)) {
        while (*p == ' ')
            p++;
        size_t n = strlen(p);
        while (n && p[n - 1] == ' ')
            p[--n] = 0;
        if (n && tag_match(v->own.text[3], p) && strlen(common) + n + 2 < sizeof common) {
            if (*common)
                strcat(common, ",");
            strcat(common, p);
        }
    }
    if (*common) {
        chips(117, common, 2);
        /* Keep every match readable when there are more than two summary chips. */
        const char *second = strchr(common, ',');
        if (second && strchr(second + 1, ','))
            text(screen, 16, 151, 208, 22, common, BLUE, false);
    } else {
        text(screen, 16, 118, 208, 44, "还没有相同标签", TEXT, false);
    }
    separator(185);
    text(screen, 16, 200, 208, 22, "TA 的兴趣", MUTED, false);
    text(screen, 16, 231, 208, 44, *v->card.text[3] ? v->card.text[3] : "还没填写", TEXT, false);
}
void sp_ui_render(const sp_view_t *v) {
    if (!screen) {
        fallback_font = sp_font_16;
        fallback_font.line_height = 22;
        fallback_font.base_line = 5;
        fallback_font.fallback = &lv_font_montserrat_14;
        body_font = sp_font_ui_16;
        body_font.line_height = 22;
        body_font.base_line = 5;
        body_font.fallback = &fallback_font;
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
    status_bar(v);
    char buf[24];
    const char *contact_names[] = {"邮箱", "Telegram", "微信"};
    switch (v->nav.page) {
    case SP_HOME:
        heading("擦肩");
        envelope(176, 52);
        text(screen, 16, 78, 151, 22,
             *v->status ? v->status
                        : (v->paused        ? "已暂停擦肩"
                           : v->radio_ready ? "等待下一次相遇"
                                            : "蓝牙启动中…"),
             MUTED, false);
        snprintf(buf, sizeof buf, "%d", v->nav.people);
        row(112, 38, "名片列表", NULL, v->nav.selected == 0, buf);
        row(156, 38, "我的名片", NULL, v->nav.selected == 1, NULL);
        row(200, 38, "扫码编辑", NULL, v->nav.selected == 2, NULL);
        row(244, 38, "设置", NULL, v->nav.selected == 3, NULL);
        foot("上下选择 · OK 打开");
        break;
    case SP_INBOX:
        heading("名片列表");
        if (!v->nav.people) {
            envelope(99, 126);
            lv_obj_t *o = text(screen, 20, 189, 200, 66, "带上设备出门走走\n下一次相遇，正在路上",
                               MUTED, false);
            lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_CENTER, 0);
        } else {
            int start = (v->nav.selected / 3) * 3;
            for (int j = 0; j < 3 && start + j < v->nav.people; j++) {
                const sp_index_t *i = &v->index[v->order[start + j]];
                lv_obj_t *o = row(83 + j * 67, 59, i->name, i->favorite ? "已收藏" : "公开名片",
                                  v->nav.selected == start + j, NULL);
                if (i->unread)
                    box(o, 179, 15, 6, 6, CYAN, 3);
            }
        }
        foot("上下选择 · 长按 OK 返回");
        break;
    case SP_OWN:
        heading("我的名片");
        text(screen, 16, 83, 208, 30, v->own.text[0], TEXT, true);
        text(screen, 16, 123, 208, 44, *v->own.text[1] ? v->own.text[1] : "写一点自己吧", MUTED,
             false);
        separator(183);
        row(202, 36, "编辑名片", NULL, v->nav.selected == 0, NULL);
        row(246, 36, "预览公开内容", NULL, v->nav.selected == 1, NULL);
        foot("上下选择 · OK 打开");
        break;
    case SP_CARD:
        if (v->nav.tab == 0) {
            heading(v->card.text[0]);
            text(screen, 16, 81, 208, 44, v->card.text[1], MUTED, false);
            separator(133);
            text(screen, 16, 150, 208, 66,
                 *v->card.text[2] ? v->card.text[2] : "这个人有点神秘，还没留言。", TEXT, false);
            chips(234, v->card.text[3], 2);
        } else if (v->nav.tab == 1) {
            heading("兴趣");
            interests(v);
        } else {
            heading("联系方式");
            row(88, 59, "联系二维码", "按 OK 查看", true, NULL);
            text(screen, 16, 169, 208, 78,
                 v->nav.own ? "这些内容会公开交换。" : "长按上键：收藏 / 取消\n长按下键：删除名片",
                 MUTED, false);
            if (!v->nav.own && v->active_slot >= 0)
                text(screen, 16, 253, 208, 22,
                     v->index[v->active_slot].favorite ? "已收藏" : "未收藏", BLUE, false);
        }
        page_dots(v->nav.tab);
        foot("上下翻页 · 长按 OK 返回");
        break;
    case SP_CONTACTS:
        heading("联系方式");
        for (int j = 0; j < 3; j++)
            row(83 + j * 67, 59, contact_names[j],
                *v->card.text[j + 4] ? v->card.text[j + 4] : "未公开", v->nav.selected == j, NULL);
        foot("上下选择 · OK 看二维码");
        break;
    case SP_QR:
        heading(contact_names[v->nav.contact]);
        qr_code(v->card.text[v->nav.contact + 4], 82, 180);
        text(screen, 16, 266, 208, 22, v->card.text[v->nav.contact + 4], MUTED, false);
        foot("扫描获取文字 · OK 返回");
        break;
    case SP_EDIT_WIFI:
    case SP_EDIT_URL: {
        bool wifi = v->nav.page == SP_EDIT_WIFI;
        heading(wifi ? "扫码编辑" : "打开编辑页");
        text(screen, 16, 76, 208, 22, wifi ? "用手机写下你的名片" : "已连接热点？扫码打开", MUTED,
             false);
        qr_code(wifi ? v->wifi_qr : "http://192.168.4.1", 105, 156);
        lv_obj_t *o =
            text(screen, 16, 267, 208, 22, wifi ? "扫码加入设备热点" : "192.168.4.1", TEXT, false);
        lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_CENTER, 0);
        foot(wifi ? "未弹出？按 OK · 长按退出" : "按 OK 切换 · 长按退出");
        break;
    }
    case SP_SETTINGS:
        heading("设置");
        setting(82, "擦肩模式", !v->paused, v->nav.selected == 0);
        setting(132, "提示音", v->sound, v->nav.selected == 1);
        setting(182, "30 秒调暗", v->dim, v->nav.selected == 2);
        row(232, 43, "管理名片", NULL, v->nav.selected == 3, NULL);
        foot(*v->status ? v->status : "上下选择 · OK 更改");
        break;
    case SP_DELETE:
        heading("删除名片");
        separator(82);
        text(screen, 16, 106, 208, 66, "删除后，下一次相遇\n仍然可以重新收到。", MUTED, false);
        row(196, 38, "保留名片", NULL, v->nav.selected == 0, NULL);
        row(244, 38, "确认删除", NULL, v->nav.selected == 1, NULL);
        foot("上下选择 · OK 确认");
        break;
    case SP_NOTICE: {
        heading("收到名片");
        envelope(99, 106);
        lv_obj_t *o = text(screen, 20, 178, 200, 30, v->notice, TEXT, true);
        lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_CENTER, 0);
        o = text(screen, 20, 221, 200, 22, "已保存到名片列表", MUTED, false);
        lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_CENTER, 0);
        foot("稍后自动收起 · OK 查看");
        break;
    }
    case SP_ERROR:
        heading("设备异常");
        separator(82);
        text(screen, 16, 103, 208, 154, v->status, TEXT, false);
        foot("请重启后重试");
        break;
    }
}
