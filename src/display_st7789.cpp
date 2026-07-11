#include "display_st7789.h"
#include <Arduino.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <cstring>
#include <algorithm>

#define TAG "ST7789"

static const uint8_t kFont5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // space
    {0x00,0x00,0x5F,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00}, // "
    {0x14,0x7F,0x14,0x7F,0x14}, // #
    {0x24,0x2A,0x7F,0x2A,0x12}, // $
    {0x23,0x13,0x08,0x64,0x62}, // %
    {0x36,0x49,0x55,0x22,0x50}, // &
    {0x00,0x05,0x03,0x00,0x00}, // '
    {0x00,0x1C,0x22,0x41,0x00}, // (
    {0x00,0x41,0x22,0x1C,0x00}, // )
    {0x08,0x2A,0x1C,0x2A,0x08}, // *
    {0x08,0x08,0x3E,0x08,0x08}, // +
    {0x00,0x50,0x30,0x00,0x00}, // ,
    {0x08,0x08,0x08,0x08,0x08}, // -
    {0x00,0x60,0x60,0x00,0x00}, // .
    {0x20,0x10,0x08,0x04,0x02}, // /
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9
    {0x00,0x36,0x36,0x00,0x00}, // :
    {0x00,0x56,0x36,0x00,0x00}, // ;
    {0x00,0x08,0x14,0x22,0x41}, // <
    {0x14,0x14,0x14,0x14,0x14}, // =
    {0x41,0x22,0x14,0x08,0x00}, // >
    {0x02,0x01,0x51,0x09,0x06}, // ?
    {0x32,0x49,0x79,0x41,0x3E}, // @
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x04,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x7F,0x40,0x30,0x40,0x7F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x07,0x08,0x70,0x08,0x07}, // Y
    {0x61,0x51,0x49,0x45,0x43}, // Z
    {0x00,0x00,0x7F,0x41,0x00}, // [
    {0x02,0x04,0x08,0x10,0x20}, // backslash
    {0x00,0x41,0x7F,0x00,0x00}, // ]
    {0x04,0x02,0x01,0x02,0x04}, // ^
    {0x40,0x40,0x40,0x40,0x40}, // _
    {0x00,0x01,0x02,0x04,0x00}, // `
    {0x20,0x54,0x54,0x54,0x78}, // a
    {0x7F,0x48,0x44,0x44,0x38}, // b
    {0x38,0x44,0x44,0x44,0x20}, // c
    {0x38,0x44,0x44,0x48,0x7F}, // d
    {0x38,0x54,0x54,0x54,0x18}, // e
    {0x08,0x7E,0x09,0x01,0x02}, // f
    {0x08,0x14,0x54,0x54,0x3C}, // g
    {0x7F,0x08,0x04,0x04,0x78}, // h
    {0x00,0x44,0x7D,0x40,0x00}, // i
    {0x20,0x40,0x44,0x3D,0x00}, // j
    {0x00,0x7F,0x10,0x28,0x44}, // k
    {0x00,0x41,0x7F,0x40,0x00}, // l
    {0x7C,0x04,0x18,0x04,0x78}, // m
    {0x7C,0x08,0x04,0x04,0x78}, // n
    {0x38,0x44,0x44,0x44,0x38}, // o
    {0x7C,0x14,0x14,0x14,0x08}, // p
    {0x08,0x14,0x14,0x18,0x7C}, // q
    {0x7C,0x08,0x04,0x04,0x08}, // r
    {0x48,0x54,0x54,0x54,0x20}, // s
    {0x04,0x3F,0x44,0x40,0x20}, // t
    {0x3C,0x40,0x40,0x20,0x7C}, // u
    {0x1C,0x20,0x40,0x20,0x1C}, // v
    {0x3C,0x40,0x30,0x40,0x3C}, // w
    {0x44,0x28,0x10,0x28,0x44}, // x
    {0x0C,0x50,0x50,0x50,0x3C}, // y
    {0x44,0x64,0x54,0x4C,0x44}, // z
    {0x00,0x00,0x7F,0x41,0x00}, // {
    {0x02,0x04,0x08,0x10,0x20}, // |
    {0x00,0x41,0x7F,0x00,0x00}, // }
    {0x04,0x02,0x01,0x02,0x04}, // ~
    {0x40,0x40,0x40,0x40,0x40}  // DEL
};



St7789Display::St7789Display(int mosi, int sclk, int cs, int dc, int rst, int bl,
                             int width, int height, int offset_x, int offset_y)
    : mosi_(mosi), sclk_(sclk), cs_(cs), dc_(dc), rst_(rst), bl_(bl),
      width_(width), height_(height), offset_x_(offset_x), offset_y_(offset_y),
      panel_io_(nullptr), panel_(nullptr), framebuffer_(nullptr),
      fb_pixels_(0), scroll_phase_(0) {}

