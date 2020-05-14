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
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "led_rgb.h"
#include "driver/ledc.h"


static const char *TAG = "led_rgb";
#define LED_RGB_CHECK(a, str, goto_tag, ret_value, ...)                             \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            ret = ret_value;                                                      \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)

typedef struct {
    led_rgb_t parent;
    ledc_mode_t speed_mode[3];
    ledc_channel_t channel[3];
    uint32_t rgb[3];
} led_pwm_t;

static const uint8_t LEDGammaTable[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 11, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21,
    22, 23, 23, 24, 24, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32, 33, 34, 35, 35, 36, 37, 38,
    38, 39, 40, 41, 42, 42, 43, 44, 45, 46, 47, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58,
    59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 84,
    85, 86, 87, 88, 89, 91, 92, 93, 94, 95, 97, 98, 99, 100, 102, 103, 104, 105, 107, 108, 109, 111,
    112, 113, 115, 116, 117, 119, 120, 121, 123, 124, 126, 127, 128, 130, 131, 133, 134, 136, 137,
    139, 140, 142, 143, 145, 146, 148, 149, 151, 152, 154, 155, 157, 158, 160, 162, 163, 165, 166,
    168, 170, 171, 173, 175, 176, 178, 180, 181, 183, 185, 186, 188, 190, 192, 193, 195, 197, 199,
    200, 202, 204, 206, 207, 209, 211, 213, 215, 217, 218, 220, 222, 224, 226, 228, 230, 232, 233,
    235, 237, 239, 241, 243, 245, 247, 249, 251, 253, 255
};


/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
static void led_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; /**< h -> [0,360] */
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    /**< RGB adjustment amount by hue */
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
        case 0:
            *r = rgb_max;
            *g = rgb_min + rgb_adj;
            *b = rgb_min;
            break;

        case 1:
            *r = rgb_max - rgb_adj;
            *g = rgb_max;
            *b = rgb_min;
            break;

        case 2:
            *r = rgb_min;
            *g = rgb_max;
            *b = rgb_min + rgb_adj;
            break;

        case 3:
            *r = rgb_min;
            *g = rgb_max - rgb_adj;
            *b = rgb_max;
            break;

        case 4:
            *r = rgb_min + rgb_adj;
            *g = rgb_min;
            *b = rgb_max;
            break;

        default:
            *r = rgb_max;
            *g = rgb_min;
            *b = rgb_max - rgb_adj;
            break;
    }
}

void led_rgb2hsv(uint8_t in_r, uint8_t in_g, uint8_t in_b, uint32_t *H, uint32_t *S, uint32_t *V)
{
    int32_t R = in_r;
    int32_t G = in_g;
    int32_t B = in_b;
    int32_t min, max, delta, tmp;
    tmp = R > G ? G : R;
    min = tmp > B ? B : tmp;
    tmp = R > G ? R : G;
    max = tmp > B ? tmp : B;
    *V = 100 * max / 255; /**< v */
    delta = max - min;

    if (max != 0) {
        *S = 100 * delta / max; /**< s */
    } else {
        /**< r = g = b = 0 */
        *S = 0;
        *H = 0;
        return;
    }

    float h_temp = 0;

    if (delta == 0) {
        *H = 0;
        return;
    } else if (R == max) {
        h_temp = ((float)(G - B) / (float)delta);           /**< between yellow & magenta */
    } else if (G == max) {
        h_temp = 2.0f + ((float)(B - R) / (float)delta);    /**< between cyan & yellow */
    } else if (B == max) {
        h_temp = 4.0f + ((float)(R - G) / (float)delta);    /**< between magenta & cyan */
    }

    h_temp *= 60.0f;

    if (h_temp < 0.0f) {
        h_temp += 360;
    }

    *H = (uint32_t)h_temp; /**< degrees */
}

static esp_err_t led_set_rgb(led_rgb_t *led_rgb, uint32_t red, uint32_t green, uint32_t blue)
{
    led_pwm_t *led_pwm = __containerof(led_rgb, led_pwm_t, parent);

    led_pwm->rgb[0] = red;
    led_pwm->rgb[1] = green;
    led_pwm->rgb[2] = blue;

    uint8_t rgb[3];
    rgb[0] = LEDGammaTable[red];
    rgb[1] = LEDGammaTable[green];
    rgb[2] = LEDGammaTable[blue];

    for (size_t i = 0; i < 3; i++) {
        ledc_set_duty(led_pwm->speed_mode[i], led_pwm->channel[i], rgb[i]);
        ledc_update_duty(led_pwm->speed_mode[i], led_pwm->channel[i]);
    }

    return ESP_OK;

}

