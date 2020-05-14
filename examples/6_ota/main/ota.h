/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef OTA_H
#define OTA_H

#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"



/**
 * @brief  start firmware upgrade
 *
 * @param  url  HTTP URL, the information on the URL is most important
 * 
 * @return
 *     - ESP_OK Success ;
 *     - ESP_ERR_INVALID_ARG pointer fn error
 */
esp_err_t ota_start(const char *url);


#endif