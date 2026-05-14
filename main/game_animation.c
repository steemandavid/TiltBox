#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_random.h"
#include "game_common.h"
#include "display.h"

typedef enum {
    ANIM_SPIRAL,
    ANIM_WAVE,
    ANIM_RAINBOW,
    ANIM_FIRE,
    ANIM_MATRIX,
    ANIM_PLASMA,
    ANIM_LIFE,
    ANIM_FIREWORKS,
    ANIM_COUNT
} animation_t;

static animation_t current_anim = ANIM_SPIRAL;
static uint32_t anim_cycle_counter = 0;
#define ANIM_CYCLE_FRAMES 200

// --- Fire state ---
static uint8_t fire_heat[8][8];

// --- Matrix state ---
static int8_t matrix_drop[8];
static int8_t matrix_speed[8];
static int8_t matrix_length[8];

// --- Life state ---
static bool life_buf_a[8][8];
static bool life_buf_b[8][8];
static bool life_initialized = false;

// --- Fireworks state ---
#define FW_MAX_PARTICLES 16
#define FW_COUNT 3
typedef struct {
    float x, y;
    float vx, vy;
    uint8_t brightness;
    uint8_t hue;
} fw_particle_t;

typedef struct {
    float x, y;
    float vy;
    uint8_t phase; // 0=rising, 1=burst, 2=done
    int age;
    fw_particle_t particles[FW_MAX_PARTICLES];
    int particle_count;
    uint8_t hue;
} firework_t;

static firework_t fireworks[FW_COUNT];

// ===== Existing patterns =====

static void draw_spiral(void)
{
    static const int8_t spiral_x[] = {3,4,4,3,3,4,5,5,5,4,3,2,2,2,2,3,4,5,6,6,6,6,6,5,4,3,2,1,1,1,1,1,1,2,3,4,5,6,7,7,7,7,7,7,7,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,1,2,3};
    static const int8_t spiral_y[] = {3,3,4,4,3,3,3,4,5,5,5,5,4,3,2,2,2,2,2,3,4,5,6,6,6,6,6,6,5,4,3,2,1,1,1,1,1,1,1,2,3,4,5,6,7,7,7,7,7,7,7,7,7,6,5,4,3,2,1,0,0,0,0,0};

    int head = (anim_frame / 2) % 64;
    int trail_len = 24;

    for (int t = 0; t < trail_len; t++) {
        int i = (head - t + 64) % 64;
        uint8_t brightness = (trail_len - t) * 3;
        if (brightness > 72) brightness = 72;
        uint8_t hue = (i * 4 + anim_frame * 2) & 0xFF;
        uint8_t g, r, b;
        hsv_to_rgb(hue, 255, brightness, &g, &r, &b);
        set_pixel_at(spiral_y[i], spiral_x[i], g, r, b);
    }
}

static void draw_wave(void)
{
    for (int x = 0; x < 8; x++) {
        float wave = 3.5f + 3.0f * sinf((x + anim_frame * 0.15f) * 0.8f);
        int y = (int)wave;
        if (y >= 0 && y < 8) {
            uint8_t hue = (x * 20 + anim_frame) & 0xFF;
            uint8_t g, r, b;
            hsv_to_rgb(hue, 255, 64, &g, &r, &b);
            set_pixel_at(y, x, g, r, b);
        }
    }
}

static void draw_rainbow(void)
{
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            uint8_t hue = (x * 20 + y * 20 + anim_frame * 3) & 0xFF;
            uint8_t g, r, b;
            hsv_to_rgb(hue, 255, 48, &g, &r, &b);
            set_pixel_at(y, x, g, r, b);
        }
    }
}

// ===== New patterns =====

static void fire_init(void)
{
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
            fire_heat[y][x] = 0;
}

