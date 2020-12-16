/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sinesp_http.h"
#include "sinesp_gpio.h"
#include "sinesp_oled.h"

static const char *TAG = "http";
static int reqcount = 1;

esp_err_t sinesp_http_read_all_req_data(httpd_req_t *req, char *buf, size_t max)
{
    int total_len = req->content_len;
    int cur_len = 0;
    int received = 0;
    if (max > 0 && total_len >= max)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Body too long!!!");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';
    return ESP_OK;
}

esp_err_t sinesp_http_response_string(httpd_req_t *req, const char *type, const char *data)
{
    ++reqcount;
    httpd_resp_set_type(req, type);
    httpd_resp_sendstr(req, data);
    return ESP_OK;
}

esp_err_t sinesp_http_api_response(httpd_req_t *req, int code, cJSON *data, const char *msg)
{
    char *sbuf = NULL;
    if (data)
    {
        const char *jsonbuf = cJSON_PrintUnformatted(data);
        sbuf = malloc(strlen(jsonbuf) + 64);
        sprintf(sbuf, "{\"code\":%d, \"data\":%s}", code, jsonbuf);
        free((void *)jsonbuf);
    }
    else
    {
        if (!msg)
        {
            msg = "ERROR";
        }
        sbuf = malloc(strlen(msg) + 64);
        sprintf(sbuf, "{\"code\":%d, \"msg\":\"%s\"}", code, msg);
    }
    if (sbuf)
    {
        sinesp_http_response_string(req, "application/json", sbuf);
        free(sbuf);
    }
    return ESP_OK;
}

esp_err_t http_html_response(httpd_req_t *req, const char *html)
{
    return sinesp_http_response_string(req, "text/html", html);
}

#define sinesp_http_api_response_data(req, data) sinesp_http_api_response(req, 0, data, NULL)
#define sinesp_http_api_response_error(req, msg) sinesp_http_api_response(req, -1, NULL, msg)
#define sinesp_http_api_response_error_code(req, msg, code) sinesp_http_api_response(req, code, NULL, msg)

/* Send HTTP response with the contents of the requested file */
static esp_err_t handle_get_index(httpd_req_t *req)
{
    http_html_response(req, "Hello~");
    return ESP_OK;
}

static esp_err_t getStatus(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "idf_ver", IDF_VER);
    cJSON_AddNumberToObject(root, "memory", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    cJSON_AddNumberToObject(root, "version", sinespConfigBase.version);
    cJSON_AddStringToObject(root, "app_version", sinespConfigBase.app_version);
    cJSON_AddNumberToObject(root, "updated", sinespConfigBase.updated);
    cJSON_AddNumberToObject(root, "request", reqcount);

    sinesp_http_api_response_data(req, root);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t getConfig(httpd_req_t *req)
{
    if (sinespConfigJson)
    {
        sinesp_http_api_response_data(req, sinespConfigJson);
    }
    else
    {
        sinesp_http_api_response_error(req, "Failed to get config");
    }
    return ESP_OK;
}

static esp_err_t setConfig(httpd_req_t *req)
{
    char *buf = malloc(MAX_HTTP_BODY_SIZE);
    if (sinesp_http_read_all_req_data(req, buf, MAX_HTTP_BODY_SIZE) == ESP_OK)
    {
        ESP_LOGI(TAG, "POST: %s", buf);
        if (sinesp_update_config_with_json_string(buf) == ESP_OK)
        {
            set_led_with_config();
            free(buf);
            return getConfig(req);
        }
    }
    sinesp_http_api_response_error(req, "Failed to save config");
    free(buf);
    return ESP_OK;
}

static inline uint8_t hex_to_u8(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    return 0;
}

static esp_err_t sendOledData(httpd_req_t *req)
{
    char *buf = malloc(MAX_HTTP_BODY_SIZE);
    if (sinesp_http_read_all_req_data(req, buf, MAX_HTTP_BODY_SIZE) == ESP_OK)
    {
        ESP_LOGI(TAG, "POST: %s", buf);
        uint8_t *data = malloc(MAX_HTTP_BODY_SIZE);
        uint8_t d;
        int i = 0;
        while (buf[i] && buf[i + 1])
        {
            d = (hex_to_u8(buf[i]) << 4) | hex_to_u8(buf[i + 1]);
            data[i / 2] = d;
            i += 2;
            // OLED_Set_All(d);
        }
        ESP_LOGI(TAG, "DATA LEN=%d [0]=%02x", i / 2, data[0]);
        // uint8_t data[] = {0xFF, 0xF0, 0x0F, 0xAA, 0x44};
        // for(int n=0; n< 8; ++n){
        //     sinesp_oled_send_data(0, n, data, i / 2);
        // }
        // sinesp_oled_send_data(0, 0, data, i / 2);
    }
    // sinesp_http_api_response_error(req, "Failed to save config");
    free(buf);
    return ESP_OK;
}

#define HTTP_URI_MAPPING(_uri, _handle, _method)   \
    ESP_LOGI(TAG, "Uri %s -> %s", _uri, #_handle); \
    httpd_uri_t _handle##uri = {                   \
        .uri = _uri,                               \
        .method = _method,                         \
        .handler = _handle,                        \
    };                                             \
    httpd_register_uri_handler(server, &_handle##uri);

#define HTTP_GET_MAPPING(_uri, _handle) HTTP_URI_MAPPING(_uri, _handle, HTTP_GET)
#define HTTP_POST_MAPPING(_uri, _handle) HTTP_URI_MAPPING(_uri, _handle, HTTP_POST)
#define HTTP_API(_mth) HTTP_POST_MAPPING("/api/" #_mth, _mth)

esp_err_t httpd_open(httpd_handle_t hd, int sockfd)
{
    sinesp_led_status_toggle();
    return ESP_OK;
}

esp_err_t sinesp_start_http_server()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.open_fn = httpd_open;
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server");
    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Start HTTP Server Failed!!!!");
        goto err_start;
    }

    // 系统状态
    HTTP_API(getStatus);

    // 系统设置
    HTTP_API(getConfig);
    HTTP_API(setConfig);
    HTTP_API(sendOledData);

    // 首页
    HTTP_GET_MAPPING("/", handle_get_index);
    set_led_with_config();
    return ESP_OK;
err_start:
    // free(rest_context);
    //     server = NULL;
    // err:
    return ESP_FAIL;
}
