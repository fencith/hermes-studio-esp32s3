#include "audio_simplex.h"

#include <Arduino.h>
#include "driver/i2s.h"
#include <algorithm>
#include <cstring>

#define TAG "AudioSimplex"

static const int kDmaBufCount = 8;
static const int kDmaBufLen = 256;

AudioSimplex::AudioSimplex() : ready_(false), sample_rate_(0), output_volume_(70) {}

AudioSimplex::~AudioSimplex() {
    Deinit();
}

bool AudioSimplex::Init(int spk_bclk, int spk_ws, int spk_dout,
                        int mic_sck, int mic_ws, int mic_din,
                        int sample_rate) {
    if (ready_) Deinit();
    sample_rate_ = sample_rate;
    esp_err_t err;

    // --- 扬声器 I2S (TX, I2S_NUM_0) ---
    i2s_driver_uninstall(I2S_NUM_0);
    i2s_config_t tx_config = {};
    tx_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    tx_config.sample_rate = sample_rate;
    tx_config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
    tx_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    tx_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    tx_config.intr_alloc_flags = 0;
    tx_config.dma_buf_count = kDmaBufCount;
    tx_config.dma_buf_len = kDmaBufLen;
    tx_config.use_apll = false;
    tx_config.tx_desc_auto_clear = true;
    tx_config.fixed_mclk = 0;

    err = i2s_driver_install(I2S_NUM_0, &tx_config, 0, nullptr);
    if (err != ESP_OK) {
        Serial.printf("I2S TX install failed: %d\n", err);
        return false;
    }

    i2s_pin_config_t tx_pins = {};
    tx_pins.mck_io_num = I2S_PIN_NO_CHANGE;
    tx_pins.bck_io_num = spk_bclk;
    tx_pins.ws_io_num = spk_ws;
    tx_pins.data_out_num = spk_dout;
    tx_pins.data_in_num = I2S_PIN_NO_CHANGE;
    err = i2s_set_pin(I2S_NUM_0, &tx_pins);
    if (err != ESP_OK) {
        Serial.printf("I2S TX pin setup failed: %d\n", err);
        i2s_driver_uninstall(I2S_NUM_0);
        return false;
    }

    i2s_zero_dma_buffer(I2S_NUM_0);
    Serial.printf("I2S TX ready: BCLK=%d WS=%d DOUT=%d\n", spk_bclk, spk_ws, spk_dout);

    // --- 麦克风 I2S (RX, I2S_NUM_1) - 标准 I2S 模式 ---
    // 注意: 官方 luxiaoban 使用 PDM 模式，但这里用标准 I2S 兼容性更好
    // 麦克风脚位: SCK=5 (BCLK), WS=4, DIN=6
    i2s_driver_uninstall(I2S_NUM_1);
    i2s_config_t rx_config = {};
    rx_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    rx_config.sample_rate = sample_rate;
    rx_config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
    rx_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    rx_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    rx_config.intr_alloc_flags = 0;
    rx_config.dma_buf_count = kDmaBufCount;
    rx_config.dma_buf_len = kDmaBufLen;
    rx_config.use_apll = false;
    // rx_desc_auto_clear not valid for RX
    rx_config.fixed_mclk = 0;

    err = i2s_driver_install(I2S_NUM_1, &rx_config, 0, nullptr);
    if (err != ESP_OK) {
        Serial.printf("I2S RX install failed: %d\n", err);
        i2s_driver_uninstall(I2S_NUM_0);
        return false;
    }

    i2s_pin_config_t rx_pins = {};
    rx_pins.mck_io_num = I2S_PIN_NO_CHANGE;
    rx_pins.bck_io_num = mic_sck;   // BCLK = SCK
    rx_pins.ws_io_num = mic_ws;      // WS
    rx_pins.data_out_num = I2S_PIN_NO_CHANGE;
    rx_pins.data_in_num = mic_din;   // DATA = DIN
    err = i2s_set_pin(I2S_NUM_1, &rx_pins);
    if (err != ESP_OK) {
        Serial.printf("I2S RX pin setup failed: %d\n", err);
        i2s_driver_uninstall(I2S_NUM_0);
        i2s_driver_uninstall(I2S_NUM_1);
        return false;
    }

    i2s_zero_dma_buffer(I2S_NUM_1);
    Serial.printf("I2S RX ready: BCLK=%d WS=%d DIN=%d\n", mic_sck, mic_ws, mic_din);

    ready_ = true;
    Serial.printf("AudioSimplex ready: %d Hz\n", sample_rate);
    return true;
}

bool AudioSimplex::Deinit() {
    ready_ = false;
    i2s_driver_uninstall(I2S_NUM_0);
    i2s_driver_uninstall(I2S_NUM_1);
    return true;
}

int AudioSimplex::Write(const int16_t* data, int samples, uint32_t timeout_ms) {
    if (!ready_ || samples <= 0 || !data) return 0;

    size_t total_bytes = samples * sizeof(int32_t);
    int32_t* buffer = new (std::nothrow) int32_t[samples];
    if (!buffer) return 0;

    for (int i = 0; i < samples; i++) {
        int32_t val = data[i];
        if (val > 32767) val = 32767;
        if (val < -32768) val = -32768;
        buffer[i] = val << 12;  // 16-bit -> 32-bit left aligned
    }

    size_t written = 0;
    esp_err_t err = i2s_write(I2S_NUM_0, buffer, total_bytes, &written, pdMS_TO_TICKS(timeout_ms));
    delete[] buffer;

    if (err != ESP_OK) return 0;
    return written / sizeof(int32_t);
}

int AudioSimplex::Read(int16_t* data, int samples, uint32_t timeout_ms) {
    if (!ready_ || samples <= 0 || !data) return 0;

    size_t total_bytes = samples * sizeof(int32_t);
    int32_t* buffer = new (std::nothrow) int32_t[samples];
    if (!buffer) return 0;

    size_t bytes_read = 0;
    esp_err_t err = i2s_read(I2S_NUM_1, buffer, total_bytes, &bytes_read, pdMS_TO_TICKS(timeout_ms));
    if (err != ESP_OK) {
        delete[] buffer;
        return 0;
    }

    int samples_read = bytes_read / sizeof(int32_t);
    for (int i = 0; i < samples_read; i++) {
        int32_t value = buffer[i] >> 12;  // 32-bit -> 16-bit
        if (value > INT16_MAX) value = INT16_MAX;
        if (value < -INT16_MAX) value = -INT16_MAX;
        data[i] = static_cast<int16_t>(value);
    }

    delete[] buffer;
    return samples_read;
}
