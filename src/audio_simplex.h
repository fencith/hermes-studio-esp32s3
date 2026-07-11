#pragma once

#include <stdint.h>
#include <stddef.h>

class AudioSimplex {
public:
    AudioSimplex();
    ~AudioSimplex();

    bool Init(int spk_bclk, int spk_ws, int spk_dout,
              int mic_sck, int mic_ws, int mic_din,
              int sample_rate);
    bool Deinit();

    int Write(const int16_t* data, int samples, uint32_t timeout_ms = 3000);
    int Read(int16_t* data, int samples, uint32_t timeout_ms = 3000);

    bool is_ready() const { return ready_; }

private:
    bool ready_;
    int sample_rate_;
    void* tx_handle_;  // i2s_chan_handle_t opaque
    void* rx_handle_;
};
