/* OTA Example

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
#include "esp_check.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include <lwip/netdb.h>
#include "cJSON.h"

#include "led_rgb.h"
#include "board_moonlight.h"
#include "blufi.h"
#include "ota.h"
#include "iot_button.h"

static const char *TAG = "moonlight";
static led_rgb_t *g_leds = NULL;
static uint8_t g_red;
static uint8_t g_green;
static uint8_t g_blue;
static TaskHandle_t g_breath_light_task_handle = NULL;
static SemaphoreHandle_t ota_Semaphore = NULL;

#define PORT CONFIG_EXAMPLE_PORT


static void button_press_3sec_cb(void *arg, void *data)
{
    ESP_LOGW(TAG, "Restore factory settings");
    nvs_flash_erase();
    esp_restart();
}

static void button_press_cb(void *arg, void *data)
{
    xSemaphoreGive(ota_Semaphore);
}

static esp_err_t button_init(void)
{
    button_handle_t button = board_button_init();
    esp_err_t ret = iot_button_register_cb(button, BUTTON_LONG_PRESS_START, button_press_3sec_cb, NULL);
    ret = iot_button_register_cb(button, BUTTON_PRESS_DOWN, button_press_cb, NULL);
    return ret;
}

static void ota_task(void *pvParameters)
{
    while (1) {
        xSemaphoreTake(ota_Semaphore, portMAX_DELAY);
        ESP_LOGW(TAG, "Start firmware upgrade");
        ota_start(CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL);
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

void app_main(void)
{
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

    /**< It's connected. It's time to change the light to white */
    g_red = 120;
    g_green = 120;
    g_blue = 120;
    ESP_ERROR_CHECK(g_leds->set_rgb(g_leds, 120, 120, 120));

    xTaskCreate(udp_server_task, "udp_server_task", 1024 * 5, NULL, 5, NULL);

    ota_Semaphore = xSemaphoreCreateBinary();
    xTaskCreate(ota_task, "ota_task", 1024 * 5, NULL, 6, NULL);

}
