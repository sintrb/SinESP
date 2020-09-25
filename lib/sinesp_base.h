#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_http_server.h"
#include "cJSON.h"

#define APP_NAME "XSerial"
#define MAX_HTTP_BODY_SIZE 2048
#define CONFIG_STORAGE_NAMESPACE "config"
#define CONFIG_DEFAULT_WIFI_SSID "NOT-CMCC"
#define CONFIG_DEFAULT_WIFI_PASSWORD "qwe123!@#"
#define CONFIG_TYPE_VERSION 0x0003
#define TOGGLE_VALUE(var, v1, v2) (var = (var) == (v1)? (v2): (v2))

typedef struct
{
    // Start 必须保留
    int version;
    int updated;
    size_t size;
    char app_version[16];
    // End 必须保留

    // char wifi_ssid[64];
    // char wifi_password[64];
    // char index_html[1024];
} sinesp_config_base_t;

extern sinesp_config_base_t sinespConfigBase;
extern cJSON *sinespConfigJson;

extern EventGroupHandle_t sinesp_event_group;
static const int SINESP_EVENT_BIT_sinesp_wifi_connectING = BIT0;
static const int SINESP_EVENT_BIT_sinesp_wifi_connectED = BIT1;
static const int SINESP_EVENT_BIT_AIRKISS_START = BIT2;
static const int SINESP_EVENT_BIT_MQTT_CONNECTING = BIT3;
static const int SINESP_EVENT_BIT_MQTT_CONNECTED = BIT4;
static const int SINESP_EVENT_BIT_MQTT_RECEIVED = BIT5;

static const int SINESP_EVENT_BIT_LED_UPDATED = BIT10;
static const int SINESP_EVENT_BIT_MQTT_TASK = BIT11;
static const int SINESP_EVENT_BIT_sinesp_wifi_disconnectED = BIT12;
static const int SINESP_EVENT_BIT_TEST = BIT15;

#define SinESPSendEvent(bits) xEventGroupSetBits(sinesp_event_group, bits)
#define SinESPWaitEvent(bits, wait) xEventGroupWaitBits(sinesp_event_group, bits, false, true, wait)
#define SinESPUntilEvent(bits) xEventGroupWaitBits(sinesp_event_group, bits, false, true, portMAX_DELAY)
#define SinESPClearEvent(bits) xEventGroupClearBits(sinesp_event_group, bits)

esp_err_t sinesp_init();
esp_err_t sinesp_load_config(void);
esp_err_t sinesp_save_config(void);
int sinesp_update_string_from_json(cJSON *root, const char *key, char *buf);
int sinesp_update_int_from_json(cJSON *root, const char *key, int *valp);
esp_err_t sinesp_update_config_with_json_string(const char* jsonbuf);



