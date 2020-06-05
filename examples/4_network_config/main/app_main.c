/* Network Config Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "string.h"
#include "iot_button.h"
#include "led_rgb.h"
#include "board_moonlight.h"
#include "blufi.h"

static const char *TAG = "moonlight";
static TaskHandle_t g_breath_light_task_handle = NULL;
static led_rgb_t *g_leds = NULL;

static void button_press_3sec_cb(void *arg)
{
    ESP_LOGW(TAG, "Restore factory settings");
    nvs_flash_erase();
    esp_restart();
}

static void configure_push_button(int gpio_num)
{
    button_handle_t btn_handle = iot_button_create(gpio_num, 0);

    if (btn_handle) {
        iot_button_add_on_press_cb(btn_handle, 3, button_press_3sec_cb, NULL);
    }
}

static void breath_light_task(void *arg)
{
    uint8_t value = 0;
    uint8_t dir = 0;

    while (1) {
        ESP_ERROR_CHECK(g_leds->set_hsv(g_leds, 60, 100, value));
        vTaskDelay(pdMS_TO_TICKS(20));

        if (dir) {
            value--;

            if (value < 8) {
                dir = 0;
            }
        } else {
            value++;

            if (value >= 80) {
                dir = 1;
            }
        }
    }
}


void app_main(void)
{
    uint16_t hue = 0;
    /**< Initialize NVS */
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    configure_push_button(BOARD_GPIO_BUTTON);

    /**< configure led driver */
    led_rgb_config_t rgb_config = LED_RGB_DEFAULT_CONFIG(BOARD_GPIO_LED_R, BOARD_GPIO_LED_G, BOARD_GPIO_LED_B);
    g_leds = led_rgb_create(&rgb_config);

    if (!g_leds) {
        ESP_LOGE(TAG, "install LED driver failed");
    }

    /**< Turn on the breathing light */
    xTaskCreatePinnedToCore(breath_light_task, "breath_light_task", 1024 * 3, NULL, 5, &g_breath_light_task_handle, 1);

    /**< Initialize the BluFi */
    blufi_network_init();
    bool configured;
    blufi_is_configured(&configured);

    if (!configured) {
        blufi_start();
    }

    ESP_LOGI(TAG, "Wait for connect");
    blufi_wait_connection(portMAX_DELAY);

    if (!configured) {
        blufi_stop();
    }

    vTaskDelete(g_breath_light_task_handle);
    ESP_LOGI(TAG, "Color fade start");

    while (true) {

        /**< Write HSV values to LED */
        ESP_ERROR_CHECK(g_leds->set_hsv(g_leds, hue, 100, 80));
        vTaskDelay(pdMS_TO_TICKS(30));
        hue++;

        if (hue > 360) {/**< The maximum value of hue in HSV color space is 360 */
            hue = 0;
        }
    }
}
