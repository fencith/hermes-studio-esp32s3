#include "display_st7789.h"

#include <Arduino.h>
#include <SPI.h>
#include <cstring>
#include <algorithm>

#define TAG "ST7789"

// 5x7 font (same as original)
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

static SPISettings spiSettings(80000000, MSBFIRST, SPI_MODE3);

St7789Display::St7789Display(int mosi, int sclk, int cs, int dc, int rst, int bl,
                             int width, int height, int offset_x, int offset_y)
    : mosi_(mosi), sclk_(sclk), cs_(cs), dc_(dc), rst_(rst), bl_(bl),
      width_(width), height_(height), offset_x_(offset_x), offset_y_(offset_y),
      framebuffer_(nullptr), fb_pixels_(0), scroll_phase_(0) {}

St7789Display::~St7789Display() {
    delete[] framebuffer_;
}

bool St7789Display::Init() {
    fb_pixels_ = static_cast<size_t>(width_) * height_;
    framebuffer_ = new uint16_t[fb_pixels_]();
    if (!framebuffer_) return false;

    // Setup GPIO
    pinMode(rst_, OUTPUT);
    pinMode(dc_, OUTPUT);
    pinMode(cs_, OUTPUT);
    pinMode(bl_, OUTPUT);
    digitalWrite(cs_, HIGH);

    // Reset sequence
    digitalWrite(rst_, HIGH);
    delay(10);
    digitalWrite(rst_, LOW);
    delay(10);
    digitalWrite(rst_, HIGH);
    delay(120);

    // Init SPI
    SPI.begin(sclk_, -1, mosi_, -1);  // SCLK, MISO, MOSI, SS

    // Backlight on
    digitalWrite(bl_, HIGH);

    // ST7789 init sequence
    WriteCmd(0x01); delay(150);  // SWRESET
    WriteCmd(0x11); delay(150);  // SLPOUT

    WriteCmd(0x36);              // MADCTL
    WriteData(0x00);             // Normal orientation

    WriteCmd(0x3A);              // COLMOD
    WriteData(0x05);             // 16-bit color (RGB565)

    WriteCmd(0xB2);              // PORCTRK
    WriteData(0x0C);
    WriteData(0x0C);
    WriteData(0x00);
    WriteData(0x33);
    WriteData(0x33);

    WriteCmd(0xB7);              // GCTRL
    WriteData(0x75);

    WriteCmd(0xBB);              // VCOMS
    WriteData(0x28);

    WriteCmd(0xC0);              // LCMCTRL
    WriteData(0x2C);

    WriteCmd(0xC2);              // VDVVRHEN
    WriteData(0x01);

    WriteCmd(0xC3);              // VRHS
    WriteData(0x1A);

    WriteCmd(0xC4);              // VDVS
    WriteData(0x1A);

    WriteCmd(0xC6);              // FRCTRL2
    WriteData(0x01);             // 60Hz

    WriteCmd(0xD0);              // PWCTRL1
    WriteData(0xA4);
    WriteData(0xA1);

    WriteCmd(0xE0);              // PVGAMCTRL
    WriteData(0xD0);
    WriteData(0x04);
    WriteData(0x0D);
    WriteData(0x11);
    WriteData(0x13);
    WriteData(0x2B);
    WriteData(0x3F);
    WriteData(0x54);
    WriteData(0x4C);
    WriteData(0x18);
    WriteData(0x0D);
    WriteData(0x0B);
    WriteData(0x1F);
    WriteData(0x23);

    WriteCmd(0xE1);              // NVGAMCTRL
    WriteData(0xD0);
    WriteData(0x04);
    WriteData(0x0C);
    WriteData(0x11);
    WriteData(0x13);
    WriteData(0x2C);
    WriteData(0x3F);
    WriteData(0x44);
    WriteData(0x51);
    WriteData(0x2F);
    WriteData(0x1F);
    WriteData(0x1F);
    WriteData(0x20);
    WriteData(0x23);

    WriteCmd(0x21);              // INVON (color inversion)

    WriteCmd(0x29);              // DISPON
    delay(100);

    Clear(0x0000);
    return true;
}

void St7789Display::SetBrightness(uint8_t percent) {
    analogWrite(bl_, (percent * 255) / 100);
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
    SPI.beginTransaction(spiSettings);
    digitalWrite(cs_, LOW);
    WriteCmd(0x2A);  // CASET
    WriteData(0x00); WriteData(0x00);
    WriteData(0x00); WriteData((width_ - 1) & 0xFF);
    WriteCmd(0x2B);  // RASET
    WriteData(0x00); WriteData(0x00);
    WriteData(0x00); WriteData((height_ - 1) & 0xFF);
    WriteCmd(0x2C);  // RAMWR
    digitalWrite(dc_, HIGH);
    SPI.writeBytes(reinterpret_cast<uint8_t*>(framebuffer_), fb_pixels_ * 2);
    digitalWrite(cs_, HIGH);
    SPI.endTransaction();
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
    FillRect(x, 1, 1, h, color);
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
}

// --- LOW LEVEL ---

void St7789Display::WriteCmd(uint8_t cmd) {
    digitalWrite(dc_, LOW);
    digitalWrite(cs_, LOW);
    SPI.write(cmd);
    digitalWrite(cs_, HIGH);
}

void St7789Display::WriteData(uint8_t data) {
    digitalWrite(dc_, HIGH);
    digitalWrite(cs_, LOW);
    SPI.write(data);
    digitalWrite(cs_, HIGH);
}

void St7789Display::WriteData16(uint16_t data) {
    digitalWrite(dc_, HIGH);
    digitalWrite(cs_, LOW);
    SPI.write16(data);
    digitalWrite(cs_, HIGH);
}

void St7789Display::WriteBuf(const uint8_t* buf, size_t len) {
    digitalWrite(dc_, HIGH);
    digitalWrite(cs_, LOW);
    SPI.writeBytes(buf, len);
    digitalWrite(cs_, HIGH);
}

void St7789Display::SetWindow(int x0, int y0, int x1, int y1) {
    WriteCmd(0x2A);  // CASET
    WriteData(0x00); WriteData(x0 & 0xFF);
    WriteData(0x00); WriteData(x1 & 0xFF);
    WriteCmd(0x2B);  // RASET
    WriteData(0x00); WriteData(y0 & 0xFF);
    WriteData(0x00); WriteData(y1 & 0xFF);
}

void St7789Display::UpdateScroll() {
    // Scrolling update called from DrawScrollingText
}
