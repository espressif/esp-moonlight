/* APP Control

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
#include "lwip/sockets.h"
#include <lwip/netdb.h>
#include "cJSON.h"

#include "led_rgb.h"
#include "board_moonlight.h"
#include "blufi.h"
#include "iot_button.h"

static const char *TAG = "moonlight";
static led_rgb_t *g_leds = NULL;
static uint8_t g_red;
static uint8_t g_green;
static uint8_t g_blue;
static TaskHandle_t g_breath_light_task_handle = NULL;

#define PORT CONFIG_EXAMPLE_PORT


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

static void udp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family = (int)AF_INET;
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;

    while (1) {

        if (addr_family == AF_INET) {
            struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
            dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr_ip4->sin_family = AF_INET;
            dest_addr_ip4->sin_port = htons(PORT);
            ip_protocol = IPPROTO_IP;
        }

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);

        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        ESP_LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }

        ESP_LOGI(TAG, "Socket bound, port %d", PORT);

        while (1) {

            ESP_LOGI(TAG, "Waiting for data");
            struct sockaddr_in6 source_addr; /**< Large enough for both IPv4 or IPv6 */
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            /**< Error occurred during receiving */
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            /**< Data received */
            else {
                /**< Get the sender's ip address as string */
                if (source_addr.sin6_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                } else if (source_addr.sin6_family == PF_INET6) {
                    inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                rx_buffer[len] = 0; /**< Null-terminate whatever we received and treat like a string... */
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);

                cJSON *root = cJSON_Parse(rx_buffer);

                if (!root) {
                    printf("JSON format error:%s \r\n", cJSON_GetErrorPtr());
                } else {
                    cJSON *item = cJSON_GetObjectItem(root, "led");
                    int32_t red = cJSON_GetObjectItem(item, "red")->valueint;
                    int32_t green = cJSON_GetObjectItem(item, "green")->valueint;
                    int32_t blue = cJSON_GetObjectItem(item, "blue")->valueint;
                    cJSON_Delete(root);

                    if (red != g_red || green != g_green || blue != g_blue) {
                        g_red = red;
                        g_green = green;
                        g_blue = blue;
                        ESP_LOGI(TAG, "Light control: red = %d, green = %d, blue = %d", g_red, g_green, g_blue);
                        ESP_ERROR_CHECK(g_leds->set_rgb(g_leds, g_red, g_green, g_blue));
                    }
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }

    vTaskDelete(NULL);
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
    /**< Initialize NVS */
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    configure_push_button(BOARD_GPIO_BUTTON);

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

    /**< Initialize the BluFi */
    blufi_init();

    ESP_LOGI(TAG, "Wait for connect");
    blufi_wait_connect();
    ESP_LOGI(TAG, "Color fade start");
    vTaskDelete(g_breath_light_task_handle);

    /**< It's connected. It's time to change the light to white */
    ESP_ERROR_CHECK(g_leds->set_rgb(g_leds, 120, 120, 120));
    xTaskCreatePinnedToCore(udp_server_task, "udp_server_task", 1024 * 5, NULL, 5, NULL, 1);
}
