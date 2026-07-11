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
// IMPORTANT: use SPI3_HOST (VSPI), NOT SPI2_HOST (FSPI)!
// SPI2_HOST is used by Arduino's SPI.begin() internally, causing conflicts.
#define LCD_HOST SPI3_HOST

// 5x7 font table (96 chars)
static const uint8_t kFont5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},{0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},{0x00,0x1C,0x22,0x41,0x00},{0x00,0x41,0x22,0x1C,0x00},{0x08,0x2A,0x1C,0x2A,0x08},{0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},{0x20,0x10,0x08,0x04,0x02},{0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},{0x00,0x56,0x36,0x00,0x00},{0x00,0x08,0x14,0x22,0x41},{0x14,0x14,0x14,0x14,0x14},
    {0x41,0x22,0x14,0x08,0x00},{0x02,0x01,0x51,0x09,0x06},{0x32,0x49,0x79,0x41,0x3E},{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},{0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},{0x7F,0x02,0x04,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},{0x7F,0x40,0x30,0x40,0x7F},{0x63,0x14,0x08,0x14,0x63},{0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},{0x00,0x00,0x7F,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x7F,0x00,0x00},{0x04,0x02,0x01,0x02,0x04},{0x40,0x40,0x40,0x40,0x40},{0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},
    {0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},{0x38,0x44,0x44,0x48,0x7F},{0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x08,0x14,0x54,0x54,0x3C},
    {0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},{0x00,0x7F,0x10,0x28,0x44},{0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x18,0x04,0x78},
    {0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},{0x7C,0x14,0x14,0x14,0x08},{0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},{0x3C,0x40,0x30,0x40,0x3C},{0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},
    {0x44,0x64,0x54,0x4C,0x44},
};

static esp_lcd_panel_io_handle_t s_panel_io = nullptr;
static esp_lcd_panel_handle_t s_panel = nullptr;

St7789Display::St7789Display(int mosi, int sclk, int cs, int dc, int rst, int bl,
                             int width, int height, int offset_x, int offset_y)
    : mosi_(mosi), sclk_(sclk), cs_(cs), dc_(dc), rst_(rst), bl_(bl),
      width_(width), height_(height), offset_x_(offset_x), offset_y_(offset_y),
      framebuffer_(nullptr), fb_pixels_(0), scroll_phase_(0) {}

St7789Display::~St7789Display() {
    delete[] framebuffer_;
    if (s_panel) { esp_lcd_panel_del(s_panel); s_panel = nullptr; }
    if (s_panel_io) { esp_lcd_panel_io_del(s_panel_io); s_panel_io = nullptr; }
}

bool St7789Display::Init() {
    ESP_LOGI(TAG, "Init ST7789 via ESP-IDF (SPI3_HOST)...");

    // Allocate framebuffer
    fb_pixels_ = static_cast<size_t>(width_) * height_;
    framebuffer_ = new (std::nothrow) uint16_t[fb_pixels_]();
    if (!framebuffer_) { ESP_LOGE(TAG, "No mem for fb"); return false; }

    // === Step 1: Init SPI bus on SPI3_HOST (AVOID SPI2 - Arduino uses it!) ===
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = mosi_;            // GPIO 41
    buscfg.miso_io_num = GPIO_NUM_NC;
    buscfg.sclk_io_num = sclk_;            // GPIO 42
    buscfg.quadwp_io_num = GPIO_NUM_NC;
    buscfg.quadhd_io_num = GPIO_NUM_NC;
    buscfg.max_transfer_sz = fb_pixels_ * 2;

    esp_err_t ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI3 init fail: %d", ret);
        return false;
    }
    ESP_LOGI(TAG, "SPI3 OK: MOSI=%d SCLK=%d", mosi_, sclk_);

    // === Step 2: Attach panel IO (CS, DC, SPI mode 3, 80MHz) ===
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = cs_;            // GPIO 21
    io_config.dc_gpio_num = dc_;            // GPIO 40
    io_config.spi_mode = 3;
    io_config.pclk_hz = 80 * 1000 * 1000;  // 80MHz
    io_config.trans_queue_depth = 10;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;

    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &s_panel_io);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "IO fail: %d", ret); spi_bus_free(LCD_HOST); return false; }
    ESP_LOGI(TAG, "Panel IO: CS=%d DC=%d", cs_, dc_);

    // === Step 3: Create ST7789 panel ===
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = rst_;     // GPIO 45 (handled by driver!)
    panel_config.color_space = ESP_LCD_COLOR_SPACE_RGB;
    panel_config.bits_per_pixel = 16;

    ret = esp_lcd_new_panel_st7789(s_panel_io, &panel_config, &s_panel);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Panel fail: %d", ret); esp_lcd_panel_io_del(s_panel_io); s_panel_io = nullptr; spi_bus_free(LCD_HOST); return false; }
    ESP_LOGI(TAG, "ST7789 panel created");

    // === Step 4: Init and configure ===
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel, false));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, true));
    ESP_LOGI(TAG, "Panel init done");

    // === Step 5: Backlight via LEDC ===
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);
    ledc_channel_config_t ledc_channel = {
        .gpio_num = bl_,                    // GPIO 20
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 255,
        .hpoint = 0,
    };
    ledc_channel_config(&ledc_channel);
    ESP_LOGI(TAG, "BL ON (GPIO %d)", bl_);

    // === Step 6: Turn display ON and show test pattern ===
    // First draw white, then turn on (same as xiaozhi-esp32)
    for (size_t i = 0; i < fb_pixels_; i++) framebuffer_[i] = 0xFFFF;
    esp_lcd_panel_draw_bitmap(s_panel, 0, 0, width_, height_, (uint8_t*)framebuffer_);
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));
    delay(20);
    
    // Draw test: red top, green middle, blue bottom
    for (size_t i = 0; i < width_ * 10; i++) framebuffer_[i] = 0xF800;
    for (size_t i = (height_-10) * width_; i < fb_pixels_; i++) framebuffer_[i] = 0x001F;
    esp_lcd_panel_draw_bitmap(s_panel, 0, 0, width_, height_, (uint8_t*)framebuffer_);

    ESP_LOGI(TAG, "ST7789 ready! (SPI3, 80MHz)");
    return true;
}

