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

#include "esp_err.h"
#include "esp_blufi_api.h"

#define BLUFI_EXAMPLE_TAG "Blufi"
#define BLUFI_INFO(fmt, ...)   ESP_LOGI(BLUFI_EXAMPLE_TAG, fmt, ##__VA_ARGS__) 
#define BLUFI_ERROR(fmt, ...)  ESP_LOGE(BLUFI_EXAMPLE_TAG, fmt, ##__VA_ARGS__) 

typedef enum
{
    BLUFI_STATUS_DISABLE       = 0x00,
    BLUFI_STATUS_BT_CONNECTED  = 0x01,
    BLUFI_STATUS_STA_CONNECTED = 0x02,
    
}blufi_status_t;

typedef void (*recv_handle_t)(esp_blufi_cb_event_t event, void* param);

void blufi_dh_negotiate_data_handler(uint8_t *data, int len, uint8_t **output_data, int *output_len, bool *need_free);
int blufi_aes_encrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len);
int blufi_aes_decrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len);
uint16_t blufi_crc_checksum(uint8_t iv8, uint8_t *data, int len);

int blufi_security_init(void);
void blufi_security_deinit(void);

esp_err_t blufi_start(void);
esp_err_t blufi_stop(void);
esp_err_t blufi_network_init(void);
esp_err_t blufi_wait_connection(TickType_t xTicksToWait);
esp_err_t blufi_get_status(blufi_status_t *status);
esp_err_t blufi_is_configured(bool *configured);
esp_err_t blufi_install_recv_handle(recv_handle_t fn);
esp_err_t blufi_set_wifi_info(const char *ssid, const char *pswd);
