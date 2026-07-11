#pragma once

#include <driver/gpio.h>

// ============================================================
// 正辰 zhengchen-1.54tft-wifi (ESP32-S3) 硬件管脚定义
// ============================================================

// ---------- I2S 音频 ----------
// 扬声器 (I2S 输出, 如 MAX98357)
static constexpr int kPinSpkBclk = GPIO_NUM_15;
static constexpr int kPinSpkWs   = GPIO_NUM_16;
static constexpr int kPinSpkDout = GPIO_NUM_7;

// 麦克风 (I2S 输入, 如 INMP441 / MSM261)
static constexpr int kPinMicSck  = GPIO_NUM_5;
static constexpr int kPinMicWs   = GPIO_NUM_4;
static constexpr int kPinMicDin  = GPIO_NUM_6;

// 功放使能 (PA_EN) — 无独立引脚, 由 I2S 供电
static constexpr int kPinPaEn    = GPIO_NUM_NC;

// ---------- 显示 (ST7789, SPI) ----------
static constexpr int kLcdMosi    = GPIO_NUM_41;
static constexpr int kLcdSclk    = GPIO_NUM_42;
static constexpr int kLcdCs      = GPIO_NUM_21;
static constexpr int kLcdDc      = GPIO_NUM_40;
static constexpr int kLcdRst     = GPIO_NUM_45;
static constexpr int kLcdBl      = GPIO_NUM_20;
static constexpr int kLcdSpiHost = 3; // SPI3_HOST

static constexpr int kLcdWidth   = 240;
static constexpr int kLcdHeight  = 240;
static constexpr int kLcdOffsetX = 0;
static constexpr int kLcdOffsetY = 0;

// ---------- 按钮 ----------
static constexpr int kPinBoot     = GPIO_NUM_0;
static constexpr int kPinVolUp    = GPIO_NUM_10;
static constexpr int kPinVolDown  = GPIO_NUM_39;

// ---------- 电池 ----------
static constexpr int kPinBattAdc   = GPIO_NUM_8;   // ADC1_CH2
static constexpr int kPinCharging  = GPIO_NUM_9;    // 充电检测
static constexpr int kPinPowerHold = GPIO_NUM_2;    // 电源保持 RTC

// ---------- LED ----------
static constexpr int kPinBuiltinLed = GPIO_NUM_NC;   // 无内置 LED
