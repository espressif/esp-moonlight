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
#include "esp_check.h"
#include "board_moonlight.h"

static const char *TAG = "moonlight";
static led_rgb_t *g_leds = NULL;
uint8_t g_led_mode = 0; /**< led mode  0:color fade 1:fixed color */

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

static void button_press_down_cb(void *arg, void *data)
{
    if (g_led_mode) {
        g_led_mode = 0;
    } else {
        g_led_mode = 1;
    }

    ESP_LOGI(TAG, "Set the light mode to %d", g_led_mode);
}

static esp_err_t button_init(void)
{
    button_handle_t button = board_button_init();
    esp_err_t ret = iot_button_register_cb(button, BUTTON_PRESS_DOWN, button_press_down_cb, NULL);
    return ret;
}

void app_main(void)
{
    /**< configure led driver */
    g_leds = board_rgb_init();

    /**< Configure button and sensors */
    ESP_ERROR_CHECK(button_init());
    ESP_ERROR_CHECK(board_sensor_init(vibration_handle));

    ESP_LOGI(TAG, "Color fade start");

    while (true) {
        board_led_rgb_ctrl(g_leds, g_led_mode);
    }
}
