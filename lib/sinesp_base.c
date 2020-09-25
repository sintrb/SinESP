#include "sinesp_base.h"
#include "sinesp_gpio.h"
static const char* TAG = "base";

sinesp_config_base_t sinespConfigBase = {0};
cJSON *sinespConfigJson = NULL;
EventGroupHandle_t sinesp_event_group;

#define STORE_CONFIG_KEY_BASE "base"
#define CONFIG_STORAGE_KEY_JSON "json"

int sinesp_update_string_from_json(cJSON *root, const char *key, char *buf)
{
    cJSON *item = cJSON_GetObjectItem(root, key);
    if (item && item->valuestring && strcmp(buf, item->valuestring))
    {
        strcpy(buf, item->valuestring);
        ESP_LOGI(TAG, "update %s: %s", key, buf);
        return 1;
    }
    return 0;
}

int sinesp_update_int_from_json(cJSON *root, const char *key, int *valp)
{
    cJSON *item = cJSON_GetObjectItem(root, key);
    if (item && *valp != item->valueint)
    {
        *valp = item->valueint;
        ESP_LOGI(TAG, "update %s: %d", key, *valp);
        return 1;
    }
    return 0;
}

esp_err_t sinesp_init()
{
    //初始化
    sinesp_event_group = xEventGroupCreate();
    return ESP_OK;
}

// 加载设置
esp_err_t sinesp_load_config(void)
{
    nvs_handle_t nvs_hd;
    esp_err_t err;
    memset(&sinespConfigBase, 0, sizeof(sinespConfigBase));

    // Open
    err = nvs_open(CONFIG_STORAGE_NAMESPACE, NVS_READONLY, &nvs_hd);
    if (err != ESP_OK)
        return err;
    // Read
    size_t size = sizeof(sinespConfigBase);
    err = nvs_get_blob(nvs_hd, STORE_CONFIG_KEY_BASE, &sinespConfigBase, &size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;

    if (sinespConfigBase.version != CONFIG_TYPE_VERSION)
    {
        // 初始化设置
        memset(&sinespConfigBase, 0, sizeof(sinespConfigBase));
        sinespConfigBase.version = CONFIG_TYPE_VERSION;
        ESP_LOGI(TAG, "Init config with default!");
    }
    else
    {
        ESP_LOGI(TAG, "Load config success!");
    }

    if (sinespConfigBase.size > 0)
    {
        // 存在设置项目
        char *jsonbuf = malloc(sinespConfigBase.size + 1);
        if (!jsonbuf)
        {
            ESP_LOGI(TAG, "malloc error");
            return ESP_FAIL;
        }

        err = nvs_get_blob(nvs_hd, CONFIG_STORAGE_KEY_JSON, jsonbuf, &sinespConfigBase.size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
            return err;
        jsonbuf[sinespConfigBase.size] = '\0';
        sinesp_update_config_with_json_string(jsonbuf);
        free(jsonbuf);
    }
    // if (!sinespConfigJson)
    // {
    //     sinespConfigJson = cJSON_CreateObject();
    // }

    // Close
    nvs_close(nvs_hd);

    return ESP_OK;
}

// 保存配置
esp_err_t sinesp_save_config(void)
{
    nvs_handle_t nvs_hd;
    esp_err_t err;

    // Open
    err = nvs_open(CONFIG_STORAGE_NAMESPACE, NVS_READWRITE, &nvs_hd);
    if (err != ESP_OK)
        return err;

    ++sinespConfigBase.updated;

    cJSON_AddStringToObject(sinespConfigJson, "ttt", "Hello");

    const char *jsonbuf = cJSON_PrintUnformatted(sinespConfigJson);
    if (jsonbuf)
    {
        sinespConfigBase.size = strlen(jsonbuf);
        ESP_LOGI(TAG, "Save JSON: %s", jsonbuf);
        err = nvs_set_blob(nvs_hd, CONFIG_STORAGE_KEY_JSON, jsonbuf, strlen(jsonbuf));
        if (err != ESP_OK)
            return err;
        free((void *)jsonbuf);
    }
    else
    {
        sinespConfigBase.size = 0;
    }

    err = nvs_set_blob(nvs_hd, STORE_CONFIG_KEY_BASE, &sinespConfigBase, sizeof(sinespConfigBase));
    if (err != ESP_OK)
        return err;

    err = nvs_commit(nvs_hd);
    if (err != ESP_OK)
        return err;

    // Close
    nvs_close(nvs_hd);
    ESP_LOGI(TAG, "Save config success!");
    return ESP_OK;
}

esp_err_t sinesp_update_config_with_json_string(const char *jsonbuf)
{
    ESP_LOGI(TAG, "sinesp_update_config_with_json_string: %s", jsonbuf);
    if (sinespConfigJson)
    {
        cJSON_Delete(sinespConfigJson);
        sinespConfigJson = NULL;
    }
    sinespConfigJson = cJSON_Parse(jsonbuf);
    return sinespConfigJson ? sinesp_save_config() : ESP_FAIL;
}
