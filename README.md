# Hermes Studio ESP32-S3 Firmware

| 项目 | 说明 |
|------|------|
| 芯片 | ESP32-S3 (8MB PSRAM, 16MB Flash) |
| 面板 | 正辰 zhengchen-1.54tft-wifi |
| 显示 | ST7789 SPI 240x240 |
| 音频 | I2S Simplex (MAX98357 扬声器 + INMP441 麦克风) |
| 连接 | Wi-Fi → UDP 发现 → Socket.IO → Hermes /global-agent |

## 管脚映射

| 功能 | GPIO |
|------|------|
| 扬声器 BCLK | 15 |
| 扬声器 WS | 16 |
| 扬声器 DOUT | 7 |
| 麦克风 SCK | 5 |
| 麦克风 WS | 4 |
| 麦克风 DIN | 6 |
| LCD MOSI | 41 |
| LCD SCLK | 42 |
| LCD CS | 21 |
| LCD DC | 40 |
| LCD RST | 45 |
| LCD BL | 20 |
| BOOT 按钮 | 0 |
| VOL+ | 10 |
| VOL- | 39 |
| 电池 ADC | 8 |
| 充电检测 | 9 |

## 适配说明

此固件基于 [EKKOLearnAI/hermes-studio](https://github.com/EKKOLearnAI/hermes-studio) 的 ESP32-C3 v1 源码适配，主要改动：

- 管脚全部重映射为正辰板配置（来自 xiaozhi-esp32 官方 boards/zhengchen-1.54tft-wifi/config.h）
- 显示从 SSD1306 OLED (I2C) 改为 ST7789 LCD (SPI 240x240)
- 音频从 ES8311 编解码器 (单 I2S 端口) 改为双端口 I2S Simplex (TX/RX 独立端口)
- 新增 VOL+/VOL- 按钮支持
- 移除 ANO 8311 I2C 编解码器依赖

## 编译

```bash
cd packages/esp32-s3/v1
pio run
```

## 烧录

WebSerial: https://espressif.github.io/esptool-js/
地址: 0x0
文件: release/v1/hermes-s3-v202.bin
