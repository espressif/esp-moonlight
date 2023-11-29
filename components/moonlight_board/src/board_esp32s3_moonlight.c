/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "board_moonlight.h"

static const char *TAG = "esp32s3_moonlight";

led_rgb_t *board_rgb_init(void)
{
    /**< configure led driver */
    led_rgb_config_t rgb_config = LED_RGB_DEFAULT_CONFIG(BOARD_GPIO_LED_R, BOARD_GPIO_LED_G, BOARD_GPIO_LED_B);
    led_rgb_t *led_rgb = led_rgb_create(&rgb_config);

    if (!led_rgb) {
        ESP_LOGE(TAG, "install LED driver failed");
    }
    return led_rgb;
}

void board_led_rgb_ctrl(led_rgb_t *led_rgb, uint8_t led_mode)
{
    static uint32_t hue = 0;
    if (!led_mode) {
        /**< Write HSV values to LED */
        esp_err_t ret = led_rgb->set_hsv(led_rgb, hue, 100, 100);
        ESP_RETURN_ON_FALSE(ret == ESP_OK, , TAG, "set rgb hsv failed");
        vTaskDelay(pdMS_TO_TICKS(30));
        hue++;

        if (hue > 360) {/**< The maximum value of hue in HSV color space is 360 */
            hue = 0;
        }
    } else {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void breath_light_task(void *arg)
{
    led_rgb_t *led_rgb = (led_rgb_t*)arg;
    uint8_t value = 0;
    uint8_t dir = 0;

    while (1) {
        esp_err_t ret = led_rgb->set_hsv(led_rgb, 60, 100, value);
        ESP_RETURN_ON_FALSE(ret == ESP_OK, , TAG, "set rgb hsv failed");
        vTaskDelay(pdMS_TO_TICKS(20));

        if (dir) {
            value--;

            if (value < 2) {
                dir = 0;
            }
        } else {
            value++;

            if (value >= 100) {
                dir = 1;
            }
        }
    }
}

button_handle_t board_button_init(void)
{
    ESP_LOGI(TAG, "esp32s3");
    button_config_t cfg = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = 0,
            .active_level = 0,
        },
    };
    button_handle_t button = iot_button_create(&cfg);
    ESP_RETURN_ON_FALSE(button != NULL, NULL, TAG, "Button initialization failed.");
    return button;
}

esp_err_t board_sensor_init(vibration_isr_t fn)
{
    esp_err_t ret = sensor_battery_init(BOARD_BAT_ADC_CHANNEL, BOARD_GPIO_BAT_CHRG, BOARD_GPIO_BAT_STBY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initialize adc for measure battery voltage failed.");
    }
    ret = sensor_vibration_init(BOARD_GPIO_SENSOR_INT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initialize vabration sensors failed.");
    }
    ret = sensor_vibration_triggered_register(fn, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Register vabration sensor triggered handler failed.");
    }
    return ret;
}

esp_err_t board_nvs_flash_init(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ret = nvs_flash_erase();
        ESP_RETURN_ON_FALSE(ret == ESP_OK, ESP_FAIL, TAG, "nvs flash erase failed");
        ret = nvs_flash_init();
    }
    return ret;
}
