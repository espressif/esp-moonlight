驱动程序
=============

:link_to_translation:`en:[English]`

这个示例将展示我们如何使用 ESP32 驱动开发板上的外设，本章节只关注实现基本的驱动的功能，后续章节会具体介绍连网功能。如需查看相关代码，请前往 ``examples/2_drivers`` 目录。

该示例包含以下功能：

-  驱动 LED 灯，可改变灯的颜色、亮度等
-  一个实体按钮，按下按钮即可控制 LED 灯
-  监测电池电压，并且把电压值输出到串口监视器
-  接收震动传感器的触发，实现触碰可变色

驱动程序的代码已按照功能分开单独放入工程目录下的 ``components`` 文件夹，因此如果后续需要修改此应用程序用于产品，您只需更改此文件夹下对应的内容。


LED 灯
---------------

首先，我们需要使 LED 灯亮起来。开发板上有 6 颗三种颜色分别并联在一起的 RGB LED 灯，ESP32 输出的信号经过三个 MOS 管驱动这些 LED 灯。

硬件的接口定义如下：

+----------+---------------+-----------------+
| Pin Name | ESP32-S2-SOLO | ESP32-WROOM-32D |
+==========+===============+=================+
| RED      |   IO36        |        IO16     |
+----------+---------------+-----------------+
| GREEN    |   IO35        |        IO4      |
+----------+---------------+-----------------+
| BLUE     |   IO37        |        IO17     |
+----------+---------------+-----------------+

代码
~~~~~~~~
初始化配置 LED 驱动的代码如下：

.. code-block:: c

    /**< configure led driver */
    led_rgb_config_t rgb_config = {0};
    rgb_config.red_gpio_num   = BOARD_GPIO_LED_R;
    rgb_config.green_gpio_num = BOARD_GPIO_LED_G;
    rgb_config.blue_gpio_num  = BOARD_GPIO_LED_B;
    rgb_config.red_ledc_ch    = LEDC_CHANNEL_0;
    rgb_config.green_ledc_ch  = LEDC_CHANNEL_1;
    rgb_config.blue_ledc_ch   = LEDC_CHANNEL_2;
    rgb_config.speed_mode = LEDC_LOW_SPEED_MODE;
    rgb_config.timer_sel  = LEDC_TIMER_0;
    rgb_config.freq       = 20000;
    rgb_config.resolution = LEDC_TIMER_8_BIT;
    g_leds = led_rgb_create(&rgb_config);

    if (!g_leds) {
        ESP_LOGE(TAG, "install LED driver failed");
    }

`LEDC <https://docs.espressif.com/projects/esp-idf/zh_CN/release-v4.0/api-reference/peripherals/ledc.html>`_ 是 ESP32 中主要用于控制 LED 亮度和颜色的一个外设，也可以产生 PWM 信号用于其他用途。上面的程序中调用 :c:func:`led_rgb_create` 函数初始化了三个通道分别控制 RGB 三色灯，在参数中指定了 PWM 的频率是 20KHz ，分辨率为 8 位。

控制 LED 灯的函数如下：

.. code-block:: c

    /**< Write HSV values to LED */
    ESP_ERROR_CHECK(g_leds->set_hsv(g_leds, a, b, c));

    /**< Write RGB values to LED */
    ESP_ERROR_CHECK(g_leds->set_rgb(g_leds, a, b, c));

这里是分别以 HSV 参数和 RGB 参数两种方式控制 LED 灯

实体按钮
---------------

在 ESP-Moonlight 开发板上有一个按钮，并连接至 ESP32 的 GPIO0，我们将配置此按钮，用于切换灯光的模式。

代码
~~~~~~~~

实现此功能的代码如下：

