#pragma once

#include "sinesp_base.h"

esp_err_t sinesp_wifi_init_softap(const char *ssid, const char *password);
esp_err_t sinesp_wifi_connect(const char *ssid, const char *password);
esp_err_t sinesp_wifi_init_with_config(int ap);