#include "display_st7789.h"
#include "hw_config.h"

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
#define LCD_HOST SPI2_HOST

// 5x7 font
static const uint8_t kFont5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},
    {0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},{0x00,0x1C,0x22,0x41,0x00},
    {0x00,0x41,0x22,0x1C,0x00},{0x08,0x2A,0x1C,0x2A,0x08},{0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x02},{0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},
    {0x00,0x56,0x36,0x00,0x00},{0x00,0x08,0x14,0x22,0x41},{0x14,0x14,0x14,0x14,0x14},
    {0x41,0x22,0x14,0x08,0x00},{0x02,0x01,0x51,0x09,0x06},{0x32,0x49,0x79,0x41,0x3E},
    {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x04,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},{0x7F,0x40,0x30,0x40,0x7F},{0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},{0x00,0x00,0x7F,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x7F,0x00,0x00},{0x04,0x02,0x01,0x02,0x04},
    {0x40,0x40,0x40,0x40,0x40},{0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},
    {0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},{0x38,0x44,0x44,0x48,0x7F},
    {0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x08,0x14,0x54,0x54,0x3C},
    {0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},
    {0x00,0x7F,0x10,0x28,0x44},{0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x18,0x04,0x78},
    {0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},{0x7C,0x14,0x14,0x14,0x08},
    {0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},
    {0x3C,0x40,0x30,0x40,0x3C},{0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},
    {0x44,0x64,0x54,0x4C,0x44},
};

// Forward declare the IDF panel
static esp_lcd_panel_io_handle_t s_panel_io = nullptr;
static esp_lcd_panel_handle_t s_panel = nullptr;

St7789Display::St7789Display(int mosi, int sclk, int cs, int dc, int rst, int bl,
                             int width, int height, int offset_x, int offset_y)
    : mosi_(mosi), sclk_(sclk), cs_(cs), dc_(dc), rst_(rst), bl_(bl),
      width_(width), height_(height), offset_x_(offset_x), offset_y_(offset_y),
      framebuffer_(nullptr), fb_pixels_(0), scroll_phase_(0) {}

St7789Display::~St7789Display() {
    delete[] framebuffer_;
    if (s_panel) {
        esp_lcd_panel_del(s_panel);
        s_panel = nullptr;
    }
    if (s_panel_io) {
        esp_lcd_panel_io_del(s_panel_io);
        s_panel_io = nullptr;
    }
}

bool St7789Display::Init() {
    ESP_LOGI(TAG, "Initializing ST7789 LCD via ESP-IDF SPI...");

    fb_pixels_ = static_cast<size_t>(width_) * height_;
    framebuffer_ = new (std::nothrow) uint16_t[fb_pixels_]();
    if (!framebuffer_) {
        ESP_LOGE(TAG, "Framebuffer alloc failed (%zu bytes)", fb_pixels_ * 2);
        return false;
    }

    // === Step 1: Free GPIO 41 and 42 from any peripheral ===
    // On ESP32-S3, GPIO 41/42 are USB D-/D+. Explicitly reset them.
    gpio_reset_pin(static_cast<gpio_num_t>(mosi_));
    gpio_reset_pin(static_cast<gpio_num_t>(sclk_));
    gpio_reset_pin(static_cast<gpio_num_t>(cs_));
    gpio_reset_pin(static_cast<gpio_num_t>(dc_));
    gpio_reset_pin(static_cast<gpio_num_t>(rst_));
    gpio_reset_pin(static_cast<gpio_num_t>(bl_));
    delay(10);

    // === Step 2: Hardware reset ===
    pinMode(rst_, OUTPUT);
    pinMode(dc_, OUTPUT);
    pinMode(cs_, OUTPUT);
    pinMode(bl_, OUTPUT);
    digitalWrite(cs_, HIGH);
    digitalWrite(dc_, HIGH);
    digitalWrite(bl_, LOW);

    digitalWrite(rst_, HIGH);
    delay(5);
    digitalWrite(rst_, LOW);
    delay(10);
    digitalWrite(rst_, HIGH);
    delay(150);

    ESP_LOGI(TAG, "Reset done, initializing SPI bus...");

    // === Step 3: Initialize SPI bus using IDF API (same as xiaozhi-esp32) ===
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = mosi_;
    buscfg.miso_io_num = GPIO_NUM_NC;
    buscfg.sclk_io_num = sclk_;
    buscfg.quadwp_io_num = GPIO_NUM_NC;
    buscfg.quadhd_io_num = GPIO_NUM_NC;
    buscfg.max_transfer_sz = fb_pixels_ * 2;  // Full framebuffer

    esp_err_t ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %d", ret);
        return false;
    }
    ESP_LOGI(TAG, "SPI bus initialized on host %d (MOSI=%d, SCLK=%d)", LCD_HOST, mosi_, sclk_);

    // === Step 4: Attach LCD panel IO ===
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = cs_;
    io_config.dc_gpio_num = dc_;
    io_config.spi_mode = 3;
    io_config.pclk_hz = 80 * 1000 * 1000;  // 80MHz
    io_config.trans_queue_depth = 10;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;

    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &s_panel_io);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel IO init failed: %d", ret);
        spi_bus_free(LCD_HOST);
        return false;
    }
    ESP_LOGI(TAG, "Panel IO created on CS=%d DC=%d", cs_, dc_);

    // === Step 5: Create ST7789 panel (using Espressif vendor driver) ===
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = rst_;
    panel_config.color_space = ESP_LCD_COLOR_SPACE_RGB;
    panel_config.bits_per_pixel = 16;

    ret = esp_lcd_new_panel_st7789(s_panel_io, &panel_config, &s_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel creation failed: %d", ret);
        esp_lcd_panel_io_del(s_panel_io);
        s_panel_io = nullptr;
        spi_bus_free(LCD_HOST);
        return false;
    }

    // === Step 6: Init panel ===
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel, false));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    ESP_LOGI(TAG, "Panel init done");

    // === Step 7: Backlight via LEDC PWM ===
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
    ESP_LOGI(TAG, "Backlight ON (GPIO %d)", bl_);

    // === Step 8: Clear and show test pattern ===
    Clear(0x0000);
    Flush();
    delay(50);

    // Draw a test pattern to confirm display works
    FillRect(0, 0, width_, 30, 0xF800);   // Red bar top
    FillRect(0, height_-30, width_, 30, 0x001F);  // Blue bar bottom
    DrawCenteredText(height_/2 - 10, "Hello!", 0xFFFF, 0x0000, 2);
    Flush();

    ESP_LOGI(TAG, "ST7789 LCD ready!");
    return true;
}

void St7789Display::SetBrightness(uint8_t percent) {
    uint32_t duty = (static_cast<uint32_t>(percent) * 255) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
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
        int row = yy * width_;
        for (int xx = x0; xx < x1; xx++) {
            framebuffer_[row + xx] = color;
        }
    }
}

void St7789Display::Flush() {
    if (!s_panel) return;
    esp_lcd_panel_draw_bitmap(s_panel, 0, 0, width_, height_,
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
    return len * 6 * scale - scale;
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
    if (fill) FillRect(x, y, w, h, color);
    else DrawRect(x, y, w, h, color);
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
}