St7789Display::~St7789Display() {
    delete[] framebuffer_;
    if (panel_) esp_lcd_panel_del(panel_);
    if (panel_io_) esp_lcd_panel_io_del(panel_io_);
}

bool St7789Display::Init() {
    ESP_LOGI(TAG, "Initializing ST7789 LCD via ESP-IDF SPI...");
    Serial.printf("ST7789: Init start, pins: MOSI=%d SCLK=%d CS=%d DC=%d RST=%d BL=%d\n", mosi_, sclk_, cs_, dc_, rst_, bl_);

    fb_pixels_ = static_cast<size_t>(width_) * height_;
    framebuffer_ = new (std::nothrow) uint16_t[fb_pixels_]();
    if (!framebuffer_) {
        ESP_LOGE(TAG, "Framebuffer alloc failed (%zu bytes)", fb_pixels_ * 2);
        return false;
    }

    // Setup control pins
    pinMode(rst_, OUTPUT);
    pinMode(dc_, OUTPUT);
    pinMode(cs_, OUTPUT);
    pinMode(bl_, OUTPUT);
    digitalWrite(cs_, HIGH);
    digitalWrite(dc_, HIGH);
    digitalWrite(bl_, LOW);

    // Hardware reset
    digitalWrite(rst_, HIGH);
    delay(5);
    digitalWrite(rst_, LOW);
    delay(10);
    digitalWrite(rst_, HIGH);
    delay(150);
    Serial.println("ST7789: Hardware reset done");

    // Init SPI bus (using ESP-IDF API, matching xiaozhi-esp32)
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = mosi_;
    buscfg.miso_io_num = GPIO_NUM_NC;
    buscfg.sclk_io_num = sclk_;
    buscfg.quadwp_io_num = GPIO_NUM_NC;
    buscfg.quadhd_io_num = GPIO_NUM_NC;
    buscfg.max_transfer_sz = fb_pixels_ * 2;

        esp_err_t ret = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %d", ret);
        return false;
    }
    ESP_LOGI(TAG, "SPI bus initialized on SPI3 (MOSI=%d, SCLK=%d)", mosi_, sclk_);

    // Backlight off during init
    digitalWrite(bl_, LOW);

    // Panel IO config
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = cs_;
    io_config.dc_gpio_num = dc_;
    io_config.spi_mode = 3;
    io_config.pclk_hz = 80 * 1000 * 1000;  // 80MHz
    io_config.trans_queue_depth = 10;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    ESP_LOGI(TAG, "Creating panel IO...");
    ret = esp_lcd_new_panel_io_spi((void*)SPI3_HOST, &io_config, &panel_io_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel IO creation failed: %d", ret);
        Serial.printf("ST7789: Panel IO creation failed: %d\n", ret);
        return false;
    }
    Serial.println("ST7789: Panel IO created");

    // Panel config - ST7789
    ESP_LOGI(TAG, "Creating ST7789 panel...");
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = rst_;
    panel_config.color_space = ESP_LCD_COLOR_SPACE_RGB;
    panel_config.bits_per_pixel = 16;

    ret = esp_lcd_new_panel_st7789(panel_io_, &panel_config, &panel_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel creation failed: %d", ret);
        Serial.printf("ST7789: Panel creation failed: %d\n", ret);
        return false;
    }
    Serial.println("ST7789: Panel created");

    ESP_LOGI(TAG, "Initializing panel...");
    ret = esp_lcd_panel_reset(panel_);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Panel reset failed: %d", ret); return false; }
    Serial.println("ST7789: Panel reset OK");

    ret = esp_lcd_panel_init(panel_);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Panel init failed: %d", ret); return false; }
    Serial.println("ST7789: Panel init OK");

    ret = esp_lcd_panel_swap_xy(panel_, false);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Swap XY failed: %d", ret); return false; }
    ret = esp_lcd_panel_mirror(panel_, false, false);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Mirror failed: %d", ret); return false; }
    ret = esp_lcd_panel_invert_color(panel_, true);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Invert color failed: %d", ret); return false; }
    ret = esp_lcd_panel_disp_on_off(panel_, true);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Display on failed: %d", ret); return false; }
    Serial.println("ST7789: Display ON");

    ESP_LOGI(TAG, "Panel initialized");

    // Backlight PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ledc_channel_config_t ledc_channel = {
        .gpio_num = bl_,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 255,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    Serial.println("ST7789: Backlight ON");

    // Clear and test
    Clear(0x0000);
    ESP_LOGI(TAG, "ST7789 initialized: %dx%d", width_, height_);
    Serial.println("ST7789: Init complete");
    return true;
}

void St7789Display::SetBrightness(uint8_t percent) {
    uint32_t duty = (static_cast<uint32_t>(percent) * 255) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

void St7789Display::Clear(uint16_t color) {
    for (size_t i = 0; i < fb_pixels_; i++) framebuffer_[i] = color;
}

void St7789Display::SetPixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    framebuffer_[y * width_ + x] = color;
}

