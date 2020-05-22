/* Speech Recognition Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "led_rgb.h"
#include "app_speech_if.h"
#include "board_moonlight.h"

static const char *TAG = "moonlight";

static led_rgb_t *g_leds = NULL;
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;

} led_color_t;

static led_color_t led_color = {0};
static led_color_t led_color_wake_bk = {0};

static TaskHandle_t g_breath_light_task_handle = NULL;


static void breath_light_task(void *arg)
{
    uint8_t value = 0;
    uint8_t dir = 0;

    while (1) {
        ESP_ERROR_CHECK(g_leds->set_hsv(g_leds, 120, 100, value));
        vTaskDelay(pdMS_TO_TICKS(20));

        if (dir) {
            value--;

            if (value < 20) {
                dir = 0;
            }
        } else {
            value++;

            if (value >= 60) {
                dir = 1;
            }
        }
    }
}

static void sr_wake(void *arg)
{
    g_leds->get_rgb(g_leds, &led_color_wake_bk.r, &led_color_wake_bk.g, &led_color_wake_bk.b);
    /**< Turn on the breathing light */
    xTaskCreate(breath_light_task, "breath_light_task", 1024 * 2, NULL, configMAX_PRIORITIES - 1, &g_breath_light_task_handle);

}

static void sr_cmd(void *arg)
{
    if (NULL != g_breath_light_task_handle) {
        vTaskDelete(g_breath_light_task_handle);
        g_leds->set_rgb(g_leds, led_color_wake_bk.r, led_color_wake_bk.g, led_color_wake_bk.b);
        g_breath_light_task_handle = NULL;
    }

    int32_t cmd_id = (int32_t)arg;

    switch (cmd_id) {
        case 0:
        case 1:
        case 2:
            g_leds->set_rgb(g_leds, led_color.r, led_color.g, led_color.b);
            break;

        case 3:
        case 4:
        case 5: {
            uint8_t r, g, b;
            g_leds->get_rgb(g_leds, &r, &g, &b);

            if (0 == r && 0 == g && 0 == b) {
                break;
            }

            g_leds->get_rgb(g_leds, &led_color.r, &led_color.g, &led_color.b);
            g_leds->set_rgb(g_leds, 0, 0, 0);
        }
        break;

        case 6: {
            uint32_t h, s, v;
            g_leds->get_hsv(g_leds, &h, &s, &v);
            /**< Set a random color */
            h = esp_random() / 11930465;
            s = esp_random() / 42949673;
            s = s < 40 ? 40 : s;
            ESP_ERROR_CHECK(g_leds->set_hsv(g_leds, h, s, v));
        }
        break;

        case 7:
        case 8: {
            uint32_t h, s, v;
            g_leds->get_hsv(g_leds, &h, &s, &v);

            if (v < 90) {
                v += 10;
            }

            g_leds->set_hsv(g_leds, h, s, v);
        }
        break;

        case 9:
        case 10: {
            uint32_t h, s, v;
            g_leds->get_hsv(g_leds, &h, &s, &v);

            if (v > 20) {
                v -= 10;
            }

            g_leds->set_hsv(g_leds, h, s, v);
        }
        break;

        default:
            break;
    }
}

static void sr_cmd_exit(void *arg)
{
    if (NULL != g_breath_light_task_handle) {
        vTaskDelete(g_breath_light_task_handle);
        g_leds->set_rgb(g_leds, led_color_wake_bk.r, led_color_wake_bk.g, led_color_wake_bk.b);
        g_breath_light_task_handle = NULL;
    }
}


void app_main(void)
{
    /**< Initialize NVS */
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    /**< configure led driver */
    led_rgb_config_t rgb_config = {0};
    rgb_config.red_gpio_num   = BOARD_GPIO_LED_R;
    rgb_config.green_gpio_num = BOARD_GPIO_LED_G;
    rgb_config.blue_gpio_num  = BOARD_GPIO_LED_B;
    rgb_config.red_ledc_ch    = LEDC_CHANNEL_0;
    rgb_config.green_ledc_ch  = LEDC_CHANNEL_1;
    rgb_config.blue_ledc_ch   = LEDC_CHANNEL_2;
    rgb_config.speed_mode = LEDC_LOW_SPEED_MODE;
    rgb_config.timer_sel  = LEDC_TIMER_0;
    rgb_config.freq       = 20000;
    rgb_config.resolution = LEDC_TIMER_8_BIT;
    g_leds = led_rgb_create(&rgb_config);

    if (!g_leds) {
        ESP_LOGE(TAG, "install LED driver failed");
        return;
    }

    speech_recognition_init();

    g_leds->set_rgb(g_leds, 120, 120, 120);
    g_leds->get_rgb(g_leds, &led_color.r, &led_color.g, &led_color.b);
    sr_handler_install(SR_CB_TYPE_WAKE, sr_wake, NULL);
    sr_handler_install(SR_CB_TYPE_CMD, sr_cmd, NULL);
    sr_handler_install(SR_CB_TYPE_CMD_EXIT, sr_cmd_exit, NULL);
}
