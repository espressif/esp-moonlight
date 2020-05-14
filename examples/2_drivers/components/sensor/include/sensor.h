/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef SENSOR_H
#define SENSOR_H

#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"


typedef void (*vibration_isr_t)(void*);


/**
 * @brief   initialize adc for measure battery voltage
 *
 * @param  adc_channel  ADC channel connected to voltage divider
 * 
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL error
 */
esp_err_t sensor_adc_init(int32_t adc_channel);

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