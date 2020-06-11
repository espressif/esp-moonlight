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


#ifndef SENSOR_H
#define SENSOR_H

#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"


typedef void (*vibration_isr_t)(void *);

typedef enum {
    CHRG_STATE_UNKNOW   = 0x00,
    CHRG_STATE_CHARGING,
    CHRG_STATE_FULL,
} chrg_state_t;


/**
 * @brief   initialize adc for measure battery voltage
 *
 * @param  adc_channel  ADC channel connected to voltage divider
 * @param  chrg_num  Connected to battery charge chip CHRG pin
 * @param  stby_num  Connected to battery charge chip STBY pin
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL error
 */
esp_err_t sensor_battery_init(int32_t adc_channel, int32_t chrg_num, int32_t stby_num);

/**
 * @brief   Get battery info, include voltage and charge state
 *
 * @param  voltage  battery voltage(mv)
 * @param  chrg_state  charge state
 *
 * @return
 *     - ESP_OK Success
 */
esp_err_t sensor_battery_get_info(int32_t *voltage, uint8_t *chrg_state);

/**
 * @brief   Get battery simple info , include battery level and charge state
 *
 * @param  level  battery level (0 ~ 100)
 * @param  state  charge state, see struct chrg_state_t
 *
 * @return
 *     - ESP_OK Success
 */
esp_err_t sensor_battery_get_info_simple(int32_t *level, chrg_state_t *state);

/**
 * @brief   initialize vabration sensors
 *
 * @param  gpio_num  vibration sensor gpio number
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL error
 */
esp_err_t sensor_vibration_init(int32_t gpio_num);

/**
 * @brief   Register vabration sensor triggered handler function. Call this function when a vibration is detected
 *
 * @param  fn  Triggered handler function.
 * @param  arg  Parameter for handler function
 *
 * @return
 *     - ESP_OK Success ;
 *     - ESP_ERR_INVALID_ARG pointer fn error
 */
esp_err_t sensor_vibration_triggered_register(vibration_isr_t fn, void *arg);


#endif