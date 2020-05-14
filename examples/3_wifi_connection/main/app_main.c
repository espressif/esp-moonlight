/* wifi connection

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "led_rgb.h"
#include "board_moonlight.h"

static const char *TAG = "moonlight";

#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY


/**< FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static TaskHandle_t g_breath_light_task_handle = NULL;
static led_rgb_t *g_leds = NULL;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    static int32_t s_retry_num = 0;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }

        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init_sta(void)
{
    esp_err_t ret = ESP_OK;
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ret = ESP_FAIL;
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
    return ret;
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


void app_main(void)
{
    uint32_t hue = 0;
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
    }

    /**< Turn on the breathing light */
    xTaskCreatePinnedToCore(breath_light_task, "breath_light_task", 1024 * 3, NULL, 5, &g_breath_light_task_handle, 1);

    /**< Start the station */
    ESP_LOGI(TAG, "Wait for connect");
    ret = wifi_init_sta();
    vTaskDelete(g_breath_light_task_handle);

    if (ESP_OK != ret) {
        /**< Set leds to red to indicate failure */
        ESP_ERROR_CHECK(g_leds->set_rgb(g_leds, 60, 0, 0));
        ESP_LOGW(TAG, "Connect failed");
        return;
    }

    ESP_LOGI(TAG, "Color fade start");

    while (true) {

        /**< Write HSV values to LED */
        ESP_ERROR_CHECK(g_leds->set_hsv(g_leds, hue, 100, 100));
        vTaskDelay(pdMS_TO_TICKS(30));
        hue++;

        if (hue > 360) {/**< The maximum value of hue in HSV color space is 360 */
            hue = 0;
        }
    }
}
