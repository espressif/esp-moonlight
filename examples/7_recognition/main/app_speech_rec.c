/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_idf_version.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "spi_flash_mmap.h"
#include "driver/i2s_std.h"
#else
#include "esp_spi_flash.h"
#include "driver/i2s.h"
#endif
#include "xtensa/core-macros.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "dl_lib_coefgetter_if.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "app_speech_if.h"
#include "board_moonlight.h"
#include "model_path.h"
#include "esp_process_sdkconfig.h"
#include "esp_mn_speech_commands.h"
#if BOARD_SUPPORT_SPEAKER
#include "audio_player.h"
#endif

static const char *TAG = "speeech recognition";
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static i2s_chan_handle_t rx_chan;        // I2S rx channel handler
#endif
typedef struct {
    sr_cb_t fn;   /*!< function */
    void *args;   /*!< function args */
} func_t;

static func_t g_sr_callback_func[SR_CB_TYPE_MAX] = {0};

#define COMMANDS_NUM 11

char *commands[] = {
    "da kai dian deng",
    "kai deng",
    "da kai xiao ye deng",
    "guan bi dian deng",
    "guan deng",
    "guan bi xiao ye deng",
    "huan yi ge yan se",
    "liang yi dian",
    "zeng da liang du",
    "an yi dian",
    "jian xiao liang du",
};

#if BOARD_SUPPORT_SPEAKER

#define SPIFFS_BASE       "/spiffs"

static i2s_chan_handle_t tx_chan;        // I2S tx device handler

static esp_err_t audio_player_mute(AUDIO_PLAYER_MUTE_SETTING setting)
{
    return ESP_OK;
}

static esp_err_t audio_player_reconfig_std(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    return ESP_OK;
}

static esp_err_t audio_player_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    return i2s_channel_write(tx_chan, audio_buffer, len, bytes_written, timeout_ms);
}

#endif

static void i2s_init(void)
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    i2s_chan_config_t rx_chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 3,
        .dma_frame_num = 300,
        .auto_clear = true,
    };
    ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, NULL, &rx_chan));

    i2s_std_config_t rx_std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,    // some codecs may require mclk signal, this example doesn't need it
            .bclk = BOARD_DMIC_I2S_SCK,
            .ws   = BOARD_DMIC_I2S_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = BOARD_DMIC_I2S_SDO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &rx_std_cfg));
    i2s_channel_enable(rx_chan);

#if BOARD_SUPPORT_SPEAKER
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(1, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));
    i2s_std_config_t tx_std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = BOARD_DSPEAKER_I2S_BCK,
            .ws = BOARD_DSPEAKER_I2S_WS,
            .dout = BOARD_DSPEAKER_I2S_SDO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    tx_std_cfg.clk_cfg.mclk_multiple = 384;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &tx_std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
#endif

#else
    i2s_config_t rx_i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,           // the mode must be set according to DSP configuration
        .sample_rate = 16000,                            // must be the same as DSP configuration
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,    // must be the same as DSP configuration
        .bits_per_sample = 32,                           // must be the same as DSP configuration
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .dma_buf_count = 3,
        .dma_buf_len = 300,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,
    };
    i2s_pin_config_t rx_pin_config = {
        .bck_io_num = BOARD_DMIC_I2S_SCK,  // IIS_SCLK
        .ws_io_num = BOARD_DMIC_I2S_WS,    // IIS_LCLK
        .data_out_num = -1,                // IIS_DSIN
        .data_in_num = BOARD_DMIC_I2S_SDO  // IIS_DOUT
    };
    i2s_driver_install(1, &rx_i2s_config, 0, NULL);
    i2s_set_pin(1, &rx_pin_config);
    i2s_zero_dma_buffer(1);
#endif

}

