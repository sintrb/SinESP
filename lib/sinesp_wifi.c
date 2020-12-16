#include "sinesp_wifi.h"
#include "sinesp_gpio.h"
static const char *TAG = "wifi";

static EventGroupHandle_t wifi_event_group;
static ip4_addr_t s_ip_addr;
static int try_connect_fails = 0;
#define ESP_WIFI_AP_SSID "ESP32"
// #define ESP_WIFI_AP_PASS ""
#define WIFI_MAX_STA_CONN 16
#define WIFI_MAX_TRY_COUNT 10

#define CONNECTED_BITS BIT(0)

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

esp_err_t sinesp_wifi_init_softap(const char *ssid, const char *password)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = {0},
            .ssid_len = 0,
            .password = {0},
            .max_connection = WIFI_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    strcpy((char *)wifi_config.ap.ssid, ssid);
    wifi_config.ap.ssid_len = strlen(ssid);
    if (password)
    {
        strcpy((char *)wifi_config.ap.password, password);
    }

    if (!password || strlen(password) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "sinesp_wifi_init_softap finished. SSID:%s password:%s",
             ssid, password);
    return ESP_OK;
}

static void on_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    memcpy(&s_ip_addr, &event->ip_info.ip, sizeof(s_ip_addr));
    try_connect_fails = 0;
    xEventGroupSetBits(wifi_event_group, CONNECTED_BITS);
}

static void on_sinesp_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    ++try_connect_fails;
    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect %d...", try_connect_fails);
    if (try_connect_fails > WIFI_MAX_TRY_COUNT || sinesp_is_key_down())
    {
        xEventGroupSetBits(wifi_event_group, CONNECTED_BITS);
    }
    else
    {
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
}
esp_err_t sinesp_wifi_disconnect()
{
    if (wifi_event_group != NULL)
    {
        vEventGroupDelete(wifi_event_group);
        wifi_event_group = NULL;
        ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_sinesp_wifi_disconnect));
        ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip));
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_deinit());
    }
    return ESP_OK;
}
esp_err_t sinesp_wifi_connect(const char *ssid, const char *password)
{
    ESP_ERROR_CHECK(sinesp_wifi_disconnect());
    wifi_event_group = xEventGroupCreate();
    try_connect_fails = 0;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_sinesp_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
        },
    };
    strcpy((char *)wifi_config.sta.ssid, ssid);
    if (password)
    {
        strcpy((char *)wifi_config.sta.password, password);
    }
    ESP_LOGI(TAG, "Connecting to SSID:%s password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    xEventGroupWaitBits(wifi_event_group, CONNECTED_BITS, true, true, portMAX_DELAY);
    if (!try_connect_fails)
    {
        ESP_LOGI(TAG, "Connected to %s", ssid);
        ESP_LOGI(TAG, "IPv4 address: " IPSTR, IP2STR(&s_ip_addr));
        return ESP_OK;
    }
    else
    {
        ESP_LOGE(TAG, "Connected to %s failed!!!", ssid);
        return ESP_FAIL;
    }
}

esp_err_t sinesp_wifi_init_with_config(int ap)
{
    /*
    根据配置，优先进入STA模式；
    如果尝试WIFI_MAX_TRY_COUNT次未能连接（或者按下用户键），则进入AP模式；
    进入AP模式时，如果长按用户键则设置为无密码的AP热点。
    */

    const char *ssid, *pswd;
    sinesp_led_set_time(200, 200);
    if (!ap && sinespConfigJson)
    {
        cJSON *wifi = cJSON_GetObjectItem(sinespConfigJson, "wifi");
        if (wifi)
        {
            ssid = cJSON_GetStringValue(cJSON_GetObjectItem(wifi, "ssid"));
            pswd = cJSON_GetStringValue(cJSON_GetObjectItem(wifi, "pswd"));
            if (ssid && strlen(ssid))
            {
                ESP_LOGI(TAG, "init Wi-Fi with STA");
                if (sinesp_wifi_connect(ssid, pswd) == ESP_OK)
                {
                    return ESP_OK;
                }
                sinesp_wifi_disconnect();
            }
            ssid = NULL;
            pswd = NULL;
        }
    }
    if (sinespConfigJson)
    {
        cJSON *wifi = cJSON_GetObjectItem(sinespConfigJson, "ap");
        if (wifi)
        {
            ssid = cJSON_GetStringValue(cJSON_GetObjectItem(wifi, "ssid"));
            pswd = cJSON_GetStringValue(cJSON_GetObjectItem(wifi, "pswd"));
        }
    }
    if (!ssid || !strlen(ssid))
    {
        ssid = ESP_WIFI_AP_SSID;
    }
    if (sinesp_is_key_down() || !pswd)
        pswd = "";
    ESP_LOGI(TAG, "init Wi-Fi with AP");
    return sinesp_wifi_init_softap(ssid, pswd);
}