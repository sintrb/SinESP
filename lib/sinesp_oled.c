#include "sinesp_oled.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

static const char *TAG = "oled";

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 19
#define PIN_NUM_CS 22

#define PIN_NUM_DC 21
#define PIN_NUM_RST 18
#define PIN_NUM_BCKL 5

//#define PARALLEL_LINES 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 8
#define SCREEN_BYTES (SCREEN_WIDTH * SCREEN_HEIGHT)
#define MAX_IMAGE 64

#define OLED_STORE_SPACE "oled"
#define OLED_STORE_IMAGE "images"

// #define OLED_WriteCmd(cmd) lcd_cmd(spi, cmd)
// #define OLED_WriteData(dat) lcd_data(spi, dat)

spi_device_handle_t spi;
typedef uint8_t u8;

static uint8_t image_count = 0;
static int image_index = 0;
//static uint8_t image_buffer[SCREEN_BYTES * MAX_IMAGE] = {0};
static uint8_t *image_buffer;

void OLED_WriteCmd(const uint8_t cmd) {
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));                    //Zero out the transaction
    t.length = 8;                                //Command is 8 bits
    t.tx_buffer = &cmd;                            //The data is the cmd itself
    t.user = (void *) 0;                            //D/C needs to be set to 0
    ret = spi_device_polling_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);                        //Should have had no issues.
}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 *
 * Since data transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
void OLED_WriteDatas(const uint8_t *data, int len) {
    esp_err_t ret;
    spi_transaction_t t;
    if (len == 0)
        return;                                    //no need to send anything
    memset(&t, 0, sizeof(t));                    //Zero out the transaction
    t.length = len * 8;                            //Len is in bytes, transaction length is in bits.
    t.tx_buffer = data;                            //Data
    t.user = (void *) 1;                            //D/C needs to be set to 1
    ret = spi_device_polling_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);                        //Should have had no issues.
}

void OLED_WriteData(uint8_t data) {
    uint8_t dats[1];
    dats[0] = data;
    OLED_WriteDatas(dats, 1);
}

void lcd_spi_pre_transfer_callback(spi_transaction_t *t) {
    int dc = (int) t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}

void OLED_Clear(void) {
    u8 i, n;
    for (i = 0; i < SCREEN_HEIGHT; i++) {
        OLED_WriteCmd(0XB0 + i); //设置页地址 0XB0~0XB7
        OLED_WriteCmd(0X00);     //设置显示位置—列低地址
        OLED_WriteCmd(0X10);     //设置显示位置—列高地址
        for (n = 0; n < SCREEN_WIDTH; n++) {
            OLED_WriteData(0);
        }
    }
}

void OLED_Set_Pos(unsigned char x, unsigned char y) {
    OLED_WriteCmd(0xb0 + y);
    OLED_WriteCmd(((x & 0xf0) >> 4) | 0x10);
    OLED_WriteCmd((x & 0x0f) | 0x00);
}

void OLED_Init() {
    image_buffer = malloc(SCREEN_BYTES * MAX_IMAGE);
    // int cmd=0;
    // const lcd_init_cmd_t* lcd_init_cmds;

    //Initialize non-SPI GPIOs
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_OUTPUT);

    //Reset the display
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(100 / portTICK_RATE_MS);

    OLED_WriteCmd(0XAE); //关闭OLED面板
    OLED_WriteCmd(0X00); //设置页地址模式的列起始地址低位
    OLED_WriteCmd(0X10); //设置页地址模式的列起始地址高位
    OLED_WriteCmd(0X40); //设置屏幕起始行

    OLED_WriteCmd(0X81); //设置对比度(相当于屏幕亮度)
    OLED_WriteCmd(0X00); //对比度设置值，范围0X00-0XFF

    /*好像是设置显示方向的*/
    OLED_WriteCmd(0XA1); //设置行扫描方向为从左到右
    OLED_WriteCmd(0XC8); //设置列扫描方向为数据低位在前

    OLED_WriteCmd(0XA8); //设置复用率

    OLED_WriteCmd(0X3F);

    /*显示偏移*/
    OLED_WriteCmd(0XD3); //设置显示偏移
    OLED_WriteCmd(0x00); //不偏移

    /*设置时钟*/
    OLED_WriteCmd(0XD5); //设置显示时钟分频值/震荡频率
    OLED_WriteCmd(0x80); //设置分频比，将时钟设置为100帧/秒

    /*充电*/
    OLED_WriteCmd(0XD9); //设置预充电周期
    OLED_WriteCmd(0xF1); //1个充电时钟和15个放电时钟

    OLED_WriteCmd(0XDA); //设置列引脚硬件配置
    OLED_WriteCmd(0x12); //列输出扫描方向从COM63到COM0(C8h), 启用列左/右映射(DAh A[5]=1)

    OLED_WriteCmd(0XDB); //设置VCOMH反压值
    OLED_WriteCmd(0X40); //设置VCOM取消选择级别

    OLED_WriteCmd(0X20); //设置内存寻址模式
    OLED_WriteCmd(0x02); //页地址寻址模式

    OLED_WriteCmd(0x8D); //电荷泵启用/禁用
    OLED_WriteCmd(0x14); //关闭

    OLED_WriteCmd(0xA4); //点亮屏幕
    OLED_WriteCmd(0xA6); //设置正常显示，不反转，1表示点亮像素
    OLED_WriteCmd(0xAF); //打开OLED面板

    OLED_Clear();        //清屏
    OLED_Set_Pos(0, 0); //设置起始坐标
}

