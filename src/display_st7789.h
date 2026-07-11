#pragma once

#include <stdint.h>
#include <stddef.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>

class St7789Display {
public:
    St7789Display(int mosi, int sclk, int cs, int dc, int rst, int bl,
                  int width, int height, int offset_x, int offset_y);
    ~St7789Display();

    bool Init();
    void SetBrightness(uint8_t percent);
    void Clear(uint16_t color = 0x0000);

    void SetPixel(int x, int y, uint16_t color);
    void FillRect(int x, int y, int w, int h, uint16_t color);
    void Flush();

    void DrawChar(int x, int y, char c, uint16_t fg, uint16_t bg, uint8_t scale = 1);
    void DrawText(int x, int y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale = 1);
    void DrawCenteredText(int y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale = 1);
    void DrawScrollingText(int y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale = 1);
    int TextWidth(const char* text, uint8_t scale = 1);

    void DrawHLine(int x, int y, int w, uint16_t color);
    void DrawVLine(int x, int y, int h, uint16_t color);
    void DrawRect(int x, int y, int w, int h, uint16_t color);
    void DrawRoundRect(int x, int y, int w, int h, int r, uint16_t color, bool fill = false);
    void DrawEye(int cx, int cy, int size, bool blink, bool thinking, bool error, uint16_t color);

    int width() const { return width_; }
    int height() const { return height_; }

private:
    void SendCmd(uint8_t cmd);
    void SendData(uint8_t data);
    void SendData16(uint16_t data);
    void WriteBuf(const uint8_t* buf, size_t len);
    void SetWindow(int x0, int y0, int x1, int y1);

    int mosi_, sclk_, cs_, dc_, rst_, bl_;
    int width_, height_, offset_x_, offset_y_;
    esp_lcd_panel_io_handle_t panel_io_;
    esp_lcd_panel_handle_t panel_;
    uint16_t* framebuffer_;
    size_t fb_pixels_;
    uint32_t scroll_phase_;
};
