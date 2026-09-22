idf_component_register(
    SRCS "main.c" "sp_core.c" "sp_nav.c" "sp_store.c" "sp_web.c" "sp_ble.c" "sp_ui.c"
         "../assets/fonts/sp_font_16.c" "../assets/fonts/sp_font_ui_16.c" "../assets/fonts/sp_font_24.c"
    INCLUDE_DIRS "."
    REQUIRES bsp bt esp_event esp_hw_support esp_netif esp_timer esp_wifi nvs_flash esp_http_server json
    EMBED_TXTFILES "sp_editor.html"
)