static void draw_fire(void)
{
    // Seed bottom row with random heat
    for (int x = 0; x < 8; x++) {
        fire_heat[7][x] = (esp_random() % 160) + 60;
        if (fire_heat[7][x] > 255) fire_heat[7][x] = 255;
    }

    // Propagate upward with cooling
    for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 8; x++) {
            int below = fire_heat[y + 1][x];
            int below_l = fire_heat[y + 1][(x - 1 + 8) % 8];
            int below_r = fire_heat[y + 1][(x + 1) % 8];
            int avg = (below + below_l + below_r) / 3;
            int cool = esp_random() % 40 + 10;
            fire_heat[y][x] = (avg - cool < 0) ? 0 : avg - cool;
        }
    }

    // Render heat to color
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            uint8_t h = fire_heat[y][x];
            uint8_t r = (h > 128) ? 255 : h * 2;
            uint8_t g = (h > 180) ? (h - 180) * 3 : 0;
            uint8_t b = (h > 220) ? (h - 220) * 6 : 0;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            set_pixel_at(y, x, g, r, b);
        }
    }
}

static void matrix_init(void)
{
    for (int x = 0; x < 8; x++) {
        matrix_drop[x] = -(esp_random() % 16);
        matrix_speed[x] = 1 + (esp_random() % 2);
        matrix_length[x] = 2 + (esp_random() % 4);
    }
}

static void draw_matrix(void)
{
    // Update drops
    for (int x = 0; x < 8; x++) {
        if (anim_frame % matrix_speed[x] == 0) {
            matrix_drop[x]++;
        }
        if (matrix_drop[x] - matrix_length[x] > 8) {
            matrix_drop[x] = -(esp_random() % 8);
            matrix_speed[x] = 1 + (esp_random() % 2);
            matrix_length[x] = 2 + (esp_random() % 4);
        }
    }

    // Draw trails
    for (int x = 0; x < 8; x++) {
        int head = matrix_drop[x];
        if (head >= 0 && head < 8) {
            // Bright white-green head
            set_pixel_at(head, x, 0x60, 0xFF, 0x60);
        }
        for (int t = 1; t < matrix_length[x]; t++) {
            int ty = head - t;
            if (ty >= 0 && ty < 8) {
                uint8_t brightness = 0x40 - t * 0x10;
                if (brightness > 0x40) brightness = 0;
                set_pixel_at(ty, x, brightness, brightness >> 1, 0x00);
            }
        }
    }
}

static void draw_plasma(void)
{
    float t = anim_frame * 0.05f;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            float v = sinf(x * 0.8f + t)
                    + sinf(y * 0.8f + t * 0.7f)
                    + sinf((x + y) * 0.5f + t * 0.5f)
                    + sinf(sqrtf((x - 3.5f) * (x - 3.5f) + (y - 3.5f) * (y - 3.5f)) * 0.8f + t);
            uint8_t hue = (uint8_t)((v + 4.0f) * 31.875f); // map [-4,4] to [0,255]
            uint8_t g, r, b;
            hsv_to_rgb(hue, 255, 48, &g, &r, &b);
            set_pixel_at(y, x, g, r, b);
        }
    }
}

static void life_seed(void)
{
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
            life_buf_a[y][x] = (esp_random() % 3 == 0);
}

static int life_count_neighbors(bool buf[8][8], int cy, int cx)
{
    int count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dy == 0 && dx == 0) continue;
            int ny = (cy + dy + 8) % 8;
            int nx = (cx + dx + 8) % 8;
            if (buf[ny][nx]) count++;
        }
    }
    return count;
}

static void draw_life(void)
{
    if (!life_initialized) {
        life_seed();
        life_initialized = true;
    }

    // Step simulation every 4 frames
    if (anim_frame % 4 == 0) {
        int population = 0;
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                int n = life_count_neighbors(life_buf_a, y, x);
                life_buf_b[y][x] = (life_buf_a[y][x]) ? (n == 2 || n == 3) : (n == 3);
                if (life_buf_b[y][x]) population++;
            }
        }
        // Swap buffers
        memcpy(life_buf_a, life_buf_b, sizeof(life_buf_a));

        // Re-seed if extinct or stale (checked every step)
        if (population == 0 || population == 64) {
            life_seed();
        }
    }

    // Draw
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (life_buf_a[y][x]) {
                uint8_t hue = (x * 30 + y * 30 + anim_frame) & 0xFF;
                uint8_t g, r, b;
                hsv_to_rgb(hue, 200, 50, &g, &r, &b);
                set_pixel_at(y, x, g, r, b);
            }
        }
    }
}

static void fireworks_init(void)
{
    for (int i = 0; i < FW_COUNT; i++) {
        fireworks[i].phase = 2; // start "done" so they launch on first frame
        fireworks[i].age = 0;
    }
}

