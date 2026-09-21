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
    lv_mem_monitor_t mem;
    lv_mem_monitor(&mem);
    printf("12 actual LVGL screens rendered; memory used %u%%, largest free block %zu\n",
           mem.used_pct, mem.free_biggest_size);
    return 0;
}
