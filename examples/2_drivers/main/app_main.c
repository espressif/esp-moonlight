/* Driver Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "led_rgb.h"
#include "board_moonlight.h"
#include "iot_button.h"
#include "sensor.h"

static const char *TAG = "moonlight";
static led_rgb_t *g_leds = NULL;
static uint8_t g_led_mode = 0; /**< led mode  0:color fade 1:fixed color */


static void vibration_handle(void *arg)
{
    uint16_t h;
    uint8_t s;

    if (!g_led_mode) {
        return;
    }

    /**< Set a random color */
    h = esp_random() / 11930465;
    s = esp_random() / 42949673;
    s = s < 40 ? 40 : s;

    ESP_ERROR_CHECK(g_leds->set_hsv(g_leds, h, s, 100));
}

static void button_press_cb(void *arg)
{
    if (g_led_mode) {
        g_led_mode = 0;
    } else {
        g_led_mode = 1;
    }

    ESP_LOGI(TAG, "Set the light mode to %d", g_led_mode);
}

static void configure_push_button(int gpio_num)
{
    button_handle_t btn_handle = iot_button_create(gpio_num, 0);

    if (btn_handle) {
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_TAP, button_press_cb, NULL);
    }
}

void app_main(void)
{
    uint32_t hue = 0;

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
    }

    /**< Configure button and sensors */
    configure_push_button(BOARD_GPIO_BUTTON);
    sensor_battery_init(BOARD_BAT_ADC_CHANNEL, BOARD_GPIO_BAT_CHRG, BOARD_GPIO_BAT_STBY);
    sensor_vibration_init(BOARD_GPIO_SENSOR_INT);
    sensor_vibration_triggered_register(vibration_handle, NULL);

    ESP_LOGI(TAG, "Color fade start");

    while (true) {
        if (!g_led_mode) {
            /**< Write HSV values to LED */
            ESP_ERROR_CHECK(g_leds->set_hsv(g_leds, hue, 100, 100));
            vTaskDelay(pdMS_TO_TICKS(30));
            hue++;

            if (hue > 360) {/**< The maximum value of hue in HSV color space is 360 */
                hue = 0;
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
