#include "lvgl.h"
#include "sp_ui.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint16_t pixels[240 * 320], draw[240 * 20];
static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *bytes) {
    const uint16_t *p = (const uint16_t *)bytes;
    for (int y = a->y1; y <= a->y2; y++)
        for (int x = a->x1; x <= a->x2; x++)
            pixels[y * 240 + x] = *p++;
    lv_display_flush_ready(d);
}
int main(void) {
    lv_init();
    lv_display_t *d = lv_display_create(240, 320);
    lv_display_set_color_format(d, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d, draw, NULL, sizeof draw, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d, flush);
    static sp_view_t v;
    v.battery = 85;
    v.radio_ready = true;
    v.nav.people = 3;
    v.dim = true;
    strcpy(v.card.text[0], "阿七");
    strcpy(v.card.text[1], "产品设计师");
    strcpy(v.card.text[2], "周末一起逛电子市场吗？");
    strcpy(v.card.text[3], "火锅,开源硬件,摄影,散步");
    strcpy(v.card.text[4], "hello@example.com");
    strcpy(v.card.text[5], "@aqi_demo");
    strcpy(v.card.text[6], "aqi_demo");
    v.own = v.card;
    strcpy(v.own.text[0], "小林");
    strcpy(v.own.text[1], "软件开发者");
    strcpy(v.own.text[3], "火锅,摄影");
    const char *names[] = {"阿七", "小满", "木木"};
    for (int i = 0; i < 3; i++) {
        strcpy(v.index[i].name, names[i]);
        v.index[i].occupied = true;
        v.order[i] = i;
    }
    v.index[0].unread = true;
    v.wifi_qr = "WIFI:T:WPA;S:StreetPass-Preview;P:preview-only-123;;";
    v.ssid = "StreetPass-Preview";
    strcpy(v.notice, "遇见了阿七");
    const sp_page_t pages[] = {SP_HOME,     SP_INBOX,    SP_CARD,   SP_CARD,
                               SP_CONTACTS, SP_QR,       SP_OWN,    SP_EDIT_WIFI,
                               SP_EDIT_URL, SP_SETTINGS, SP_NOTICE, SP_DELETE};
    for (size_t i = 0; i < sizeof pages / sizeof pages[0]; i++) {
        v.nav.page = pages[i];
        v.nav.tab = i == 3 ? 1 : 0;
        sp_ui_render(&v);
        for (int j = 0; j < 5; j++) {
            lv_tick_inc(35);
            lv_timer_handler();
        }
        char path[128];
        snprintf(path, sizeof path, "build/preview/screen-%02zu.ppm", i + 1);
        FILE *f = fopen(path, "wb");
        if (!f)
            return 2;
        fprintf(f, "P6\n240 320\n255\n");
        for (int k = 0; k < 240 * 320; k++) {
            uint16_t p = pixels[k];
            uint8_t rgb[] = {(uint8_t)((p >> 11) * 255 / 31), (uint8_t)(((p >> 5) & 63) * 255 / 63),
                             (uint8_t)((p & 31) * 255 / 31)};
            fwrite(rgb, 1, 3, f);
        }
        fclose(f);
    }
    /* Long top titles remain fixed and use ellipsis; body paragraphs still scroll. */
    v.nav.page = SP_CARD;
    v.nav.tab = 0;
    strcpy(v.card.text[0], "这个名字很长请不要滚动显示");
    v.card.text[1][0] = 0;
    v.card.text[2][0] = 0;
    sp_ui_render(&v);
    bool fixed_heading = false;
    lv_obj_t *title_root = lv_screen_active();
    for (uint32_t i = 0; i < lv_obj_get_child_count(title_root); i++) {
        lv_obj_t *item = lv_obj_get_child(title_root, i);
        if (lv_obj_check_type(item, &lv_label_class) && lv_obj_get_y(item) == 35) {
            lv_point_t extent;
            lv_text_get_size(&extent, v.card.text[0], lv_obj_get_style_text_font(item, 0), 0, 0,
                             LV_COORD_MAX, LV_TEXT_FLAG_NONE);
            assert(extent.x > lv_obj_get_width(item));
            assert(lv_label_get_long_mode(item) == LV_LABEL_LONG_DOT);
            fixed_heading = true;
        }
    }
    assert(fixed_heading);
    /* Exercise valid maximum-length ASCII paragraphs, then verify they scroll inside a viewport. */
    v.nav.page = SP_CARD;
    v.nav.tab = 0;
    memset(v.card.text[1], 'W', 96);
    v.card.text[1][96] = 0;
    memset(v.card.text[2], 'W', 144);
    v.card.text[2][144] = 0;
    sp_ui_render(&v);
    for (int frame = 0; frame < 20; frame++) {
        lv_tick_inc(250);
        lv_timer_handler();
    }
    bool moving = false;
    lv_obj_t *root = lv_screen_active();
    for (uint32_t i = 0; i < lv_obj_get_child_count(root); i++) {
        lv_obj_t *o = lv_obj_get_child(root, i);
        if (lv_obj_get_child_count(o) == 1 && lv_obj_get_y(lv_obj_get_child(o, 0)) < 0)
            moving = true;
    }
    assert(moving);
    /* Recreate pages with maximum text and empty states, checking LVGL memory
     * recovery after animation/QR teardown in the same 40 KB pool as firmware. */
    strcpy(v.card.text[0], "长昵称也需要完整地显示在屏幕里");
    v.own = v.card;
    v.battery = -1;
    v.nav.people = 0;
    v.card.text[4][0] = 0;
    size_t settled_free = 0;
    for (int cycle = 0; cycle < 20; cycle++) {
        for (size_t i = 0; i < sizeof pages / sizeof pages[0]; i++) {
            v.nav.page = pages[i];
            v.nav.tab = i == 3 ? 1 : 0;
            sp_ui_render(&v);
            lv_tick_inc(35);
            lv_timer_handler();
            assert(lv_mem_test() == LV_RESULT_OK);
        }
        v.nav.page = SP_HOME;
        sp_ui_render(&v);
        lv_tick_inc(35);
        lv_timer_handler();
        lv_mem_monitor_t current;
        lv_mem_monitor(&current);
        assert(current.free_biggest_size >= 8192);
        if (cycle == 1)
            settled_free = current.free_size;
        if (cycle > 1)
            assert(current.free_size == settled_free);
    }
    puts("Long text, empty states and 20 page cycles: PASS");
    lv_mem_monitor_t mem;
    lv_mem_monitor(&mem);
    printf("12 actual LVGL screens rendered; memory used %u%%, largest free block %zu\n",
           mem.used_pct, mem.free_biggest_size);
    return 0;
}
