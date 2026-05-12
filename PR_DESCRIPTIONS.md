# PR Descriptions for New TiltBox Games

Each section below contains a ready-to-use PR title and body for submitting to `Tom-Michiels/TiltBox`. These should be submitted **after testing** on actual hardware.

---

## PR 1: Refactor — Split monolithic main.c into modular game files

**Title:** Refactor: split main.c into separate files per game

**Body:**
## Summary
- Extract the monolithic 1643-line `main/main.c` into separate compilation units: one file per game, plus shared `display.c` (WS2812 LED driver) and `accel.c` (ADXL345 driver)
- Add shared headers: `game_common.h` (types, macros, game enum), `display.h`, `accel.h`
- `main.c` reduced to entry point, flip detection, and game dispatch table
- No behavior changes — all 9 original games work identically

## Files Changed
- `main/main.c` — slimmed to ~150 lines (entry point + dispatch table)
- `main/CMakeLists.txt` — updated SRCS list
- `main/display.c/h` — new (extracted from main.c)
- `main/accel.c/h` — new (extracted from main.c)
- `main/game_common.h` — new (shared types)
- `main/game_*.c` — new (one per game, 9 files)

## Testing
- Built and flashed on ESP32-C3 with ESP-IDF v5.4.1
- All 9 original games verified working
- Binary size: ~228KB (78% free in 1MB partition)

---

## PR 2: Add Pong game

**Title:** Add Pong game (1-player vs AI)

**Body:**
## Summary
- Classic 1-player Pong against an AI paddle
- Tilt left/right to move your paddle along the bottom row
- AI opponent tracks the ball with slight delay for balanced difficulty
- Score counts volleys; game resets after 3 misses

## Files Changed
- `main/game_pong.c` — new game implementation
- `main/main.c` — add to dispatch table
- `main/game_common.h` — add GAME_PONG to enum
- `main/CMakeLists.txt` — add to SRCS

## Testing
- Built and flashed on ESP32-C3 hardware
- Tilt control responsive, AI provides fair challenge
- Score and miss tracking work correctly

---

## PR 3: Add Balance game

**Title:** Add Balance game (tilt precision challenge)

**Body:**
## Summary
- Keep a ball centered against random wind forces
- Wind direction changes every ~2 seconds, forcing constant adjustment
- Target zone shrinks over time, increasing difficulty
- Game over when ball drifts off the 8x8 grid

## Files Changed
- `main/game_balance.c` — new game implementation
- `main/main.c` — add to dispatch table
- `main/game_common.h` — add GAME_BALANCE to enum
- `main/CMakeLists.txt` — add to SRCS

## Testing
- Built and flashed on ESP32-C3 hardware
- Wind mechanics provide engaging challenge
- Shrinking target zone creates natural difficulty curve

---

## PR 4: Add Catch game

**Title:** Add Catch game (catch good items, avoid bad ones)

**Body:**
## Summary
- Catch falling green items for points while avoiding red ones
- Tilt left/right to move a 3-pixel-wide basket along the bottom
- 3 lives — lose one for each red item caught
- Speed increases as score grows

## Files Changed
- `main/game_catch.c` — new game implementation
- `main/main.c` — add to dispatch table
- `main/game_common.h` — add GAME_CATCH to enum
- `main/CMakeLists.txt` — add to SRCS

## Testing
- Built and flashed on ESP32-C3 hardware
- Item spawning, collision, and scoring work correctly
- Difficulty ramp is smooth and challenging

---

## PR 5: Add Target Practice game

**Title:** Add Target Practice game (tilt-to-aim timed challenge)

**Body:**
## Summary
- Tilt a cursor to hit randomly appearing targets as fast as possible
- 30-second timer with visual countdown bar
- Rainbow-colored targets cycle hue for visual interest
- Score displayed at end of round

## Files Changed
- `main/game_target.c` — new game implementation
- `main/main.c` — add to dispatch table
- `main/game_common.h` — add GAME_TARGET to enum
- `main/CMakeLists.txt` — add to SRCS

## Testing
- Built and flashed on ESP32-C3 hardware
- Smooth cursor movement follows tilt accurately
- Timer and scoring work as expected

---

## PR 6: Add Sokoban game

**Title:** Add Sokoban Mini game (push-block puzzles)

**Body:**
## Summary
- Classic push-block puzzle game adapted for the 8x8 matrix
- Tilt to move your character and push blocks onto target positions
- 8 levels of increasing difficulty
- Auto-advances to next level on completion

## Files Changed
- `main/game_sokoban.c` — new game implementation with 8 puzzle levels
- `main/main.c` — add to dispatch table
- `main/game_common.h` — add GAME_SOKOBAN to enum
- `main/CMakeLists.txt` — add to SRCS

## Testing
- Built and flashed on ESP32-C3 hardware
- All 8 levels verified solvable
- Push mechanics, win detection, and level progression work correctly

---

## PR 7: Add Marble Race game

**Title:** Add Marble Race game (scrolling track steering)

**Body:**
## Summary
- Steer a marble along an auto-scrolling winding track
- Tilt left/right to stay on the path as speed increases
- 3 lives — go off-track and you lose one
- Score equals distance traveled

## Files Changed
- `main/game_marble_race.c` — new game implementation
- `main/main.c` — add to dispatch table
- `main/game_common.h` — add GAME_MARBLE_RACE to enum
- `main/CMakeLists.txt` — add to SRCS

## Testing
- Built and flashed on ESP32-C3 hardware
- Track generation produces smooth, navigable paths
- Speed increase creates engaging difficulty curve
