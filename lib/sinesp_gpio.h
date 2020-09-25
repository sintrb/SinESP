#pragma once

#include "sinesp_base.h"

#define GPIO_LED 2
#define GPIO_KEY 0


esp_err_t sinesp_init_gpio(void);
void sinesp_led_status_toggle();
void sinesp_led_set_time(int onms, int offms);
esp_err_t set_led_with_config(void);
int sinesp_is_key_down();