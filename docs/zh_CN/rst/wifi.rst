Wi-Fi 连接
================

:link_to_translation:`en:[English]`

ESP32 芯片具有 WiFi 和蓝牙的功能，本节我们将使用 ESP32 连接上一个指定的 WiFi 网络，此 Wi-Fi 网络信息已嵌入到设备固件中。如需查看相关代码，请前往 ``examples/3_wifi_connection`` 目录。


WiFi 模式
-----------

ESP32 的 WiFi 具有以下模式：

- 基站模式（即 STA 模式或 Wi-Fi 客户端模式），此时 ESP32 连接到接入点 (AP)

- AP 模式（即 Soft-AP 模式或接入点模式），此时基站连接到 ESP32

- AP-STA 共存模式（ESP32 既是接入点，同时又作为基站连接到另外一个接入点）

- 使用混杂模式监控 IEEE802.11 Wi-Fi 数据包

在本示例中将设置为 STA 模式去连接一个接入点，并且在程序中嵌入 SSID 和 PASSWORD，让 ESP32 一上电就直接去进行连接，并且设置了一个最大重连次数，超过这个次数认为本次连接失败。

.. note::

    ESP32 芯片只支持 2.4G 的WiFi，不能连接5G WiFi。


代码
--------
在 ``app_main.c`` 添加了几个函数，下面是把 WiFi 初始化为 sta 模式的代码：

.. code-block:: c
   :linenos:

    esp_err_t wifi_init_sta(void)
    {
        esp_err_t ret = ESP_OK;
        s_wifi_event_group = xEventGroupCreate();

        tcpip_adapter_init();

        ESP_ERROR_CHECK(esp_event_loop_create_default());

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

        wifi_config_t wifi_config = {
            .sta = {
                .ssid = EXAMPLE_ESP_WIFI_SSID,
                .password = EXAMPLE_ESP_WIFI_PASS
            },
        };
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "wifi_init_sta finished.");

        /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
        * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE,
                                            pdFALSE,
                                            portMAX_DELAY);

        /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
        * happened. */
        if (bits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                    EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        } else if (bits & WIFI_FAIL_BIT) {
            ret = ESP_FAIL;
            ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                    EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        } else {
            ESP_LOGE(TAG, "UNEXPECTED EVENT");
        }

        ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
        ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
        vEventGroupDelete(s_wifi_event_group);
        return ret;
    }

- 调用 :c:func:`xEventGroupCreate` 来创建一个事件标志组，它是操作系统提供的功能， 使用参见 `Event Group API <https://docs.espressif.com/projects/esp-idf/zh_CN/v4.0/api-reference/system/freertos.html#event-group-api>`_ 
- 调用 :c:func:`esp_event_handler_register` 向 WiFi 组件注册一些事件通知
- 调用 :c:func:`tcpip_adapter_init`  来初始化 TCP/IP 堆栈
- 通过 :c:type:`WIFI_INIT_CONFIG_DEFAULT` 读取一个 WiFi 的默认配置
- 调用 :c:func:`esp_wifi_init` 、:c:func:`esp_wifi_set_config`  和 :c:func:`esp_wifi_set_mode` 来初始化 Wi-Fi 子系统及其 station 接口。将连接的 WiFi 名称和密码分别是 :c:type:`EXAMPLE_ESP_WIFI_SSID` 和 :c:type:`EXAMPLE_ESP_WIFI_PASS` 
- 30~52 行代码是无限等待事件标志组被置位，当 :c:type:`WIFI_CONNECTED_BIT` 或 :c:type:`WIFI_FAIL_BIT` 被置位后，打印出信息并删除一些变量和注销事件通知

`Event loop <https://docs.espressif.com/projects/esp-idf/zh_CN/v4.0/api-reference/system/esp_event.html>`_ 是 idf 中一个组件之间事件通知的库，这允许低耦合组件在不涉及应用程序的情况下将所需的动作行为附加到其他组件的状态更改上。
下面就是我们前面注册到 WIFI 组件的回调函数:

