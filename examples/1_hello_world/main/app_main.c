/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_moonlight.h"

void app_main()
{
    int i = 0;
#if CONFIG_ESP32_MOONLIGHT_BOARD
    printf("Select ESP32_MOONLIGHT_BOARD\n");
#elif CONFIG_ESP32_S3_MOONLIGHT_BOARD
    printf("Select ESP32_S3_MOONLIGHT_BOARD\n");
#endif

    while (1) {
        printf("[%d] Hello world!\n", i);
        i++;
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}