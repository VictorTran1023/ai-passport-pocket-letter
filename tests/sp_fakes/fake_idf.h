#pragma once
#include <stddef.h>
#include <stdint.h>
typedef int esp_err_t;
typedef uint32_t nvs_handle_t;
typedef void *SemaphoreHandle_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_NO_MEM 1
#define ESP_ERR_INVALID_ARG 2
#define ESP_ERR_INVALID_STATE 3
#define ESP_ERR_INVALID_CRC 4
#define ESP_ERR_NVS_NOT_FOUND 5
#define NVS_READWRITE 1
#define portMAX_DELAY UINT32_MAX
SemaphoreHandle_t xSemaphoreCreateMutex(void);
int xSemaphoreTake(SemaphoreHandle_t,uint32_t);
int xSemaphoreGive(SemaphoreHandle_t);
void esp_fill_random(void *,size_t);
esp_err_t nvs_flash_init_partition(const char *);
esp_err_t nvs_open_from_partition(const char *,const char *,int,nvs_handle_t *);
esp_err_t nvs_get_blob(nvs_handle_t,const char *,void *,size_t *);
esp_err_t nvs_set_blob(nvs_handle_t,const char *,const void *,size_t);
esp_err_t nvs_commit(nvs_handle_t);
esp_err_t nvs_erase_key(nvs_handle_t,const char *);
esp_err_t nvs_get_u8(nvs_handle_t,const char *,uint8_t *);
esp_err_t nvs_set_u8(nvs_handle_t,const char *,uint8_t);
