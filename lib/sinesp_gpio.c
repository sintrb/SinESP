#include "sinesp_gpio.h"

static const char *TAG = "gpid";

#define GPIO_LED_SEL (1ULL << GPIO_LED)
#define GPIO_KEY_SEL (1ULL << GPIO_KEY)

#define LED_SLEEPTIME_ON -1
#define LED_SLEEPTIME_OFF 0
#define LED_SLEEPTIME_DEFAULT 1000

static int led_state = 0;
static int led_on_time = 1000;
static int led_off_time = 1000;

static void set_led(int state)
{
    if ((state ? 1 : 0) != (led_state ? 1 : 0))
    {
        gpio_set_level(GPIO_LED, state ? 1 : 0);
        led_state = state;
    }
}
static void gpio_thread(void *parm)
{
    int sleep = led_on_time;
    int nstate = 0;
    for (;;)
    {
        nstate = !led_state;
        if (nstate)
        {
            // on
            sleep = led_on_time;

        }
        else
        {
            sleep = led_off_time;
        }
        if(sleep){
            set_led(nstate);
        }
        
        // ESP_LOGI(TAG, "On:%d Off:%d Sleep:%d %s", led_on_time, led_off_time, sleep, led_state ? "ON" : "OFF");
        if (SinESPWaitEvent(SINESP_EVENT_BIT_LED_UPDATED, sleep > portTICK_RATE_MS ? (sleep / portTICK_RATE_MS) : portTICK_RATE_MS) & SINESP_EVENT_BIT_LED_UPDATED)
            SinESPClearEvent(SINESP_EVENT_BIT_LED_UPDATED);
    }
}

void sinesp_led_set_time(int onms, int offms)
{
    led_on_time = onms;
    led_off_time = offms;
    SinESPSendEvent(SINESP_EVENT_BIT_LED_UPDATED);
    // char payload[MQTT_PAYLOAD_BUFFER_SIZE];
    // sprintf(payload, "{\"data\": {\"devices\": [{\"dev_id\": \"%s\", \"attr\": {\"counter\": %d, \"freq\": %d}}], \"dtu_id\": \"dtu2\"}, \"type\": \"update\"}", "dev5", get_timestamp(), led_sleep);
    // mqtt_publish("xt15429507412810938/espmqtt", 0, payload, strlen(payload));
}

int sinesp_is_key_down()
{
    return gpio_get_level(GPIO_KEY) == 0? 1: 0;
}

void sinesp_led_status_toggle()
{
    // set_led(!led_state);
    // TOGGLE_VALUE(led_sleep, LED_SLEEPTIME_OFF, LED_SLEEPTIME_ON);
    SinESPSendEvent(SINESP_EVENT_BIT_LED_UPDATED);
}

static void gpio_isr_handler(void *arg)
{
    sinesp_led_status_toggle();
}

esp_err_t set_led_with_config(void)
{
    if (sinespConfigJson)
    {
        cJSON *led = cJSON_GetObjectItem(sinespConfigJson, "led");
        if (led)
        {
            int on = 0, off = 0;
            int up = 0;
            up += sinesp_update_int_from_json(led, "on", &on);
            up += sinesp_update_int_from_json(led, "off", &off);
            if (up)
            {
                sinesp_led_set_time(on, off);
                return ESP_OK;
            }
        }
    }
    return ESP_FAIL;
}

esp_err_t sinesp_init_gpio(void)
{
    // LED
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_LED_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_LED, 1);
    ESP_LOGI(TAG, "Time:%d", portTICK_RATE_MS);

    // KEY
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_KEY_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_set_intr_type(GPIO_KEY, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(0);
    // gpio_isr_handler_add(GPIO_KEY, gpio_isr_handler, (void *)GPIO_KEY);
    // gpio_isr_handler_remove(GPIO_KEY);
    gpio_isr_handler_add(GPIO_KEY, gpio_isr_handler, (void *)GPIO_KEY);

    xTaskCreate(gpio_thread, "gpio_thread", 2048, NULL, 10, NULL);

    return ESP_OK;
}