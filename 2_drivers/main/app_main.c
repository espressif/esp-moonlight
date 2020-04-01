/* RMT example -- RGB LED Strip

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"


static const char *TAG = "moonlight";

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define CONFIG_EXAMPLE_STRIP_LED_NUMBER 48

static void led_set_color(led_strip_t *strip, uint8_t red, uint8_t green, uint8_t blue)
{
    for (size_t i = 0; i < CONFIG_EXAMPLE_STRIP_LED_NUMBER; i++)
    {
        // Write RGB values to strip driver
        ESP_ERROR_CHECK(strip->set_pixel(strip, i, red, green, blue));
    }
    
}


void app_main(void)
{
    uint8_t red;
    uint8_t blue;
    uint8_t green;

    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(18, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(CONFIG_EXAMPLE_STRIP_LED_NUMBER, (led_strip_dev_t)config.channel);
    led_strip_t *strip = led_strip_new_rmt_ws2812(&strip_config);

    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }

    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(strip->clear(strip, 100));

    while (true) {

        // Write RGB values to strip driver
        led_set_color(strip, );

        // Flush RGB values to LEDs
        ESP_ERROR_CHECK(strip->refresh(strip, 100));
        vTaskDelay(pdMS_TO_TICKS(500));
        start_rgb+=30;

    }
}
