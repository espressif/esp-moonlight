// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "driver/ledc.h"

/**
* @brief LED rgb Type
*
*/
typedef struct led_rgb_s led_rgb_t;


/**
* @brief Declare of LED rgb Type
*
*/
struct led_rgb_s {
    /**
    * @brief Set RGB for LED
    *
    * @param led_rgb: Pointer of led_rgb struct
    * @param red: red part of color
    * @param green: green part of color
    * @param blue: blue part of color
    *
    * @return
    *      - ESP_OK: Set RGB successfully
    *      - ESP_ERR_INVALID_ARG: Set RGB failed because of invalid parameters
    *      - ESP_FAIL: Set RGB failed because other error occurred
    */
    esp_err_t (*set_rgb)(led_rgb_t *led_rgb, uint32_t red, uint32_t green, uint32_t blue);

    /**
    * @brief Set HSV for LED
    *
    * @param led_rgb: Pointer of led_rgb struct
    * @param hue: hue part of color
    * @param saturation: saturation part of color
    * @param value: value part of color
    *
    * @return
    *      - ESP_OK: Set HSV successfully
    *      - ESP_ERR_INVALID_ARG: Set HSV failed because of invalid parameters
    *      - ESP_FAIL: Set HSV failed because other error occurred
    */
    esp_err_t (*set_hsv)(led_rgb_t *led_rgb, uint32_t hue, uint32_t saturation, uint32_t value);

    /**
    * @brief Get RGB for LED
    *
    * @param led_rgb: Pointer of led_rgb struct
    * @param red: red part of color
    * @param green: green part of color
    * @param blue: blue part of color
    *
    * @return
    *      - ESP_OK: Get RGB successfully
    */
    esp_err_t (*get_rgb)(led_rgb_t *led_rgb, uint8_t *red, uint8_t *green, uint8_t *blue);

    /**
    * @brief Get HSV for LED
    *
    * @param led_rgb: Pointer of led_rgb struct
    * @param hue: hue part of color
    * @param saturation: saturation part of color
    * @param value: value part of color
    *
    * @return
    *      - ESP_OK: Get HSV successfully
    */
    esp_err_t (*get_hsv)(led_rgb_t *led_rgb, uint32_t *hue, uint32_t *saturation, uint32_t *value);

    /**
    * @brief Clear LED rgb (turn off LED)
    *
    * @param led_rgb: Pointer of led_rgb struct
    *
    * @return
    *      - ESP_OK: Clear LEDs successfully
    *      - ESP_ERR_TIMEOUT: Clear LEDs failed because of timeout
    *      - ESP_FAIL: Clear LEDs failed because some other error occurred
    */
    esp_err_t (*clear)(led_rgb_t *led_rgb);

    /**
    * @brief Free LED rgb resources
    *
    * @param led_rgb: Pointer of led_rgb struct
    *
    * @return
    *      - ESP_OK: Free resources successfully
    *      - ESP_FAIL: Free resources failed because error occurred
    */
    esp_err_t (*del)(led_rgb_t *led_rgb);
};

/**
* @brief LED rgb Configuration Type
*
*/
typedef struct {
    ledc_mode_t speed_mode;         /*!< LEDC speed speed_mode, high-speed mode or low-speed mode */
    ledc_timer_t timer_sel;         /*!< Select the timer source of channel (0 - 3) */ 

    int32_t red_gpio_num;      /*!< Red LED pwm gpio number */
    int32_t green_gpio_num;    /*!< Green LED pwm gpio number */
    int32_t blue_gpio_num;     /*!< Blue LED pwm gpio number */
    ledc_channel_t red_ledc_ch;      /*!< Red LED LEDC channel */
    ledc_channel_t green_ledc_ch;    /*!< Green LED LEDC channel */
    ledc_channel_t blue_ledc_ch;     /*!< Blue LED LEDC channel */

    uint32_t freq;
    uint32_t resolution;
} led_rgb_config_t;


#define LED_RGB_DEFAULT_CONFIG(red_gpio, green_gpio, blue_gpio) {  \
    .red_gpio_num   = (red_gpio),         \
    .green_gpio_num = (green_gpio),       \
    .blue_gpio_num  = (blue_gpio),        \
    .red_ledc_ch    = LEDC_CHANNEL_0,     \
    .green_ledc_ch  = LEDC_CHANNEL_1,     \
    .blue_ledc_ch   = LEDC_CHANNEL_2,     \
    .speed_mode = LEDC_LOW_SPEED_MODE,    \
    .timer_sel  = LEDC_TIMER_0,           \
    .freq       = 20000,                  \
    .resolution = LEDC_TIMER_8_BIT,       \
}

/**
* @brief Create a new LED driver (based on LEDC peripheral)
*
* @param config: Pointer of led_rgb_config_t struct
* @return
*      LED instance or NULL
*/
led_rgb_t *led_rgb_create(const led_rgb_config_t *cfg);





#ifdef __cplusplus
}
#endif
