# ESP-Moonlight 支持政策

自 2023 年 11 月起，我们仅提供有限的支持，但是 Pull Request 仍然是被欢迎的！

## 支持的版本和芯片
-----------------

| ESP-Moonlight | Dependent ESP-IDF |    Target     |  Support State  |
| ------------- | ----------------- | ------------- | --------------- |
| master        | >=release/v4.4    | ESP32 ESP32S3 | Limited Support |

# ESP-MoonLight

[![Documentation Status](https://readthedocs.com/projects/espressif-esp-moonlight/badge/?version=latest)](https://docs.espressif.com/projects/espressif-esp-moonlight/zh_CN/latest/)

中文文档: [https://docs.espressif.com/projects/espressif-esp-moonlight/zh_CN/latest/introduction.html](https://docs.espressif.com/projects/espressif-esp-moonlight/zh_CN/latest/introduction.html)

| <img src="docs/_static/espressif-logo.svg" alt="espressif-logo" width="300" /> |
| ------------------------------------------------------------ |
| <img src="docs/_static/cover_page_pdf.jpg" alt="cover_page_pdf" width="300" /> |

ESP-Moonlight 是乐鑫推出的基于 ESP32 开发的月球灯示例项目，内含使用 ESP-IDF 开发的完整步骤。

## 概述

本项目以快速开发为特点，硬件结构简单，代码架构清晰完善，方便功能扩展，可实现驱动 LED 灯、局域网内使用小程序端控制灯的亮度和色彩等参数，可用于教育领域。

<table>
    <tr>
        <td ><img src="docs/_static/moonlight2.jpg" alt="moonlight" width=450 /></td>
        <td ><img src="docs/_static/moonlight_cover.jpg" alt="moonlight" width=450 /></td>
    </tr>
</table>


## 硬件

本项目使用的开发板为 ESP32-Moonlight ，集成了以下组件：

<table>
    <tr>
        <td ><div align=center><img src="docs/_static/ESP-Moonlight_front.png" alt="moonlight" width=460 /></div></td>
        <td ><div align=center><img src="docs/_static/ESP-Moonlight_back.png" alt="moonlight" width=460 /></div></td>
    </tr>
</table>

- [ESP32-Moonlight 原理图 (PDF)](hardware/ESP-Moonlight_V2.0_N_XX_20200421_V0.3/01_Schematic/SCH_ESP-MOONLIGHT_V2_0_20200421A.pdf)

- 月球灯开发板具有以下功能：
  - 板载自动下载电路
  - 电池充电
  - 支持手机进行配网
  - 支持手机控制
  - 支持触碰控制
  - 支持 OTA 升级
  - 支持语音控制


## 视频教程

- [ESP-IDF 环境搭建 Windows](https://www.bilibili.com/video/BV1Ke411s7Go)

- [ESP-IDF 环境搭建 Mac OS](https://www.bilibili.com/video/BV17K4y1k7Ht)

- [ESP32 驱动 LED RGB5050 及震动控制](https://www.bilibili.com/video/BV1JK411s7ZA)

- [ESP32 驱动 LED WS2812](https://www.bilibili.com/video/BV1jC4y1W7CZ)

- [ESP32 进行 Wi-Fi 连接、配网及手机控制 LED 灯](https://www.bilibili.com/video/BV1nQ4y1N7ZC)

- [ESP32 OTA(Over The Air) 升级介绍](https://www.bilibili.com/video/BV155411Y7VJ)

