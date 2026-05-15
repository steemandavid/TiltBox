#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "game_common.h"
#include "display.h"
#include "accel.h"

extern void scroll_text_init(void);
extern void scroll_text_update(int16_t dx, int16_t dy, int16_t z);
extern void scroll_text_draw(void);

void app_main(void)
{
    printf("Scroll Text starting...\n");

    ws2812_init();
    i2c_init();

    clear_display();
    ws2812_send();

    scroll_text_init();

    while (1) {
        int16_t x, y, z;

        if (adxl345_read(&x, &y, &z) == ESP_OK) {
            scroll_text_update(x, y, z);
        }

        scroll_text_draw();
        ws2812_send();

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