.. code-block:: c
   :linenos:

    static void event_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
    {
        static int32_t s_retry_num = 0;

        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "retry to connect to the AP");
            } else {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }

            ESP_LOGI(TAG, "connect to the AP fail");
        } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }

- 当调用 :c:func:`esp_wifi_start` 后会产生一个 :c:type:`WIFI_EVENT_STA_START` 事件，随即调用 :c:func:`esp_wifi_connect` 函数开始连接过程
- 当连接上后，会产生 :c:type:`IP_EVENT_STA_GOT_IP` 事件，这时读取 event_data 来获得 ip 地址，并调用 :c:func:`xEventGroupSetBits` 来设置事件标志组的 :c:type:`WIFI_CONNECTED_BIT` 位。
- 当 WiFi 断开连接后将产生 :c:type:`WIFI_EVENT_STA_DISCONNECTED` 事件，这时将执行 9~15 行的代码进行重连

主程序代码：

.. code-block:: c
   :linenos:

    void app_main(void)
    {
        uint32_t hue = 0;
        /**< Initialize NVS */
        esp_err_t ret = nvs_flash_init();

        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }

        ESP_ERROR_CHECK(ret);
        /**< install ws2812 driver */
        led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(BOARD_GPIO_WS2812_DIN, BOARD_STRIP_LED_NUMBER, (led_strip_dev_t)RMT_CHANNEL_0);
        g_strip = led_strip_new_rmt_ws2812(&strip_config);

        if (!g_strip) {
            ESP_LOGE(TAG, "install WS2812 driver failed");
        }

        xTaskCreate(breath_light_task, "breath_light_task", 1024 * 3, NULL, 5, &g_breath_light_task_handle);
        ESP_LOGI(TAG, "Wait for connect");

        /**< Start the station */
        ret = wifi_init_sta();
        vTaskDelete(g_breath_light_task_handle);

        if (ESP_OK != ret) {
            /**< Set leds to red to indicate failure */
            ESP_ERROR_CHECK(g_strip->set_all_rgb(g_strip, 60, 0, 0));
            ESP_ERROR_CHECK(g_strip->refresh(g_strip, 10));
            ESP_LOGW(TAG, "Connect failed");
            return;
        }

        ESP_LOGI(TAG, "Color fade start");

        while (true) {

            /**< Write HSV values to strip driver */
            ESP_ERROR_CHECK(g_strip->set_all_hsv(g_strip, hue, 100, 100));
            /**< Flush to LEDs */
            ESP_ERROR_CHECK(g_strip->refresh(g_strip, 10));
            vTaskDelay(pdMS_TO_TICKS(30));
            hue++;

            if (hue > 360) {/**< The maximum value of hue in HSV color space is 360 */
                hue = 0;
            }
        }
    }


- 5~10 行初始化 NVS(Non-volatile storage) ，WiFi 需要保存一些参数到 NVS 中
- 第 21 行调用 :c:func:`xTaskCreate` 创建了一个任务来控制 LED 灯实现呼吸效果，表示正在进行连接
- :c:func:`wifi_init_sta`  连接 WiFi ，连接结束了函数才会返回

.. note::

    在编译例程之前，需要配置连接的 WiFi 的信息：输入 ``idf.py menuconfig`` 进入menuconfig ，按照提示在 Example Configuration 设置项中输入信息

使用演示
----------

- 上电进行 WiFi 连接，LED 亮黄色呼吸灯，这表示 ESP32 还在连接中
- 一段时间后，如果连接成功 LED 将会高亮并颜色渐变；如果失败了则保持低亮度并变成红色。
- 程序结束

未完待续
---------------

这种连接的方法对业余开发项目而言没有问题，但是对于实际的使用场景，则希望自定义配置设备。这就是我们下一章要讨论的问题。
