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
#include "esp_check.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "string.h"
#include "iot_button.h"
#include "led_rgb.h"
#include "board_moonlight.h"
#include "blufi.h"

static const char *TAG = "moonlight";
static TaskHandle_t g_breath_light_task_handle = NULL;
static led_rgb_t *g_leds = NULL;

static void button_press_3sec_cb(void *arg, void *data)
{
    ESP_LOGW(TAG, "Restore factory settings");
    nvs_flash_erase();
    esp_restart();
}

static esp_err_t button_init(void)
{
    button_handle_t button = board_button_init();
    esp_err_t ret = iot_button_register_cb(button, BUTTON_LONG_PRESS_START, button_press_3sec_cb, NULL);
    return ret;
}

void app_main(void)
{
    uint16_t hue = 0;
    /**< Initialize NVS */
    ESP_ERROR_CHECK(board_nvs_flash_init());
    /**< Initialize button */
    ESP_ERROR_CHECK(button_init());
    /**< configure led driver */
    g_leds = board_rgb_init();

    /**< Turn on the breathing light */
    xTaskCreatePinnedToCore(breath_light_task, "breath_light_task", 1024 * 3, (void*)g_leds, 5, &g_breath_light_task_handle, 1);

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
