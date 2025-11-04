#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdbool.h>
#include <inttypes.h>

#include <esp_random.h>

/** User configuration variables **/

#define LED_STRIP_GPIO GPIO_NUM_8   // GPIO connected to the WS2812
#define LED_STRIP_LED_COUNT 64      // Total number of LEDs

// Set to 1 to use DMA for driving the LED strip, 0 otherwise
// Please note the RMT DMA feature is only available on chips e.g. ESP32-S3/P4
// => not on C6
#define LED_STRIP_USE_DMA  0

/** Misc **/
#define MAX_(a, b)    (((a) > (b)) ? (a) : (b))
#define MIN_(a, b)    (((a) < (b)) ? (a) : (b))

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} color_t;

uint8_t get_pix_id(uint8_t x, uint8_t y, uint8_t z);

/** Global settings **/
#define SIDE_LENGTH    4

extern uint8_t g_side2;
extern uint8_t g_side3;

extern bool g_button_pressed;

// 2D
// uint8_t g_cube[SIDE_LENGTH][SIDE_LENGTH];
// 3D
extern uint8_t g_cube[SIDE_LENGTH][SIDE_LENGTH][SIDE_LENGTH];

#endif // __COMMON_H__
