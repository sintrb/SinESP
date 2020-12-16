#include "esp_event_loop.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"
#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mdns.h"
// #include "protocol_examples_common.h"
#include "sdkconfig.h"
#include "sinesp/sinesp_base.h"
#include "sinesp/sinesp_gpio.h"
#include "sinesp/sinesp_wifi.h"
#include "sinesp/sinesp_http.h"
#include "sinesp/sinesp_oled.h"

#define CONFIG_EXAMPLE_MDNS_HOST_NAME "sinesp"
#define MDNS_INSTANCE "esp home web server"
#define TAG APP_NAME

static void initialise_mdns(void)
{
    mdns_init();
    mdns_hostname_set(CONFIG_EXAMPLE_MDNS_HOST_NAME);
    mdns_instance_name_set(MDNS_INSTANCE);

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}};

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

void app_main()
{
    int waitSec = 0, onTimes = 0;
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    sinesp_init();

    sinesp_init_gpio();
    sinesp_led_set_time(1000, 0);
    sinesp_oled_init();

    ESP_ERROR_CHECK(sinesp_load_config());


    for (waitSec = 5; waitSec; --waitSec)
    {
        if (sinesp_is_key_down())
        {
            ++onTimes;
            ESP_LOGI(TAG, "onTimes: %d", onTimes);
        }
        if (onTimes >= 2)
        {
            break;
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    ESP_LOGI(TAG, "onTimes: %d", onTimes);
    // led_set_time(200, 200);
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    initialise_mdns();
    ESP_ERROR_CHECK(sinesp_wifi_init_with_config(onTimes > 2 ? 1 : 0));

    sinesp_led_set_time(1000, 1000);
    ESP_ERROR_CHECK(sinesp_start_http_server());
    ESP_LOGI(TAG, "Running....");
}