.. code-block:: c

    static void button_press_cb(void *arg)
    {
        if (g_led_mode) {
            g_led_mode = 0;
        } else {
            g_led_mode = 1;
        }

        ESP_LOGI(TAG, "Set the light mode to %d", g_led_mode);
    }

    static void configure_push_button(int gpio_num, void (*btn_cb)(void *))
    {
        button_handle_t btn_handle = iot_button_create(gpio_num, 0);

        if (btn_handle) {
            iot_button_set_evt_cb(btn_handle, BUTTON_CB_TAP, button_press_cb, NULL);
        }
    }

我们使用 :c:func:`configure_push_button` 函数来配置按钮功能。首先，创建 button 对象，指定 GPIO 输出端及其有效电平用于检测按钮动作。
然后我们为按钮注册事件回调函数，松开按钮时，就会在 esp-timer 线程中调用 :c:func:`button_press_cb` 函数。请确保为 esp-timer 线程配置的默认堆栈足以满足回调函数需求。


震动传感器
---------------

在开发板上有一个圆柱形的震动传感器。震动开关在静止时为开路状态，当受到外力触碰而达到一定振动力时或移动速度达到一定离心力时，两个引脚将会瞬间导通，当外力消失后恢复到开路状态，传感器将震动转换为可以被 ESP32 检测到的高低电平信号。

和按键的特性类似，在一次震动中会存在很多的抖动信号，电路上传感器的两端并联了一个小电容来消除一些电平的抖动。ESP32 上使用 IO 中断来检测震动传感器的电平变化，软件上通过在检测到第一次电平变化后延时一段时间来避开连续的抖动。

代码
~~~~~~~~
实现此功能的代码如下：

.. code-block:: c

    sensor_vibration_init(BOARD_GPIO_SENSOR_INT);
    sensor_vibration_triggered_register(vibration_handle, NULL);

首先调用 :c:func:`sensor_vibration_init` 进行初始化，指定了震动传感器的 IO 口，其后注册了一个传感器触发的回调函数，当震动传感器输出的电平发生变化即会进行回调。

回调函数代码如下：

.. code-block:: c

    static void vibration_handle(void *arg)
    {
        uint16_t h;
        uint8_t s;

        if (!g_led_mode) {
            return;
        }

        /**< Set a random color */
        h = esp_random() / 11930465;
        s = esp_random() / 42949673;
        s = s < 40 ? 40 : s;

        ESP_ERROR_CHECK(g_leds->set_hsv(g_leds, h, s, 100));
    }


电池电压监测
---------------

ESP32 集成有两个 12 位的逐次逼近型 `ADC <https://docs.espressif.com/projects/esp-idf/zh_CN/release-v4.0/api-reference/peripherals/adc.html>`_ ，一共支持 18 个模拟通道输入。电池的电压经过电阻 1/2 分压后输入到 ESP32 ADC 的一个通道。
ADC 内部的参考电压为 1100mv ，内部还有一个可调的衰减系数，增大了 ADC 的输入范围。

代码
~~~~~~~~
实现此功能的代码如下：

.. code-block:: c

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

调用 :c:func:`adc1_config_width` 配置 ADC 的分辨率为 12 位，然后使用 :c:func:`adc1_config_channel_atten` 设置内部衰减为 11 DB，也就是大约原来的 1/3.6 ，再加上外部电阻的衰减才能保证电压动态范围不超过 ADC 的量程，
最后通过 :c:func:`xTaskCreatePinnedToCore` 在 CPU1 上创建了一个用于电池监测的任务。

演示
---------
将此固件编译并烧录至设备后，LED 灯的颜色从红色渐变到绿色再到蓝色，以此不断循环。每次按下按钮，ESP32 就会在灯光受震动传感器控制和自动渐变之间来回切换。
同时，在串口 monitor 中还会不断打印出当前 ADC 测量得到的 电池电压值。

未完待续
---------------

现在，我们已经实现了一个小夜灯本身的驱动功能，当然，目前该设备还无法连网。
下一步，我们将增加 Wi-Fi 连接功能。
