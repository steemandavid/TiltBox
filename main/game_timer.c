#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "game_common.h"
#include "display.h"

// States
typedef enum {
    TIMER_SET,
    TIMER_COUNTDOWN,
    TIMER_ALARM
} timer_state_t;

static timer_state_t timer_state;
static int timer_minutes;
static int32_t timer_total_frames;
static int32_t timer_remaining_frames;
static int timer_move_counter;
static int timer_alarm_counter;

// 3x5 digit font (3 LSBs per row)
static const uint8_t timer_digits[10][5] = {
    {0xE, 0xA, 0xA, 0xA, 0xE}, // 0
    {0x4, 0xC, 0x4, 0x4, 0xE}, // 1
    {0xE, 0x2, 0xE, 0x8, 0xE}, // 2
    {0xE, 0x2, 0xE, 0x2, 0xE}, // 3
    {0xA, 0xA, 0xE, 0x2, 0x2}, // 4
    {0xE, 0x8, 0xE, 0x2, 0xE}, // 5
    {0xE, 0x8, 0xE, 0xA, 0xE}, // 6
    {0xE, 0x2, 0x2, 0x4, 0x4}, // 7
    {0xE, 0xA, 0xE, 0xA, 0xE}, // 8
    {0xE, 0xA, 0xE, 0x2, 0xE}, // 9
};

#define TIMER_DEBOUNCE 10
#define TIMER_MAX_MINUTES 99
#define FRAMES_PER_MINUTE 1200  // 20 FPS * 60s

static void draw_digit(int digit, int start_row, int start_col, uint8_t g, uint8_t r, uint8_t b)
{
    for (int row = 0; row < 5; row++) {
        uint8_t bits = timer_digits[digit][row];
        for (int col = 0; col < 3; col++) {
            if (bits & (1 << (2 - col))) {
                set_pixel_at(start_row + row, start_col + col, g, r, b);
            }
        }
    }
}

static void draw_minutes(int mins, uint8_t g, uint8_t r, uint8_t b)
{
    int tens = mins / 10;
    int ones = mins % 10;
    // Two digits: 3 cols + 1 gap + 3 cols = 7, centered in 8
    draw_digit(tens, 1, 0, g, r, b);
    draw_digit(ones, 1, 4, g, r, b);
}

static void draw_progress_bar(float fraction)
{
    int lit = (int)(fraction * 8.0f);
    if (lit < 0) lit = 0;
    if (lit > 8) lit = 8;
    for (int x = 0; x < lit; x++) {
        set_pixel_at(7, x, 0x20, 0x00, 0x20); // dim blue
    }
}

void timer_init(void)
{
    timer_state = TIMER_SET;
    timer_minutes = 5;
    timer_remaining_frames = 0;
    timer_total_frames = 0;
    timer_move_counter = 0;
    timer_alarm_counter = 0;
}

void timer_update(int16_t dx, int16_t dy, int16_t z)
{
    (void)z;
    timer_move_counter++;

    switch (timer_state) {
    case TIMER_SET:
        if (timer_move_counter >= TIMER_DEBOUNCE) {
            if (dx > TILT_THRESHOLD && timer_minutes < TIMER_MAX_MINUTES) {
                timer_minutes++;
                timer_move_counter = 0;
            } else if (dx < -TILT_THRESHOLD && timer_minutes > 1) {
                timer_minutes--;
                timer_move_counter = 0;
            }
        }
        if (dy > TILT_THRESHOLD * 2 && timer_move_counter >= TIMER_DEBOUNCE) {
            timer_total_frames = timer_minutes * FRAMES_PER_MINUTE;
            timer_remaining_frames = timer_total_frames;
            timer_state = TIMER_COUNTDOWN;
            timer_move_counter = 0;
        }
        break;

    case TIMER_COUNTDOWN:
        timer_remaining_frames--;
        if (timer_remaining_frames <= 0) {
            timer_state = TIMER_ALARM;
            timer_alarm_counter = 0;
        }
        break;

    case TIMER_ALARM:
        timer_alarm_counter++;
        if ((abs(dx) > TILT_THRESHOLD || abs(dy) > TILT_THRESHOLD) && timer_alarm_counter > 20) {
            timer_state = TIMER_SET;
            timer_alarm_counter = 0;
        }
        if (timer_alarm_counter >= 600) {
            timer_state = TIMER_SET;
        }
        break;
    }
}

void timer_draw(void)
{
    clear_display();

    switch (timer_state) {
    case TIMER_SET: {
        // Blink digits at ~1Hz (10 on, 10 off)
        bool show = (anim_frame % 20) < 14;
        if (show) {
            draw_minutes(timer_minutes, GREEN);
        }
        // Small dot as "set" indicator
        set_pixel_at(0, 3, 0x10, 0x10, 0x00);
        break;
    }

    case TIMER_COUNTDOWN: {
        int remaining_min = timer_remaining_frames / FRAMES_PER_MINUTE;
        if (remaining_min < 1) {
            // Show seconds (< 1 min left)
            int remaining_sec = timer_remaining_frames / 20;
            draw_minutes(remaining_sec, YELLOW);
        } else {
            draw_minutes(remaining_min, 0x00, 0x30, 0x30); // cyan-ish
        }
        float fraction = (float)timer_remaining_frames / timer_total_frames;
        draw_progress_bar(fraction);
        break;
    }

    case TIMER_ALARM: {
        // Flash between red and yellow
        bool flash = (timer_alarm_counter % 10) < 5;
        if (flash) {
            for (int y = 0; y < 8; y++)
                for (int x = 0; x < 8; x++)
                    set_pixel_at(y, x, RED);
        } else {
            for (int y = 0; y < 8; y++)
                for (int x = 0; x < 8; x++)
                    set_pixel_at(y, x, YELLOW);
        }
        break;
    }
    }
}
