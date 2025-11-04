// Espressif imports
#include <esp_log.h>

// Local imports
#include "include/rainbow.h"
#include "include/commons.h"

static const char *TAG = "RAINBOW";

/** Rainbow animation **/

/**
 * @brief Generate rainbow colors across 0-255 positions
 */
color_t wheel(uint8_t pos) {
    if (pos < 85) {
        return (color_t) {
            .red = 255 - pos * 3,
            .green = pos * 3,
            .blue = 0,
        };  // Red -> Green
    } else if (pos < 170) {
        pos -= 85;
        return (color_t) {
            .red = 0,
            .green = 255 - pos * 3,
            .blue = pos * 3,
        };  // Green -> Blue
    } else {
        pos -= 170;
        return (color_t) {
            .red = pos * 3,
            .green = 0,
            .blue = 255 - pos * 3,
        };  // Blue -> Red
    }
}


/**
 * @brief Entry point for a rainbow animation accros the planes
 */
void rainbow(led_strip_handle_t *led_strip) {
    uint8_t pos = 0;

    ESP_LOGI(TAG, "Animation: rainbow");

#ifndef PIO_QEMU_ENV
    ESP_ERROR_CHECK(led_strip_clear(*led_strip));
#endif

    // Bottom plane first
    for (uint8_t z = 0; z < SIDE_LENGTH; z++) {
        for (uint8_t y = 0; y < SIDE_LENGTH; y++) {
            for (uint8_t x = 0; x < SIDE_LENGTH; x++) {
                color_t color = wheel(pos * 256 / g_side3);
#ifndef PIO_QEMU_ENV
                led_strip_set_pixel(*led_strip, get_pix_id(x, y, z), color.red, color.green, color.blue);
                ESP_ERROR_CHECK(led_strip_refresh(*led_strip));
#endif
                ESP_LOGD(TAG, "px id: %d, red: %d, green: %d, blue: %d", get_pix_id(x, y, z), color.red, color.green, color.blue);
                pos++;

                if (g_button_pressed)
                    return;

                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
}
