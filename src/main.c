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
// Standard imports
#include <stdio.h>
#include <stdlib.h>

// FreeRTOS imports
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Espressif imports
#include <esp_log.h>
#include <driver/gpio.h>
#include <rom/gpio.h>  // gpio_output_set
#include <driver/rmt_tx.h>

#include "led_strip.h"

// Local imports
#include "include/commons.h"
#include "include/base.h"
#include "include/rainbow.h"
#include "include/random.h"
#include "include/fire.h"
#include "include/matrix.h"


/** RMT / SPI driver configuration **/

#if LED_STRIP_USE_DMA
// Numbers of the LED in the strip
// #define LED_STRIP_LED_COUNT 256
#define LED_STRIP_MEMORY_BLOCK_WORDS    1024 // this determines the DMA block size
#else
// Numbers of the LED in the strip
// #define LED_STRIP_LED_COUNT 24
// let the driver choose a proper memory block size automatically
// should be at least 64
#define LED_STRIP_MEMORY_BLOCK_WORDS    0
#endif  // LED_STRIP_USE_DMA

// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ    (10 * 1000 * 1000)

bool g_button_pressed = false;
uint8_t g_side2;
uint8_t g_side3;

static const char *TAG = "LED_CUBE";

/**
 * @brief ISR for a BOOT button status change
 */
void IRAM_ATTR isr_handler(void *arg) {
    if (g_button_pressed)
        return;

    g_button_pressed = true;

    // Will get set to pdTRUE inside the interrupt safe
    // API function if a context switch is required.
    // BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Notification of an incoming event
    // vTaskNotifyGiveFromISR(xTaskHandle, &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
     * is performed to ensure the interrupt returns directly to the highest
     * priority task. */
    // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/**
 * @brief Configure the LED driver via the RMT (Remote Control Transceiver) peripheral
 * @return LED strip object
 */
led_strip_handle_t configure_led_rmt(void) {
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num         = LED_STRIP_GPIO,                    // The GPIO that connected to the LED strip's data line
        .max_leds               = LED_STRIP_LED_COUNT,               // The number of LEDs in the strip,
        .led_model              = LED_MODEL_WS2812,                  // LED strip model LED_MODEL_SK6812, LED_MODEL_WS2811, LED_MODEL_WS2812
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // Don't invert the output signal
        }
    };

    // LED strip backend configuration: RMT
    // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/rmt.html#install-rmt-tx-channel
    led_strip_rmt_config_t rmt_config = {
        .clk_src           = RMT_CLK_SRC_DEFAULT,          // Different clock source can lead to different power consumption
        .resolution_hz     = LED_STRIP_RMT_RES_HZ,         // RMT counter clock frequency
        .mem_block_symbols = LED_STRIP_MEMORY_BLOCK_WORDS, // The memory block size used by the RMT channel
        .flags = {
            // Only one RMT channel can use the DMA feature, and this is a hardware limitation.
            // Thus, only one led_strip handle can utilize the DMA ability.
            .with_dma = LED_STRIP_USE_DMA, // Using DMA can improve performance when driving more LEDs
        }
    };

    // Create the LED strip object
    led_strip_handle_t led_strip = NULL;

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return led_strip;
}


/**
 * @brief Configure the LED driver via the SPI master driver
 * @return LED strip object
 */
led_strip_handle_t configure_led_spi(void) {
    // LED strip common configuration
    led_strip_config_t strip_config = {
        .strip_gpio_num         = LED_STRIP_GPIO,                    // The GPIO that connected to the LED strip's data line
        .max_leds               = LED_STRIP_LED_COUNT,               // The number of LEDs in the strip,
        .led_model              = LED_MODEL_WS2812,                  // LED strip model, it determines the bit timing
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color component format is GRB
        .flags ={
            .invert_out = false, // Don't invert the output signal
        }
    };

    // SPI backend specific configuration
    led_strip_spi_config_t spi_config = {
        .clk_src = SPI_CLK_SRC_DEFAULT, // Different clock source can lead to different power consumption
        .spi_bus = SPI2_HOST,           // SPI bus ID
        .flags = {
            .with_dma = true,  // Using DMA can improve performance and help drive more LEDs
        }
    };

    // Create the LED strip object
    led_strip_handle_t led_strip = NULL;

    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with SPI backend");
    return led_strip;
}


/**
 * @brief Use Boot button to change the scenario
 */
void configure_button(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_9),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,  // React on falling edge
    };

    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_9, isr_handler, NULL);
}


/**
 * @brief Test GPIOS of the ESP32c6 nano board
 * (Chinese copy of the devkit with modified pinout & components)
 *
 * Exluded pins:
 *   USB pins:
 *   12, 13
 *   UART:
 *   16, 17
 *   BOOT:
 *   9
 *   ??:
 *   14
 *   SDIO:
 *   18-23
 */
void test_gpios(void) {
    uint64_t gpio_mask = 0;

    for (int i = 0; i < 16; i++) {
        if ((i == 12) || (i == 13) || (i == 9) || (i == 14))
            continue;

        gpio_mask |= (1ULL << i);
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = gpio_mask,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    gpio_config(&io_conf);

    // 10 tests, 5 secs each
    bool level = false;
    for (uint8_t i = 0; i < 10; i++) {
        level = !level;

        ESP_LOGI(TAG, "GPIO: %s", level ? "HIGH" : "LOW");

        if (level) {
            gpio_output_set(gpio_mask, 0, 0, 0);  // HIGH
        } else {
            gpio_output_set(0, gpio_mask, 0, 0);  // LOW
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


void app_main(void) {
    configure_button();

    g_side2 = SIDE_LENGTH * SIDE_LENGTH;
    g_side3 = g_side2 * SIDE_LENGTH;

    ESP_LOGI(TAG, "Initialisation of the LED cube driver...");
    led_strip_handle_t led_strip = configure_led_rmt();
    // led_strip_handle_t led_strip = configure_led_spi();

    uint8_t scenario = 4;
    while (1) {
        ESP_LOGI(TAG, "scenario: %d", scenario);

        switch (scenario) {
            case 0:
                base(&led_strip);
                break;

            case 1:
                rainbow(&led_strip);
                break;

            case 2:
                randomisation(&led_strip);
                break;

            case 3:
                // Red fire
                fire(&led_strip, true);
                break;

            case 4:
                // Green fire
                fire(&led_strip, false);
                break;

            case 5:
                matrix(&led_strip);
                break;

            default:
                scenario = 0;
        }

        if (g_button_pressed) {
            g_button_pressed = false;
            scenario++;
        }
    }
}
