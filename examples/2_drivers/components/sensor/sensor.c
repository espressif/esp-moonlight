/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "sensor.h"

static const char *TAG = "sensor";

static xQueueHandle g_gpio_evt_queue  = NULL;
static vibration_isr_t g_vibration_fn = NULL;
static void *g_vibration_fn_arg       = NULL;

#define DEFAULT_VREF    1100        /**< Use adc2_vref_to_gpio() to obtain a better estimate */
#define NO_OF_SAMPLES   16          /**< Multisampling */
static int32_t g_adc_ch_bat = 0;
static esp_adc_cal_characteristics_t *g_adc_chars;
static int32_t g_vol_bat = 0;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(g_gpio_evt_queue, &gpio_num, NULL);
}

esp_err_t sensor_vibration_triggered_register(vibration_isr_t fn, void *arg)
{
    if (NULL == fn) {
        return ESP_ERR_INVALID_ARG;
    }

    g_vibration_fn = fn;
    g_vibration_fn_arg = arg;
    return ESP_OK;

}

static void sensor_vibration_task(void *arg)
{
    int32_t gpio_num = (int32_t)arg;
    while (1) {
        uint32_t io_num;
        uint8_t level;

        if (xQueueReceive(g_gpio_evt_queue, &io_num, portMAX_DELAY)) {
            level = gpio_get_level(io_num);
            ESP_LOGI(TAG, "GPIO[%d] intr, val: %d\n", io_num, level);

            if (io_num == gpio_num) {
                /**
                 * remove isr handler for gpio number.
                 * Discard interrupts generated during the delay period
                 */
                gpio_isr_handler_remove(gpio_num);

                if (NULL != g_vibration_fn) {
                    g_vibration_fn(g_vibration_fn_arg);
                }

                vTaskDelay(pdMS_TO_TICKS(250));
                /**< hook isr handler for specific gpio pin */
                gpio_isr_handler_add(gpio_num, gpio_isr_handler, (void *) gpio_num);
            }
        }
    }
}

esp_err_t sensor_vibration_init(int32_t gpio_num)
{
    gpio_config_t io_conf = {0};

    /**< interrupt of rising edge */
    io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE;
    /**< bit mask of the pins */
    io_conf.pin_bit_mask = (((uint64_t) 1) << gpio_num);
    /**< set as input mode */
    io_conf.mode = GPIO_MODE_INPUT;
    /**< enable pull-up mode */
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    /**< create a queue to handle gpio event from isr */
    g_gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));

    if (g_gpio_evt_queue == NULL) {
        return ESP_FAIL;
    }

    /**< install gpio isr service */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(gpio_num, gpio_isr_handler, (void *) gpio_num);

    xTaskCreatePinnedToCore(sensor_vibration_task, "vibration", 1024 * 2, (void*)gpio_num, 3, NULL, 1);

    return ESP_OK;
}



static void adc_check_efuse()
{
    /**< Check TP is burned into eFuse */
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGI(TAG, "ADC eFuse Two Point: Supported");
    } else {
        ESP_LOGI(TAG, "ADC eFuse Two Point: NOT supported");
    }

    /**< Check Vref is burned into eFuse */
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGI(TAG, "ADC eFuse Vref: Supported");
    } else {
        ESP_LOGI(TAG, "ADC eFuse Vref: NOT supported");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGI(TAG, "ADC Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI(TAG, "ADC Characterized using eFuse Vref\n");
    } else {
        ESP_LOGI(TAG, "ADC Characterized using Default Vref\n");
    }
}

static void adc_get_voltage(int32_t *out_voltage)
{
    static uint32_t sample_index = 0;
    static uint16_t filter_buf[NO_OF_SAMPLES] = {0};

    uint32_t adc_reading = adc1_get_raw(g_adc_ch_bat);
    filter_buf[sample_index++] = adc_reading;

    if (NO_OF_SAMPLES == sample_index) {
        sample_index = 0;
    }

    uint32_t sum = 0;

    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        sum += filter_buf[i];
    }

    sum /= NO_OF_SAMPLES;
    /**< Convert adc_reading to voltage in mV */
    uint32_t voltage = esp_adc_cal_raw_to_voltage(sum, g_adc_chars);
    *out_voltage = voltage;
}

static void adc_proid_sample(TimerHandle_t xTimer)
{
    adc_get_voltage(&g_vol_bat);
    /**< The resistance on the hardware has decreased twice */
    g_vol_bat *= 2;
}

static void sensor_battery_task(void *arg)
{
    static TimerHandle_t tmr;
    tmr = xTimerCreate("adc measure",
                       10 / portTICK_PERIOD_MS,
                       pdTRUE,
                       NULL,
                       adc_proid_sample);

    if (tmr) {
        xTimerStart(tmr, 0);
    }

    while (1) {
        ESP_LOGI(TAG, "battery voltage: %dmv", g_vol_bat);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

esp_err_t sensor_adc_init(int32_t adc_channel)
{
    g_adc_ch_bat = adc_channel;
    /**< Check if Two Point or Vref are burned into eFuse */
    adc_check_efuse();

    /**< Configure ADC */
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(g_adc_ch_bat, ADC_ATTEN_DB_11);

    /**< Characterize ADC */
    g_adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, g_adc_chars);
    print_char_val_type(val_type);

    xTaskCreatePinnedToCore(sensor_battery_task, "battery", 1024 * 2, NULL, 3, NULL, 1);
    
    return ESP_OK;
}



