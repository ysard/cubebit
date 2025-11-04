// Standard imports
#include <string.h>  // memset

// Espressif imports
#include <esp_log.h>

// Local imports
#include "include/fire.h"
#include "include/commons.h"

static const char *TAG = "FIRE";

/** Fire animation **/

#define MAX_GREEN       70  // The max green value for red flames, higher = more yellow
#define MAX_RED         210 // The max red for green flames, higher = more yellow
#define MIN_COOLING     80  // Higher values of cooling lead to more 'flicker' and more 'gaps' in the flame
#define MAX_COOLING     220 // Wider range in values leads to more variation
#define MIN_SPARKING    100 // Sparking leads to a flame which progresses up the strip, more sparks=more flames
#define MAX_SPARKING    150 // Wider range in values leads to more variation
#define FRAME_DELAY     20  // The millisecond delay for each frame, 50ms = 20 FPS


/**
 * @brief Get the color of the selected pixel in the selected strand according to the heat value
 */
color_t get_pixel_heat_color(uint8_t strand, int z, uint8_t heat_value, bool red_flames) {
    // ESP_LOGI(TAG, "strand: %d, z: %d, heat: %d", strand, z, heat_value);

    // Scale 'heat' down from 0-255 to 0-191
    uint8_t t192 = (heat_value * 191) / 255;

    // ESP_LOGI(TAG, "t192: %d", t192);

    uint8_t red;
    uint8_t green;

    if (red_flames) {
        // Adjust red and green components to create a gradient from red at the bottom to yellow at the top
        red = 255;
        // Gradually increase green from 0 to MAX_GREEN
        // Transpose the position z [0; STRAND_HEIGHT[ to the interval [0;MAX_GREEN]
        green = ((MAX_GREEN + 95) * z) / SIDE_LENGTH;
    } else {
        // Adjust red and green components to create a gradient from green at the bottom to yellow at the top
        green = 200;  // This a very dominant channel: reduce it
        // Gradually increase red from 0 to MAX_RED
        // Transpose the position z [0; STRAND_HEIGHT[ to the interval [0;MAX_GREEN]
        red = ((MAX_RED) * z) / SIDE_LENGTH;
    }

    // Use the temperature to influence the brightness/intensity
    // ESP_LOGI(TAG, "red: %d, green: %d [BEFORE]", red, green);
    red = (red * t192) / 255;
    green = (green * t192) / 255;
    // ESP_LOGI(TAG, "red: %d, green: %d", red, green);

    color_t color = (color_t){
        .red   = red,
        .green = green,
        .blue  = 0,
    };

    return color;
}


/**
 * @brief Apply the fire effect on the given column
 */
void column_fire(led_strip_handle_t *led_strip, uint8_t col, uint8_t y, bool red_flames) {
    // Cooling & sparking limits for the current strand
    uint8_t cooling = (rand() % (255 - MAX_COOLING + 1)) + MIN_COOLING;
    uint8_t sparking = (rand() % (255 - MAX_SPARKING + 1)) + MIN_SPARKING;

    // Working strand
    uint8_t (*strand)[SIDE_LENGTH] = &g_cube[col][y];

    // Cool down every cell a little
    for (uint8_t z = 0; z < SIDE_LENGTH; z++) {
        // 0;552 ... ???? TODO uint8_t
        // Cooling * 10 ?
        (*strand)[z] = MAX_(0, (*strand)[z] - (rand() % ((cooling) / SIDE_LENGTH) + 2));
    }

    // Heat from each cell drifts 'up' and diffuses a little
    for (uint8_t z = SIDE_LENGTH - 1; z >= 2; z--) {
        // ponderation ? Twice for the cell 2 cells below ? TODO
        (*strand)[z] = ((*strand)[z - 1] + (*strand)[z - 2] + (*strand)[z - 2]) / 3;
    }

    // Randomly ignite new 'sparks' near the bottom (2 first z-index)
    if ((rand() % 256) < sparking) {
        uint8_t z = rand() % 2;
        uint8_t heat_value = (*strand)[z];
        // ESP_LOGI(TAG, "heat: %d [BEFORE]", heat_value);
        heat_value = (rand() % (255 - heat_value + 1)) + heat_value;
        // ESP_LOGI(TAG, "heat: %d", heat_value);
        (*strand)[z] = heat_value;
    }

    // Step 4. Convert heat to color and set pixels
    for (uint8_t z = 0; z < SIDE_LENGTH; z++) {
        color_t color = get_pixel_heat_color(col, z, (*strand)[z], red_flames);

#ifndef PIO_QEMU_ENV
        led_strip_set_pixel(*led_strip, get_pix_id(col, y, z), color.red, color.green, color.blue);
#endif
    }
}


/**
 * @brief Entry point for the fire animation
 *
 * Inspired from https://www.hauntforum.com/threads/chatgpt-and-i-design-a-flicker-fire-effect-for-arduino-and-neopixels.48028/
 * Barely works...
 */
void fire(led_strip_handle_t *led_strip, bool red_flames) {
    ESP_LOGI(TAG, "Animation: fire");

    // Clear buffer
    memset(g_cube, 0, sizeof(uint8_t) * SIDE_LENGTH * SIDE_LENGTH * SIDE_LENGTH);

#ifndef PIO_QEMU_ENV
    ESP_ERROR_CHECK(led_strip_clear(*led_strip));
#endif

    while (1) {
        for (uint8_t y = 0; y < SIDE_LENGTH; y++) {
            for (uint8_t col = 0; col < SIDE_LENGTH; col++) {
                column_fire(led_strip, col, y, red_flames);
                // ESP_LOGI(TAG, "end strand");
            }
        }

#ifndef PIO_QEMU_ENV
        ESP_ERROR_CHECK(led_strip_refresh(*led_strip));
#endif

        if (g_button_pressed)
            break;

        vTaskDelay(pdMS_TO_TICKS(FRAME_DELAY));
    }
}