static void firework_launch(firework_t *fw)
{
    fw->x = 1.0f + (esp_random() % 6);
    fw->y = 7.0f;
    fw->vy = -(0.3f + (esp_random() % 20) / 40.0f);
    fw->phase = 0;
    fw->age = 0;
    fw->hue = esp_random() & 0xFF;
    fw->particle_count = 0;
}

static void draw_fireworks(void)
{
    for (int i = 0; i < FW_COUNT; i++) {
        firework_t *fw = &fireworks[i];
        fw->age++;

        if (fw->phase == 0) {
            // Rising
            fw->y += fw->vy;
            if (fw->y <= 1.0f + (esp_random() % 3)) {
                // Burst
                fw->phase = 1;
                fw->age = 0;
                fw->particle_count = 6 + (esp_random() % (FW_MAX_PARTICLES - 5));
                for (int p = 0; p < fw->particle_count; p++) {
                    float angle = (p * 2.0f * M_PI) / fw->particle_count + (esp_random() % 10) / 50.0f;
                    float speed = 0.2f + (esp_random() % 15) / 30.0f;
                    fw->particles[p].x = fw->x;
                    fw->particles[p].y = fw->y;
                    fw->particles[p].vx = cosf(angle) * speed;
                    fw->particles[p].vy = sinf(angle) * speed;
                    fw->particles[p].brightness = 255;
                    fw->particles[p].hue = fw->hue + (esp_random() % 40);
                }
            } else {
                // Draw rising dot
                int px = (int)fw->x;
                int py = (int)fw->y;
                if (px >= 0 && px < 8 && py >= 0 && py < 8) {
                    set_pixel_at(py, px, 0x40, 0x40, 0x40);
                }
            }
        }

        if (fw->phase == 1) {
            bool all_dead = true;
            for (int p = 0; p < fw->particle_count; p++) {
                fw_particle_t *pt = &fw->particles[p];
                pt->x += pt->vx;
                pt->y += pt->vy;
                pt->vy += 0.015f; // gravity
                if (pt->brightness > 8) pt->brightness -= 4;
                else pt->brightness = 0;
                if (pt->brightness > 0) all_dead = false;

                int px = (int)pt->x;
                int py = (int)pt->y;
                if (px >= 0 && px < 8 && py >= 0 && py < 8 && pt->brightness > 0) {
                    uint8_t g, r, b;
                    hsv_to_rgb(pt->hue, 200, pt->brightness >> 2, &g, &r, &b);
                    set_pixel_at(py, px, g, r, b);
                }
            }
            if (all_dead) fw->phase = 2;
        }

        if (fw->phase == 2 && fw->age > 20 + i * 10) {
            firework_launch(fw);
        }
    }
}

// ===== Main animation interface =====

void animation_init(void)
{
    anim_frame = 0;
    anim_cycle_counter = 0;
    current_anim = ANIM_SPIRAL;
    fire_init();
    matrix_init();
    life_initialized = false;
    fireworks_init();
}

void animation_update(int16_t dx, int16_t dy, int16_t z)
{
    (void)dx; (void)dy; (void)z;

    anim_frame++;
    anim_cycle_counter++;

    if (anim_cycle_counter >= ANIM_CYCLE_FRAMES) {
        anim_cycle_counter = 0;
        current_anim = (current_anim + 1) % ANIM_COUNT;
        // Reset state for patterns that need it
        if (current_anim == ANIM_FIRE) fire_init();
        if (current_anim == ANIM_MATRIX) matrix_init();
        if (current_anim == ANIM_LIFE) life_initialized = false;
        if (current_anim == ANIM_FIREWORKS) fireworks_init();
    }
}

void animation_draw(void)
{
    clear_display();

    switch (current_anim) {
        case ANIM_SPIRAL:    draw_spiral(); break;
        case ANIM_WAVE:      draw_wave(); break;
        case ANIM_RAINBOW:   draw_rainbow(); break;
        case ANIM_FIRE:      draw_fire(); break;
        case ANIM_MATRIX:    draw_matrix(); break;
        case ANIM_PLASMA:    draw_plasma(); break;
        case ANIM_LIFE:      draw_life(); break;
        case ANIM_FIREWORKS: draw_fireworks(); break;
        default: break;
    }
}
