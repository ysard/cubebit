// Espressif imports
#include <esp_log.h>

// Local imports
#include "include/random.h"
#include "include/commons.h"

static const char *TAG = "RANDOM";

/** Random animation **/

/**
 * @brief Entry point for the randomisation animation
 * Fade in / out LEDs, with random positions & colors.
 *
 * - Choose random coordinates (x, y, z)
 * - Choose random color
 * - Keep the choosen color in memory for the next draws
 * - Keep the number of draws for each LED
 *  (used to decide when fade in/out must be selected)
 *
 * Fade in/out process needs 5 draws each.
 * After the initial draw, for each draw, every channel value is
 * multiplied/divided by 2. Thus the initial value of a channel
 * should accept a multiplication by 2**5 (32) and still not overflow
 * the uint8_t max value (255).
 */
void randomisation(led_strip_handle_t *led_strip) {
    ESP_LOGI(TAG, "Animation: randomisation");

    // Init seed
    srand(esp_random());

#ifndef PIO_QEMU_ENV
    ESP_ERROR_CHECK(led_strip_clear(*led_strip));
#endif

    // Keep the number of draws for each LED
    uint8_t *shots = calloc(LED_STRIP_LED_COUNT, sizeof(int));
    if (!shots)
        return;

    color_t *colors = calloc(LED_STRIP_LED_COUNT, sizeof(color_t));
    if (!colors) {
        free(shots);
        return;
    }

    for (uint16_t draw = 0; draw < 16000; draw++) {
        // Choose coordinates: [0;4[
        uint8_t x = rand() % SIDE_LENGTH;
        uint8_t y = rand() % SIDE_LENGTH;
        uint8_t z = rand() % SIDE_LENGTH;

        uint8_t pos = get_pix_id(x, y, z);
        uint8_t shot = shots[pos];

        // Working cell color
        color_t *color = &colors[pos];

        ESP_LOGD(TAG, "px id: %d; shots: %d", pos, shot);

        // Shot == 0: initialize the channels
        // Shot <= 5: increase the brightness
        // Shot <= 10: decrease the brightness
        // Shot == 11: reset the channels
        if (shot == 0) {
            // Set color
            // 16 max divided by 2: 8max (try to eliminate small values)
            // Then followed by 5 multiplications by 2 : 255 max (keep color channels ratio)
            // 15: (15>>1)*2**5 = 224 max
            color->red   = (rand() % 17) >> 1; // 256 max
            color->green = (rand() % 17) >> 1; // 256 max
            color->blue  = (rand() % 14) >> 1; // 14: 192 max, 11: 160 max
        } else if (shot <= 5) {
            // Increase brightness
            ESP_LOGD(TAG, "px id: %d, red: %d, green: %d, blue: %d [BEFORE]", pos, color->red, color->green, color->blue);

            color->red = MIN_(224, color->red << 1);
            color->green = MIN_(224, color->green << 1);
            color->blue = MIN_(224, color->blue << 1);
        } else if (shot <= 10) {
            // Decrease brightness
            ESP_LOGD(TAG, "px id: %d, red: %d, green: %d, blue: %d [BEFORE]", pos, color->red, color->green, color->blue);

            color->red >>= 1;
            color->green >>= 1;
            color->blue >>= 1;
        } else {
            // Shutdown
            color->red = 0;
            color->green = 0;
            color->blue = 0;
        }

        if (shot == 11) {
            shots[pos] = 0;
        } else {
            shots[pos]++;
        }

#ifndef PIO_QEMU_ENV
        led_strip_set_pixel(*led_strip, pos, color->red, color->green, color->blue);
        ESP_ERROR_CHECK(led_strip_refresh(*led_strip));
#endif
        ESP_LOGD(TAG, "px id: %d, red: %d, green: %d, blue: %d", pos, color->red, color->green, color->blue);

        if (g_button_pressed)
            goto end;

        // Random delay between 2 draws
        uint16_t delay = rand() % 51;
        ESP_LOGD(TAG, "Wait: %dms", delay);
        vTaskDelay(pdMS_TO_TICKS(delay));
    }

    vTaskDelay(pdMS_TO_TICKS(2000));

end:
    free(shots);
    free(colors);
}