void St7789Display::FillRect(int x, int y, int w, int h, uint16_t color) {
    int x0 = std::max(0, x), y0 = std::max(0, y);
    int x1 = std::min(width_, x + w), y1 = std::min(height_, y + h);
    for (int yy = y0; yy < y1; yy++) {
        for (int xx = x0; xx < x1; xx++) {
            framebuffer_[yy * width_ + xx] = color;
        }
    }
}

void St7789Display::Flush() {
    if (!panel_) return;
    esp_lcd_panel_draw_bitmap(panel_, 0, 0, width_, height_,
                              reinterpret_cast<uint8_t*>(framebuffer_));
}

void St7789Display::DrawChar(int x, int y, char c, uint16_t fg, uint16_t bg, uint8_t scale) {
    if (c < ' ' || c >= 127) return;
    c -= ' ';
    const uint8_t* glyph = kFont5x7[(uint8_t)c];
    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 7; row++) {
            uint16_t color = (glyph[col] & (1 << row)) ? fg : bg;
            for (int sx = 0; sx < scale; sx++)
                for (int sy = 0; sy < scale; sy++)
                    SetPixel(x + col * scale + sx, y + row * scale + sy, color);
        }
    }
}

void St7789Display::DrawText(int x, int y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale) {
    for (int i = 0; text[i]; i++) {
        DrawChar(x + i * 6 * scale, y, text[i], fg, bg, scale);
    }
}

int St7789Display::TextWidth(const char* text, uint8_t scale) {
    int len = strlen(text);
    return len == 0 ? 0 : len * 6 * scale - scale;
}

void St7789Display::DrawCenteredText(int y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale) {
    int tw = TextWidth(text, scale);
    int x = (width_ - tw) / 2;
    if (x < 0) x = 0;
    DrawText(x, y, text, fg, bg, scale);
}

void St7789Display::DrawScrollingText(int y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale) {
    int tw = TextWidth(text, scale);
    if (tw <= width_) {
        DrawCenteredText(y, text, fg, bg, scale);
        return;
    }
    int gap = 18 * scale;
    int cycle = tw + gap;
    scroll_phase_ = (scroll_phase_ + 1) % cycle;
    int offset = -static_cast<int>(scroll_phase_ / 2);
    DrawText(offset, y, text, fg, bg, scale);
    if (offset + tw < width_) {
        DrawText(tw + gap + offset, y, text, fg, bg, scale);
    }
}

void St7789Display::DrawHLine(int x, int y, int w, uint16_t color) {
    FillRect(x, y, w, 1, color);
}

void St7789Display::DrawVLine(int x, int y, int h, uint16_t color) {
    FillRect(x, y, 1, h, color);
}

void St7789Display::DrawRect(int x, int y, int w, int h, uint16_t color) {
    DrawHLine(x, y, w, color);
    DrawHLine(x, y + h - 1, w, color);
    DrawVLine(x, y, h, color);
    DrawVLine(x + w - 1, y, h, color);
}

void St7789Display::DrawRoundRect(int x, int y, int w, int h, int r, uint16_t color, bool fill) {
    r = std::max(0, std::min(r, std::min(w, h) / 2));
    if (fill) {
        FillRect(x, y, w, h, color);
    } else {
        DrawRect(x, y, w, h, color);
    }
}

void St7789Display::DrawEye(int cx, int cy, int size, bool blink, bool thinking, bool error, uint16_t color) {
    if (error) {
        uint16_t red = 0xF800;
        DrawHLine(cx - size / 2, cy, size, red);
        DrawHLine(cx - size / 2, cy + 2, size, red);
        DrawVLine(cx, cy - size / 2, size, red);
        DrawVLine(cx + 2, cy - size / 2, size, red);
        return;
    }
    if (blink) {
        DrawRoundRect(cx - size / 2, cy - 1, size, 3, 1, color);
        return;
    }
    int eyeH = thinking ? size * 3 / 4 : size;
    DrawRoundRect(cx - size / 2, cy - eyeH / 2, size, eyeH, size / 4, color);
    // Pupil
    int phase = (millis() / 1400) % 5;
    int look_x = thinking ? 0 : (phase == 1 ? -2 : (phase == 3 ? 2 : 0));
    DrawRoundRect(cx + look_x - size / 6, cy - size / 5, size / 3, size * 2 / 5, size / 8, 0x0000, true);
}

void St7789Display::SendCmd(uint8_t cmd) {
    // Not used - we use esp_lcd_panel_draw_bitmap
}

void St7789Display::SendData(uint8_t data) {
    // Not used
}

void St7789Display::SendData16(uint16_t data) {
    // Not used
}

void St7789Display::WriteBuf(const uint8_t* buf, size_t len) {
    // Not used
}

void St7789Display::SetWindow(int x0, int y0, int x1, int y1) {
    // Not used - handled by esp_lcd_panel_draw_bitmap
}
