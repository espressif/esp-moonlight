语音识别控制
=============



这个示例将展示如何使用一个乐鑫提供的语音识别相关方向算法模型 ESP-SR 来控制 LED 灯。如需查看相关代码，请前往 ``examples/7_recognition`` 目录。

该示例包含以下功能：

-  使用语音唤醒开发板
-  支持连续对话
-  语音控制 LED 灯的开关、颜色、亮度


ESP-SR 介绍
---------------

包含三个主要模块：

- 唤醒词识别模型：WakeNet
- 语音命令识别模型：MultiNet
- 声学算法：麦克风阵列语音增强（Mic-Array Speech Enhancement，简称 MASE）、回声消除（Acoustic Echo Cancellation，简称 AEC）、语音端点检测（Voice Activity Detection，简称 VAD）、自动增益控制（Automatic Gain Control，简称 AGC）、噪声抑制（Noise Suppression，简称 NS）

唤醒词模型 WakeNet 目前提供了免费的 “Hi，乐鑫” 唤醒词，这也是我们这里使用的唤醒词。命令词识别模型 MultiNet 则允许用户自定义语音命令而无需重新训练模型。

详细信息可参见 `ESP-SR <https://github.com/espressif/esp-sr>`_。


麦克风输入
---------------

要使用语音识别控制，首先应该有一个音频输入。开发板使用了一个 I2S 数字输出的 MEMS 麦克风直接与 ESP32 的 I2S 接口连接。

硬件接口如下：

+--------------+------------------+---------------+-----------------+
|   Pin Name   | ESP32-S3-WROOM-1 | ESP32-S2-SOLO | ESP32-WROOM-32D |
+==============+==================+===============+=================+
| DMIC_I2S_SCK | IO39             | IO15          | IO32            |
+--------------+------------------+---------------+-----------------+
| DMIC_I2S_WS  | IO38             | IO16          | IO32            |
+--------------+------------------+---------------+-----------------+
| DMIC_I2S_SDO | IO40             | IO17          | IO25            |
+--------------+------------------+---------------+-----------------+

这种设计无需使用外置 Audio Codec ，极大地简化了电路。

扬声器输出
----------------

同时该实例还支持一路扬声器输出，通过 I2S 接口连接。硬件接口如下：

+------------------+------------------+---------------+-----------------+
|     Pin Name     | ESP32-S3-WROOM-1 | ESP32-S2-SOLO | ESP32-WROOM-32D |
+==================+==================+===============+=================+
| DSPEAKER_I2S_BCK | IO48             | ×             | ×               |
+------------------+------------------+---------------+-----------------+
| DSPEAKER_I2S_WS  | IO45             | ×             | ×               |
+------------------+------------------+---------------+-----------------+
| DSPEAKER_I2S_SDO | IO47             | ×             | ×               |
+------------------+------------------+---------------+-----------------+

.. note::
   `ESP32S3` 开启扬声器功能，需使用 IDF 版本 v5.0 及以上。

代码
---------

以下所示为调用语音识别模型的部分代码：

.. code-block:: c

   i2s_read(1, buffer, size * 2 * sizeof(int), &read_len, portMAX_DELAY);

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

- 首先调用 :c:func:`i2s_read` 从麦克风读取一段音频数据，然后进行数据格式的调整。
- 根据 ``enable_wn`` 变量来控制使用唤醒识别还是命令词识别。
- 调用 :c:func:`detect` 函数将音频数据送入对应的识别网络进行识别。
- 在识别命令词时，当识别超时时回到唤醒词识别状态。


命令词定义
---------------

``app_speech_rec.c`` 文件中定义了 11 条控制命令，如下所示：

::

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

   esp_mn_commands_clear();
   for (int i = 0; i<COMMANDS_NUM ; i++) {
      esp_mn_commands_add(i, commands[i]);
   }
   esp_mn_commands_update();


通过调用 `esp_mn_commands_add` 接口添加命令词，你也可以其他方式添加自己的语音命令，具体方法可参见 `MultiNet 介绍 <https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/speech_command_recognition/README.html>`_。
请注意，添加语音命令后需更改语音命令的数量，使之显示实际数量。


演示
---------------

- 先说出唤醒词“Hi，乐鑫”，唤醒开发板，让 ESP32 运行命令词识别模型，此时 LED 呈现绿色呼吸灯状态。
- 唤醒后可说出“打开电灯”、“关闭电灯”、“增大亮度”等命令来控制灯的状态，前文已列出可支持的全部语音指令。
- 支持连续对话，唤醒一次后，可连续识别多条命令。
- 唤醒后亮绿色呼吸灯为命令词识别状态，若一段时间后未识别到有效指令，开发板将自动回到等待唤醒的状态。

