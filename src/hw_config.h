#pragma once

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <hal/spi_types.h>

// ============================================================
// 官方 luxiaoban-xiaozhi-1.54tft-wifi 配置 (已验证)
// ============================================================

// ---------- I2S 音频 ----------
// 扬声器 (I2S 输出, MAX98357)
static constexpr int kPinSpkBclk = GPIO_NUM_15;
static constexpr int kPinSpkWs   = GPIO_NUM_16;
static constexpr int kPinSpkDout = GPIO_NUM_7;

// 麦克风 (I2S 输入, ICS-43434)
static constexpr int kPinMicSck  = GPIO_NUM_5;
static constexpr int kPinMicWs   = GPIO_NUM_4;
static constexpr int kPinMicDin  = GPIO_NUM_6;

// ---------- 显示 (ST7789, SPI3_HOST) ----------
static constexpr int kLcdMosi    = GPIO_NUM_10;
static constexpr int kLcdSclk    = GPIO_NUM_9;
static constexpr int kLcdCs      = GPIO_NUM_14;
static constexpr int kLcdDc      = GPIO_NUM_8;
static constexpr int kLcdRst     = GPIO_NUM_18;
static constexpr int kLcdBl      = GPIO_NUM_13;
static constexpr spi_host_device_t kLcdSpiHost = SPI3_HOST;

static constexpr int kLcdWidth   = 240;
static constexpr int kLcdHeight  = 240;
static constexpr int kLcdOffsetX = 0;
static constexpr int kLcdOffsetY = 0;
static constexpr int kLcdSwapXY  = 0;
static constexpr int kLcdMirrorX = 0;
static constexpr int kLcdMirrorY = 0;
static constexpr int kLcdInvert  = 1;

// ---------- 按钮 ----------
static constexpr int kPinBoot     = GPIO_NUM_0;   // BOOT键
static constexpr int kPinVolUp    = GPIO_NUM_39;  // 音量+
static constexpr int kPinVolDown  = GPIO_NUM_40;  // 音量-

// ---------- 电池 ----------
static constexpr int kPinBattAdc   = GPIO_NUM_8;   // ADC1_CH2
static constexpr int kPinCharging  = GPIO_NUM_9;    // 充电检测
static constexpr int kPinPowerHold = GPIO_NUM_2;    // 电源保持 RTC

// ---------- LED ----------
static constexpr int kPinBuiltinLed = GPIO_NUM_NC;  // 无内置 LED