static esp_err_t led_set_hsv(led_rgb_t *led_rgb, uint32_t h, uint32_t s, uint32_t v)
{
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    /**< convert to RGB */
    led_hsv2rgb(h, s, v, &red, &green, &blue);
    return led_set_rgb(led_rgb, red, green, blue);
}

static esp_err_t led_get_rgb(led_rgb_t *led_rgb, uint8_t *red, uint8_t *green, uint8_t *blue)
{
    led_pwm_t *led_pwm = __containerof(led_rgb, led_pwm_t, parent);
    *red   = led_pwm->rgb[0];
    *green = led_pwm->rgb[1];
    *blue  = led_pwm->rgb[2];
    return ESP_OK;
}

static esp_err_t led_get_hsv(led_rgb_t *led_rgb, uint32_t *h, uint32_t *s, uint32_t *v)
{
    led_pwm_t *led_pwm = __containerof(led_rgb, led_pwm_t, parent);

    led_rgb2hsv(led_pwm->rgb[0], led_pwm->rgb[1], led_pwm->rgb[2], h, s, v);
    return ESP_OK;
}

static esp_err_t led_del(led_rgb_t *led_rgb)
{
    led_pwm_t *led_pwm = __containerof(led_rgb, led_pwm_t, parent);
    free(led_pwm);
    return ESP_OK;
}

static esp_err_t led_clear(led_rgb_t *led_rgb)
{
    led_pwm_t *led_pwm = __containerof(led_rgb, led_pwm_t, parent);

    for (size_t i = 0; i < 3; i++) {
        ledc_set_duty(led_pwm->speed_mode[i], led_pwm->channel[i], 0);
        ledc_update_duty(led_pwm->speed_mode[i], led_pwm->channel[i]);
    }

    return ESP_OK;
}


led_rgb_t *led_rgb_create(const led_rgb_config_t *cfg)
{
    led_rgb_t *ret = NULL;
    LED_RGB_CHECK(cfg, "configuration can't be null", err, NULL);

    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = cfg->resolution,     /**< resolution of PWM duty */
        .freq_hz         = cfg->freq,           /**< frequency of PWM signal */
        .speed_mode      = cfg->speed_mode,     /**< timer mode */
        .timer_num       = cfg->timer_sel,      /**< timer index */
        .clk_cfg         = LEDC_USE_APB_CLK,    /**< Auto select the source clock */
    };
    /**< Set configuration of timer for low speed channels */
    LED_RGB_CHECK(ledc_timer_config(&ledc_timer) == ESP_OK,
                  "ledc timer config failed", err, NULL);

    /*
     * Prepare and set configuration of ledc channels
     */
    ledc_channel_config_t ledc_channel = {0};
    ledc_channel.channel    = cfg->red_ledc_ch;
    ledc_channel.duty       = 0;
    ledc_channel.gpio_num   = cfg->red_gpio_num;
    ledc_channel.speed_mode = cfg->speed_mode;
    ledc_channel.hpoint     = 0;
    ledc_channel.timer_sel  = cfg->timer_sel;
    LED_RGB_CHECK(ledc_channel_config(&ledc_channel) == ESP_OK,
                  "ledc channel config failed", err, NULL);

    ledc_channel.channel    = cfg->green_ledc_ch;
    ledc_channel.gpio_num   = cfg->green_gpio_num;
    LED_RGB_CHECK(ledc_channel_config(&ledc_channel) == ESP_OK,
                  "ledc channel config failed", err, NULL);

    ledc_channel.channel    = cfg->blue_ledc_ch;
    ledc_channel.gpio_num   = cfg->blue_gpio_num;
    LED_RGB_CHECK(ledc_channel_config(&ledc_channel) == ESP_OK,
                  "ledc channel config failed", err, NULL);

    /**< alloc memory for led */
    led_pwm_t *led_pwm = calloc(1, sizeof(led_pwm_t));
    LED_RGB_CHECK(led_pwm, "request memory for led failed", err, NULL);

    for (size_t i = 0; i < 3; i++) {
        led_pwm->speed_mode[i] = ledc_channel.speed_mode;
    }

    led_pwm->channel[0] = cfg->red_ledc_ch;
    led_pwm->channel[1] = cfg->green_ledc_ch;
    led_pwm->channel[2] = cfg->blue_ledc_ch;

    led_pwm->parent.set_rgb = led_set_rgb;
    led_pwm->parent.set_hsv = led_set_hsv;
    led_pwm->parent.get_rgb = led_get_rgb;
    led_pwm->parent.get_hsv = led_get_hsv;
    led_pwm->parent.clear = led_clear;
    led_pwm->parent.del = led_del;

    return &led_pwm->parent;
err:
    return ret;
}