void OLED_Set_All(uint8_t d) {
    u8 dats[SCREEN_WIDTH];
    int i;
    for (i = 0; i < SCREEN_WIDTH; ++i) {
        dats[i] = d;
    }
    for (int i = 0; i < SCREEN_HEIGHT; ++i) {
        OLED_Set_Pos(0, i);
        OLED_WriteDatas(dats, SCREEN_WIDTH);
    }
}

esp_err_t sinesp_oled_send_data(int x, int y, const uint8_t *data, size_t len) {
    OLED_Set_Pos(x, y);
    OLED_WriteDatas(data, len);
    return ESP_OK;
}

esp_err_t sinesp_oled_send_image(const uint8_t *data) {
    for (int i = 0; i < SCREEN_HEIGHT; ++i) {
        sinesp_oled_send_data(0, i, data + (i * SCREEN_WIDTH), SCREEN_WIDTH);
    }
    return ESP_OK;
}

esp_err_t sinesp_oled_load_image() {
    // 加载图片
    nvs_handle_t nvs_hd;
    esp_err_t err;
    err = nvs_open(OLED_STORE_SPACE, NVS_READONLY, &nvs_hd);
    if (err == ESP_OK) {
        size_t size = SCREEN_BYTES;
        err = nvs_get_blob(nvs_hd, OLED_STORE_IMAGE, image_buffer, &size);
        if (err == ESP_OK) {
            image_count = 1;
            image_index = -1;
            ESP_LOGI(TAG, "Load Image: %dbytes %dframes!", size, image_count);
        } else {
            ESP_LOGE(TAG, "nvs_get_blob failed: 0x%04x", err);
        }
        nvs_close(nvs_hd);
    } else {
        ESP_LOGE(TAG, "nvs_open failed: 0x%04x", err);
    }
    return err;
}

esp_err_t sinesp_oled_save_image() {
    // 加载图片
    nvs_handle_t nvs_hd;
    esp_err_t err;
    err = nvs_open(OLED_STORE_SPACE, NVS_READWRITE, &nvs_hd);
    if (err == ESP_OK) {
        err = nvs_set_blob(nvs_hd, OLED_STORE_IMAGE, image_buffer, image_count * SCREEN_BYTES);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Save Image: %d!", image_count);
        } else {
            ESP_LOGE(TAG, "nvs_set_blob failed: 0x%04x", err);
        }
        nvs_close(nvs_hd);
    }
    return err;
}

esp_err_t sinesp_oled_set_image(uint8_t index, const uint8_t *data) {
    if (index < MAX_IMAGE) {
        ESP_LOGI(TAG, "set image %d", index);
        memcpy(image_buffer + (SCREEN_BYTES * index), data, SCREEN_BYTES);
        image_count = index + 1;
        if (image_count == 1) {
            image_index = -1;
            sinesp_oled_save_image();
        }
        return ESP_OK;
    } else {
        ESP_LOGI(TAG, "image overflow %d", index);
        return ESP_FAIL;
    }
}

void sinesp_oled_ticks() {
    int index = image_index;
    ++image_index;
    if (image_index >= image_count) {
        image_index = 0;
    }
    if (image_index < 0) {
        image_index = 0;
    }
    if (image_index < image_count && index != image_index) {
        sinesp_oled_send_image(image_buffer + (SCREEN_BYTES * image_index));
    }
}

static void oled_thread(void *parm) {
    for (;;) {
        sinesp_oled_ticks();
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

esp_err_t sinesp_oled_init() {
    spi_bus_config_t buscfg = {
            .miso_io_num = PIN_NUM_MISO,
            .mosi_io_num = PIN_NUM_MOSI,
            .sclk_io_num = PIN_NUM_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = SCREEN_BYTES + 8};
    spi_device_interface_config_t devcfg = {
#ifdef CONFIG_LCD_OVERCLOCK
            .clock_speed_hz = 26 * 1000 * 1000, //Clock out at 26 MHz
#else
            .clock_speed_hz = 10 * 1000 * 1000, //Clock out at 10 MHz
#endif
            .mode = 0,                                 //SPI mode 0
            .spics_io_num = PIN_NUM_CS,                 //CS pin
            .queue_size = 7,                         //We want to be able to queue 7 transactions at a time
            .pre_cb = lcd_spi_pre_transfer_callback, //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, 1));
    //Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &spi));
    //Initialize the LCD

    OLED_Init();
    if (sinesp_oled_load_image() != ESP_OK)
        OLED_Set_All(0x00);
    xTaskCreate(oled_thread, "oled_thread", 2048, NULL, 10, NULL);
    ESP_LOGI(TAG, "Inited!!!");
    return ESP_OK;
}