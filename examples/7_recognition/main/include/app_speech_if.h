/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef SRC_IF_H
#define SRC_IF_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef void (*sr_cb_t)(void*);

typedef enum {
    SR_CB_TYPE_WAKE     = 0,     /*!< Wake up callback function */
    SR_CB_TYPE_CMD      = 1,     /*!< Command callback function */
    SR_CB_TYPE_CMD_EXIT = 2,     /*!< Exit command mode, wait for wakeup*/
    SR_CB_TYPE_MAX,
} sr_cb_type_t;

/**
* @brief Initialize speech recognition
*
* @return
*      - ESP_OK: Create speech recognition task successfully
*/
esp_err_t speech_recognition_init(void);

/**
* @brief Install an handler to the speech recognition.
*
* @param type: Handler type ,see sr_cb_type_t
* @param handler: Pointer of callback function
* @param args: Parameter of callback function
*
* @return
*      - ESP_OK: Install handler successfully
*      - ESP_ERR_INVALID_ARG: Callback handler type invaild
*/
esp_err_t sr_handler_install(sr_cb_type_t type, sr_cb_t handler, void *args);


#endif
