// Copyright (C) 2025  Ysard
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/**
 * @brief Matrix raining code animation
 */
// Standard imports
#include <string.h>  // memset

// Espressif imports
#include <esp_log.h>

// Local imports
#include "include/matrix.h"
#include "include/commons.h"

static const char *TAG = "MATRIX";

enum matrix_green { MATRIX_ZERO, MATRIX_ONE, MATRIX_TWO, MATRIX_THREE, MATRIX_FOUR, MATRIX_FIVE, MATRIX_MAX, MATRIX_INVALID };
color_t matrix_colors[MATRIX_INVALID] = {
    {
        .red = 0,
        .green = 0,
        .blue = 0,
    },
    {
        .red = 0,  // 0x0D,
        .green = 0x01,
        .blue = 0x00,  // 0x08,  // Can't add anything other than green to avoid redish color
    },
    {
        .red = 0x00,
        .green = 0x03,  // Divide previous val by 4
        .blue = 0x00,
    },
    {
        .red = 0x00,
        .green = 0x0E,  // Divide previous val by 4
        .blue = 0x00,
    },
    {
        .red = 0x00,
        .green = 0x3B,
        .blue = 0x00,
    },
    {
        .red = 0x00,
        .green = 0x8F,
        .blue = 0x11,
    },
    {
        .red = 0x00,
        .green = 0xFF,
        .blue = 0x41,
    }
};


/**
 * @brief Apply raining code algorithm to the given column
 * Each color is defined by its unique id in the 3D array.
 * The colors gradually fade away on the lowest cell.
 */
void raining_code(led_strip_handle_t *led_strip, uint8_t col, uint8_t y) {
    uint8_t (*strand)[SIDE_LENGTH] = &g_cube[col][y];

    // Init new rain only if all cells of the strand are disabled
    bool activated_cells = false;
    for (uint8_t z = 0; z < SIDE_LENGTH; z++) {
        uint8_t color_idx = (*strand)[z];
        if (color_idx != 0) {
            activated_cells = true;
            break;
        }
    }

    if (!activated_cells) {
        // Enable the current (empty) strand with ~5% of chance
        // Enabling a strand consists of setting the maximum color to the top led of it
        uint8_t draw = (rand() % 101);
        if (draw > 5) {
            return;
        }

        (*strand)[SIDE_LENGTH - 1] = MATRIX_MAX;
        ESP_LOGD(TAG, "Rain enabled: x: %d, y: %d", col, y);

        // Set the color immediately
        color_t color = matrix_colors[MATRIX_MAX];
        // ESP_LOGD(TAG, "Rain enabled: red: %d, green: %d; blue: %d", color.red, color.green, color.blue);
#ifndef PIO_QEMU_ENV
        led_strip_set_pixel(*led_strip, get_pix_id(col, y, SIDE_LENGTH - 1), color.red, color.green, color.blue);
#endif
        return;
    } else {
        ESP_LOGD(TAG, "Rain already enabled: x: %d, y: %d", col, y);
    }


    // Reduce luminosity of all cells & search the current max color position
    uint8_t max_pos = 0;
    for (uint8_t z = 0; z < SIDE_LENGTH; z++) {
        uint8_t *color_idx = &(*strand)[z];

        if (*color_idx == MATRIX_MAX) {
            max_pos = z;
        }
        *color_idx = MAX_(0, (*color_idx) - 1);
    }

    // Move max luminosity on the cell below the old position (if found)
    if (max_pos > 0) {
        (*strand)[max_pos - 1] = MATRIX_MAX;
    }

    // Show pixels
    for (uint8_t z = 0; z < SIDE_LENGTH; z++) {
        uint8_t color_idx = (*strand)[z];
        color_t color = matrix_colors[color_idx];
#ifndef PIO_QEMU_ENV
        led_strip_set_pixel(*led_strip, get_pix_id(col, y, z), color.red, color.green, color.blue);
#endif
    }
}


/**
 * @brief Entry point for the Matrix raining code effect
 */
void matrix(led_strip_handle_t *led_strip) {
    ESP_LOGI(TAG, "Animation: matrix");

#ifndef PIO_QEMU_ENV
    ESP_ERROR_CHECK(led_strip_clear(*led_strip));
#endif

    // Clear buffer
    memset(g_cube, 0, sizeof(uint8_t) * SIDE_LENGTH * SIDE_LENGTH * SIDE_LENGTH);

    // Seed rand
    srand(esp_random());

    while (1) {
        for (uint8_t y = 0; y < SIDE_LENGTH; y++) {
            for (uint8_t col = 0; col < SIDE_LENGTH; col++) {
                raining_code(led_strip, col, y);
                // ESP_LOGI(TAG, "end strand");
            }
        }

#ifndef PIO_QEMU_ENV
        ESP_ERROR_CHECK(led_strip_refresh(*led_strip));
#endif

        if (g_button_pressed)
            break;

        vTaskDelay(pdMS_TO_TICKS(150));
    }
}
