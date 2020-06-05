语音识别控制
=============



这个示例将展示我们如何使用一个乐鑫提供的语音识别相关方向算法模型 ESP-SR 来控制 LED 灯。如需查看相关代码，请前往 ``examples/7_recognition`` 目录。

该示例包含以下功能：

-  使用语音唤醒开发板
-  语音控制 LED 灯的开关、颜色、亮度


ESP-SR 介绍
---------------

包含三个主要模块：

- 唤醒词识别模型 WakeNet
- 语音命令识别模型 MultiNet
- 声学算法：MASE(Mic Array Speech Enhancement), AEC(Acoustic Echo Cancellation), VAD(Voice Activity Detection), AGC(Automatic Gain Control), NS(Noise Suppression)

唤醒词模型 WakeNet 目前提供了免费的 “Hi，乐鑫” 唤醒词，这也是我们这里使用的唤醒词。命令词识别模型 MultiNet 则允许用户自定义语音命令而无需重新训练模型。

详细信息可参见 `ESP-SR <https://github.com/espressif/esp-sr>`_ 

麦克风输入
---------------
要使用语音识别控制，首先应该有一个音频输入。开发板使用了一个 I2S 数字输出的 MEMS 麦克风直接与 ESP32 的 I2S 接口连接

硬件接口如下：

+--------------+---------------+-----------------+
| Pin Name     | ESP32-S2-SOLO | ESP32-WROOM-32D |
+==============+===============+=================+
| DMIC_I2S_SCK |   IO15        |        IO32     |
+--------------+---------------+-----------------+
| DMIC_I2S_WS  |   IO16        |        IO33     |
+--------------+---------------+-----------------+
| DMIC_I2S_SDO |   IO17        |        IO25     |
+--------------+---------------+-----------------+

这种设计无需使用外置 Audio Codec ，极大的简化了电路。

代码
---------

下面是调用语音识别模型的部分代码

.. code-block:: c

   i2s_read(1, buffer, size * 2 * sizeof(int), &read_len, portMAX_DELAY);

   for (int x = 0; x < size * 2 / 4; x++) {
      int s1 = ((buffer[x * 4] + buffer[x * 4 + 1]) >> 13) & 0x0000FFFF;
      int s2 = ((buffer[x * 4 + 2] + buffer[x * 4 + 3]) << 3) & 0xFFFF0000;
      buffer[x] = s1 | s2;
   }

   if (enable_wn) {

      int r = g_wakenet->detect(g_model_wn_data, (int16_t *)buffer);

      if (r) {
            ESP_LOGI(TAG, "%s DETECTED", g_wakenet->get_word_name(g_model_wn_data, r));

            if (NULL != g_sr_callback_func[SR_CB_TYPE_WAKE].fn) {
               g_sr_callback_func[SR_CB_TYPE_WAKE].fn(g_sr_callback_func[SR_CB_TYPE_WAKE].args);
            }

            enable_wn = false;
      }
   } else {

      mn_count++;
      int command_id = g_multinet->detect(g_model_mn_data, (int16_t *)buffer);

      if (command_id > -1) {
            ESP_LOGI(TAG, "MN test successfully, Commands ID: %d", command_id);

            if (NULL != g_sr_callback_func[SR_CB_TYPE_CMD].fn) {
               if (NULL != g_sr_callback_func[SR_CB_TYPE_CMD].args) {
                  g_sr_callback_func[SR_CB_TYPE_CMD].fn(g_sr_callback_func[SR_CB_TYPE_CMD].args);
               } else {
                  g_sr_callback_func[SR_CB_TYPE_CMD].fn((void *)command_id);
               }
            }

            enable_wn = true;
            mn_count = 0;
      } else {

      }

      if (mn_count == mn_num) {
            ESP_LOGW(TAG, "stop multinet");

            if (NULL != g_sr_callback_func[SR_CB_TYPE_CMD_EXIT].fn) {
               g_sr_callback_func[SR_CB_TYPE_CMD_EXIT].fn(g_sr_callback_func[SR_CB_TYPE_CMD_EXIT].args);
            }

            enable_wn = true;
            mn_count = 0;
      }
   }

- 首先调用 :c:func:`i2s_read` 从麦克风读取一段音频数据，然后进行数据格式的调整
- 根据 ``enable_wn`` 变量来控制使用唤醒识别还是命令词识别
- 调用 :c:func:`detect` 函数将音频数据送入对应的识别网络进行识别
- 在识别命令词时，当识别的帧数达到最大时也就是 mn_count == mn_num 时回到唤醒词识别状态


命令词定义
---------------
在 ``sdkconfig.defaults`` 文件中定义了 11 条控制命令如下：

::

   CONFIG_CN_SPEECH_COMMAND_ID0="da kai dian deng"
   CONFIG_CN_SPEECH_COMMAND_ID1="kai deng"
   CONFIG_CN_SPEECH_COMMAND_ID2="da kai xiao ye deng"
   CONFIG_CN_SPEECH_COMMAND_ID3="guan bi dian deng"
   CONFIG_CN_SPEECH_COMMAND_ID4="guan deng"
   CONFIG_CN_SPEECH_COMMAND_ID5="guan bi xiao ye deng"
   CONFIG_CN_SPEECH_COMMAND_ID6="huan yi ge yan se"
   CONFIG_CN_SPEECH_COMMAND_ID7="liang yi dian"
   CONFIG_CN_SPEECH_COMMAND_ID8="zeng da liang du"
   CONFIG_CN_SPEECH_COMMAND_ID9="an yi dian"
   CONFIG_CN_SPEECH_COMMAND_ID10="jian xiao liang du"

你也可以使用 menuconfig 添加自己的语音命令，方法可参见 `MultiNet 介绍 <https://github.com/espressif/esp-sr/blob/master/speech_command_recognition/README.md>`_。
在添加完语音后记得更改语音命令的数量，使之对应实际的数量



演示
---------------
 - 使用语音控制应先说出唤醒词 “Hi，乐鑫” 用于唤醒开发板，让 ESP32 运行命令词识别模型，此时呈现绿色呼吸灯状态
 - 唤醒后可说出 “打开点灯” “关闭点灯” “增大亮度” 等来控制灯的状态，支持的语音指令在上面已列出
 - 在唤醒后亮绿色呼吸灯的时间内为命令词识别状态，一段时间后未识别到有效指令将自动回到等待唤醒的状态

