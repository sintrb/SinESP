#pragma once

#include "sinesp_base.h"

esp_err_t sinesp_oled_init();
esp_err_t sinesp_oled_send_data(int x, int y, const uint8_t *data, size_t len);
void OLED_Set_All(uint8_t d);