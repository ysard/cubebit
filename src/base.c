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
 * @brief Basic animation
 */
// Espressif imports
#include <esp_log.h>

// Local imports
#include "include/base.h"
#include "include/commons.h"

static const char *TAG = "BASE";

/**
 * @brief Entry point for a progressive red line following the natural LED indexes
 */
void base(led_strip_handle_t *led_strip) {
    ESP_LOGI(TAG, "Animation: Basic red line");

    ESP_ERROR_CHECK(led_strip_clear(*led_strip));

    for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
        ESP_ERROR_CHECK(led_strip_set_pixel(*led_strip, i, 200, 0, 0));

        // Refresh the strip
        ESP_ERROR_CHECK(led_strip_refresh(*led_strip));

        ESP_LOGD(TAG, "idx: %d", i);

        vTaskDelay(pdMS_TO_TICKS(100));

        if (g_button_pressed)
            return;
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
}
