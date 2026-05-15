#include <stdint.h>
#include <stdbool.h>
#include "game_common.h"
#include "display.h"

// 5x7 font (5 bytes per char, each byte is a column, bit 0 = top row)
// Text: "Makerspace Baasrode"
static const uint8_t font_5x7[][5] = {
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},  // M
    {0x20, 0x54, 0x54, 0x7C, 0x00},  // a
    {0x7F, 0x08, 0x14, 0x22, 0x41},  // k
    {0x38, 0x54, 0x54, 0x54, 0x18},  // e
    {0x7C, 0x04, 0x08, 0x04, 0x00},  // r
    {0x48, 0x54, 0x54, 0x54, 0x20},  // s
    {0x7C, 0x14, 0x14, 0x1C, 0x00},  // p
    {0x20, 0x54, 0x54, 0x7C, 0x00},  // a
    {0x38, 0x44, 0x44, 0x44, 0x20},  // c
    {0x38, 0x54, 0x54, 0x54, 0x18},  // e
    {0x00, 0x00, 0x00, 0x00, 0x00},  // (space)
    {0x7F, 0x49, 0x49, 0x49, 0x36},  // B
    {0x20, 0x54, 0x54, 0x7C, 0x00},  // a
    {0x20, 0x54, 0x54, 0x7C, 0x00},  // a
    {0x48, 0x54, 0x54, 0x54, 0x20},  // s
    {0x7C, 0x04, 0x08, 0x04, 0x00},  // r
    {0x38, 0x44, 0x44, 0x44, 0x38},  // o
    {0x18, 0x24, 0x24, 0x20, 0x7F},  // d
    {0x38, 0x54, 0x54, 0x54, 0x18},  // e
};

#define SCROLL_TEXT_LEN 19
#define SCROLL_CHAR_WIDTH 6
#define SCROLL_TOTAL_WIDTH (SCROLL_TEXT_LEN * SCROLL_CHAR_WIDTH)
static float scroll_pos = 8.0f;

void scroll_text_init(void)
{
    scroll_pos = 8.0f;
}

void scroll_text_update(int16_t dx, int16_t dy, int16_t z)
{
    (void)dx; (void)dy; (void)z;

    scroll_pos -= 0.5f;

    if (scroll_pos < -SCROLL_TOTAL_WIDTH) {
        scroll_pos = 8.0f;
    }
}

void scroll_text_draw(void)
{
    clear_display();

    for (int screen_x = 0; screen_x < 8; screen_x++) {
        float text_col_f = screen_x - scroll_pos;
        int text_col = (int)text_col_f;
        float frac = text_col_f - text_col;

        for (int blend = 0; blend < 2; blend++) {
            int col = text_col + blend;
            if (col < 0 || col >= SCROLL_TOTAL_WIDTH) continue;

            int char_idx = col / SCROLL_CHAR_WIDTH;
            int char_col = col % SCROLL_CHAR_WIDTH;

            if (char_idx < 0 || char_idx >= SCROLL_TEXT_LEN) continue;
            if (char_col >= 5) continue;

            uint8_t column_data = font_5x7[char_idx][char_col];

            float blend_factor = (blend == 0) ? (1.0f - frac) : frac;
            uint8_t brightness = (uint8_t)(70.0f * blend_factor);

            for (int bit = 0; bit < 7; bit++) {
                if (column_data & (1 << bit)) {
                    int screen_y = bit;
                    if (screen_y >= 0 && screen_y < 8) {
                        int idx = get_led_index(screen_y, screen_x);
                        uint8_t curr_g = led_data[idx * 3 + 0];
                        uint8_t curr_r = led_data[idx * 3 + 1];

                        uint8_t new_g = curr_g + brightness / 2;
                        uint8_t new_r = curr_r + brightness;
                        if (new_g > 70) new_g = 70;
                        if (new_r > 70) new_r = 70;

                        led_data[idx * 3 + 0] = new_g;
                        led_data[idx * 3 + 1] = new_r;
                        led_data[idx * 3 + 2] = 0;
                    }
                }
            }
        }
    }
}