void recsrcTask(void *arg)
{
    srmodel_list_t *models = esp_srmodel_init("model");
    const char *wn_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL); // select WakeNet model
    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_CHINESE); // select MultiNet model
    const esp_wn_iface_t *wakenet = esp_wn_handle_from_name(wn_name);
    esp_mn_iface_t *multinet = esp_mn_handle_from_name(mn_name);
    model_iface_data_t *model_wn_data = wakenet->create(wn_name, DET_MODE_90);
    int wn_num = wakenet->get_word_num(model_wn_data);
    float wn_threshold = 0;
    int wn_sample_rate = wakenet->get_samp_rate(model_wn_data);
    int audio_wn_chunksize = wakenet->get_samp_chunksize(model_wn_data);
    ESP_LOGI(TAG, "keywords_num = %d, threshold = %f, sample_rate = %d, chunksize = %d, sizeof_uint16 = %d", wn_num, wn_threshold, wn_sample_rate, audio_wn_chunksize, sizeof(int16_t));

    model_iface_data_t *model_mn_data = multinet->create(mn_name, 6000);
    esp_mn_commands_clear();
    for (int i = 0; i<COMMANDS_NUM ; i++) {
        esp_mn_commands_add(i, commands[i]);
    }
    esp_mn_commands_update();

    int audio_mn_chunksize = multinet->get_samp_chunksize(model_mn_data);
    int mn_num = multinet->get_samp_chunknum(model_mn_data);
    int mn_sample_rate = multinet->get_samp_rate(model_mn_data);
    ESP_LOGI(TAG, "keywords_num = %d , sample_rate = %d, chunksize = %d, sizeof_uint16 = %d", mn_num,  mn_sample_rate, audio_mn_chunksize, sizeof(int16_t));

    int size = audio_wn_chunksize;

    if (audio_mn_chunksize > audio_wn_chunksize) {
        size = audio_mn_chunksize;
    }
    int *buffer = (int *)malloc(size * 2 * sizeof(int));
    bool enable_wn = true;

    i2s_init();

    size_t read_len = 0;
    while (1) {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        i2s_channel_read(rx_chan, buffer, size * 2 * sizeof(int), &read_len, portMAX_DELAY);
#else
        i2s_read(1, buffer, size * 2 * sizeof(int), &read_len, portMAX_DELAY);
#endif
        for (int x = 0; x < size * 2 / 4; x++) {
            int s1 = ((buffer[x * 4] + buffer[x * 4 + 1]) >> 13) & 0x0000FFFF;
            int s2 = ((buffer[x * 4 + 2] + buffer[x * 4 + 3]) << 3) & 0xFFFF0000;
            buffer[x] = s1 | s2;
        }

        if (enable_wn) {

            wakenet_state_t r = wakenet->detect(model_wn_data, (int16_t *)buffer);

            if (r == WAKENET_DETECTED) {
                ESP_LOGI(TAG, "%s DETECTED", wakenet->get_word_name(model_wn_data, r));

                if (NULL != g_sr_callback_func[SR_CB_TYPE_WAKE].fn) {
                    g_sr_callback_func[SR_CB_TYPE_WAKE].fn(g_sr_callback_func[SR_CB_TYPE_WAKE].args);
                }

                enable_wn = false;
            }
        } else {
            esp_mn_state_t mn_state = multinet->detect(model_mn_data, (int16_t *)buffer);

            if (mn_state == ESP_MN_STATE_DETECTED) {
                esp_mn_results_t *mn_result = multinet->get_results(model_mn_data);
                ESP_LOGI(TAG, "MN test successfully, Commands ID: %d", mn_result->phrase_id[0]);
                int command_id = mn_result->phrase_id[0];

                if (NULL != g_sr_callback_func[SR_CB_TYPE_CMD].fn) {
                    if (NULL != g_sr_callback_func[SR_CB_TYPE_CMD].args) {
                        g_sr_callback_func[SR_CB_TYPE_CMD].fn(g_sr_callback_func[SR_CB_TYPE_CMD].args);
                    } else {
                        g_sr_callback_func[SR_CB_TYPE_CMD].fn((void *)command_id);
                    }
                }

            } else if (mn_state == ESP_MN_STATE_TIMEOUT) {
                esp_mn_results_t *mn_result = multinet->get_results(model_mn_data);
                ESP_LOGI(TAG, "timeout, string:%s\n", mn_result->string);

                if (NULL != g_sr_callback_func[SR_CB_TYPE_CMD_EXIT].fn) {
                    g_sr_callback_func[SR_CB_TYPE_CMD_EXIT].fn(g_sr_callback_func[SR_CB_TYPE_CMD_EXIT].args);
                }

                enable_wn = true;
            } else {
                continue;
            }
        }
    }

    vTaskDelete(NULL);
}

esp_err_t sr_handler_install(sr_cb_type_t type, sr_cb_t handler, void *args)
{
    switch (type) {
        case SR_CB_TYPE_WAKE:
            g_sr_callback_func[SR_CB_TYPE_WAKE].fn = handler;
            g_sr_callback_func[SR_CB_TYPE_WAKE].args = args;
            break;

        case SR_CB_TYPE_CMD:
            g_sr_callback_func[SR_CB_TYPE_CMD].fn = handler;
            g_sr_callback_func[SR_CB_TYPE_CMD].args = args;
            break;

        case SR_CB_TYPE_CMD_EXIT:
            g_sr_callback_func[SR_CB_TYPE_CMD_EXIT].fn = handler;
            g_sr_callback_func[SR_CB_TYPE_CMD_EXIT].args = args;
            break;

        default:
            return ESP_ERR_INVALID_ARG;
            break;
    }

    return ESP_OK;
}


esp_err_t speech_recognition_init(void)
{
    xTaskCreatePinnedToCore(recsrcTask, "recsrcTask", 8 * 1024, NULL, 8, NULL, 1);
#if BOARD_SUPPORT_SPEAKER
    printf("Initializing SPIFFS\n");
    esp_vfs_spiffs_conf_t conf = {
        .base_path = SPIFFS_BASE,
        .partition_label = NULL,
        .max_files = 2,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    audio_player_config_t config = {
        .mute_fn = audio_player_mute,
        .clk_set_fn = audio_player_reconfig_std,
        .write_fn = audio_player_write,
    };
    ESP_ERROR_CHECK(audio_player_new(config));
    ESP_LOGI(TAG, "audio player init success");
#endif
    return ESP_OK;
}