void St7789Display::SetBrightness(uint8_t percent) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (percent * 255) / 100);
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
    for (int yy = y0; yy < y1; yy++)
        for (int xx = x0; xx < x1; xx++)
            framebuffer_[yy * width_ + xx] = color;
}

void St7789Display::Flush() {
    if (!s_panel) return;
    esp_lcd_panel_draw_bitmap(s_panel, 0, 0, width_, height_, (uint8_t*)framebuffer_);
}

void St7789Display::DrawChar(int x, int y, char c, uint16_t fg, uint16_t bg, uint8_t scale) {
    if (c < ' ' || c >= 127) return;
    c -= ' ';
    const uint8_t* glyph = kFont5x7[(uint8_t)c];
    for (int col = 0; col < 5; col++)
        for (int row = 0; row < 7; row++)
            for (int sx = 0; sx < scale; sx++)
                for (int sy = 0; sy < scale; sy++)
                    SetPixel(x + col*scale + sx, y + row*scale + sy,
                             (glyph[col] & (1<<row)) ? fg : bg);
}

void St7789Display::DrawText(int x, int y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale) {
    for (int i = 0; text[i]; i++)
        DrawChar(x + i*6*scale, y, text[i], fg, bg, scale);
}

int St7789Display::TextWidth(const char* text, uint8_t scale) {
    int len = strlen(text);
    return len * 6 * scale - scale;
}

void St7789Display::DrawCenteredText(int y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale) {
    int x = (width_ - TextWidth(text, scale)) / 2;
    if (x < 0) x = 0;
    DrawText(x, y, text, fg, bg, scale);
}

void St7789Display::DrawScrollingText(int y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale) {
    int tw = TextWidth(text, scale);
    if (tw <= width_) { DrawCenteredText(y, text, fg, bg, scale); return; }
    scroll_phase_ = (scroll_phase_ + 1) % (tw + 18*scale);
    DrawText(-int(scroll_phase_/2), y, text, fg, bg, scale);
    if (int(tw + 18*scale - scroll_phase_/2) < width_)
        DrawText(tw + 18*scale - int(scroll_phase_/2), y, text, fg, bg, scale);
}

void St7789Display::DrawHLine(int x, int y, int w, uint16_t color) { FillRect(x, y, w, 1, color); }
void St7789Display::DrawVLine(int x, int y, int h, uint16_t color) { FillRect(x, y, 1, h, color); }
void St7789Display::DrawRect(int x, int y, int w, int h, uint16_t color) {
    DrawHLine(x, y, w, color); DrawHLine(x, y+h-1, w, color);
    DrawVLine(x, y, h, color); DrawVLine(x+w-1, y, h, color);
}
void St7789Display::DrawRoundRect(int x, int y, int w, int h, int r, uint16_t color, bool fill) {
    r = std::max(0, std::min(r, std::min(w, h)/2));
    if (fill) FillRect(x, y, w, h, color); else DrawRect(x, y, w, h, color);
}
void St7789Display::DrawEye(int cx, int cy, int size, bool blink, bool thinking, bool error, uint16_t color) {
    if (error) {
        DrawHLine(cx-size/2, cy, size, 0xF800); DrawHLine(cx-size/2, cy+2, size, 0xF800);
        DrawVLine(cx, cy-size/2, size, 0xF800); DrawVLine(cx+2, cy-size/2, size, 0xF800);
        return;
    }
    if (blink) { DrawRoundRect(cx-size/2, cy-1, size, 3, 1, color); return; }
    int h = thinking ? size*3/4 : size;
    DrawRoundRect(cx-size/2, cy-h/2, size, h, size/4, color);
}
