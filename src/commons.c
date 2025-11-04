#include "include/commons.h"

uint8_t g_cube[SIDE_LENGTH][SIDE_LENGTH][SIDE_LENGTH] = { 0 };

/**
 * @brief Convert (x,y,z) coords to the real index in the led strip
 */
uint8_t get_pix_id(uint8_t x, uint8_t y, uint8_t z) {
    uint8_t q = 0;

    if ((z % 2) == 0) {
        if ((y % 2) == 0) {
            q = y * SIDE_LENGTH + x;
        } else {
            q = y * SIDE_LENGTH + SIDE_LENGTH - 1 - x;
        }
    } else {
        if ((SIDE_LENGTH % 2) == 0)
            y = SIDE_LENGTH - y - 1;

        if ((x % 2) == 0)
            q = SIDE_LENGTH * (SIDE_LENGTH - x) - 1 - y;
        else
            q = (SIDE_LENGTH - 1 - x) * SIDE_LENGTH + y;
    }
    return z * g_side2 + q;
}
