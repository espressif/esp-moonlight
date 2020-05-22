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


#include "sdkconfig.h"

#pragma once

#ifdef CONFIG_IDF_TARGET_ESP32S2

#define BOARD_BAT_ADC_CHANNEL 3
#define BOARD_GPIO_SENSOR_INT 5
#define BOARD_DMIC_I2S_SCK    15
#define BOARD_DMIC_I2S_WS     16
#define BOARD_DMIC_I2S_SDO    17
#define BOARD_GPIO_LED_R      36
#define BOARD_GPIO_LED_G      35
#define BOARD_GPIO_LED_B      37
#define BOARD_GPIO_BAT_CHRG   6
#define BOARD_GPIO_BAT_STBY   7

#elif defined CONFIG_IDF_TARGET_ESP32

#define BOARD_BAT_ADC_CHANNEL 0
#define BOARD_GPIO_SENSOR_INT 39
#define BOARD_DMIC_I2S_SCK    32
#define BOARD_DMIC_I2S_WS     33
#define BOARD_DMIC_I2S_SDO    25
#define BOARD_GPIO_LED_R      16
#define BOARD_GPIO_LED_G      4
#define BOARD_GPIO_LED_B      17
#define BOARD_GPIO_BAT_CHRG   34
#define BOARD_GPIO_BAT_STBY   35

#endif 

#define BOARD_GPIO_BUTTON      0  /**< button gpio number */
