# CLAUDE.md - TiltBox

This file provides guidance to Claude Code (claude.ai/code) when working with the TiltBox project.

## Build Commands

This is an ESP-IDF project. Use the `idf.py` tool for all build operations:

```bash
idf.py build          # Build the project
idf.py flash          # Flash to connected ESP32
idf.py monitor        # Open serial monitor (Ctrl+] to exit)
idf.py flash monitor  # Flash and immediately open monitor
idf.py menuconfig     # Configure project options (stored in sdkconfig)
idf.py fullclean      # Clean all build artifacts
```

Ensure the ESP-IDF environment is sourced before running commands (`source ~/esp/esp-idf/export.sh`).

## Architecture

TiltBox is an ESP32-C3 project driving an 8x8 WS2812 LED matrix with an ADXL345 accelerometer for tilt control.

**Hardware interfaces:**
- WS2812 LEDs on GPIO 10 via RMT peripheral
- ADXL345 accelerometer on I2C (SDA: GPIO 8, SCL: GPIO 9)

**Source structure:**
- `main/main.c` — Entry point, flip detection, game dispatch table, main loop
- `main/display.c/h` — WS2812 LED driver, pixel drawing, HSV conversion
- `main/accel.c/h` — I2C and ADXL345 accelerometer driver, calibration
- `main/game_common.h` — Shared types (game_t enum, game_interface_t, position_t), color macros
- `main/game_*.c` — One file per game, each implementing init/update/draw

**Game system:** Uses a dispatch table pattern (`game_interface_t`) where each game implements `init()`, `update(dx, dy, z)`, and `draw()` functions. Games are switched by flipping the board (z-axis detection). 16 games in order: Kitchen Timer, Maze, Snake, Breakout, Dodge, Catch, Tetris, Pong, Balance, Target Practice, Sokoban, Marble Race, Ball Trail, Glow Ball, Scroll Text, Animation. The Animation game cycles through 8 patterns: Spiral, Wave, Rainbow, Fire, Matrix Rain, Plasma, Game of Life, Fireworks.

**Display:** 8x8 matrix using GRB color format. Pixels are addressed via `set_pixel_at(row, col, g, r, b)`. The `ws2812_send()` function transmits the frame buffer.

## Adding a New Game

1. Create `main/game_yourgame.c` with `yourgame_init()`, `yourgame_update(dx, dy, z)`, `yourgame_draw()`
2. Add entry to `game_t` enum in `game_common.h` (before `GAME_COUNT`)
3. Add extern declarations and dispatch table entry in `main/main.c`
4. Add source file to `main/CMakeLists.txt` SRCS list
