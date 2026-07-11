#pragma once

#include <stdint.h>
#include <stddef.h>

class AudioSimplex {
public:
    AudioSimplex();
    ~AudioSimplex();

    // I2S 双端口初始化: 扬声器 I2S_NUM_0 + 麦克风 I2S_NUM_1
    bool Init(int spk_bclk, int spk_ws, int spk_dout,
              int mic_sck, int mic_ws, int mic_din,
              int sample_rate);
    bool Deinit();

    int Write(const int16_t* data, int samples, uint32_t timeout_ms = 3000);
    int Read(int16_t* data, int samples, uint32_t timeout_ms = 3000);

    bool is_ready() const { return ready_; }
    void SetOutputVolume(int volume) { output_volume_ = volume; }
    int output_volume() const { return output_volume_; }

private:
    bool ready_;
    int sample_rate_;
    int output_volume_;
};
