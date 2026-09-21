#pragma once
#include "esp_err.h"
/* Start/stop from the application worker. The server owns its tasks until stop returns. */
esp_err_t sp_web_start(void);
void sp_web_stop(void);
const char *sp_web_wifi_qr(void);
const char *sp_web_ssid(void);
