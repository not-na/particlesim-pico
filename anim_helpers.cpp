#include "anim_helpers.h"

void gl_fillscreen(uint32_t color) {
    gl_fillrect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, color);
}

void gl_fillrect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (int yc = y; yc < y + h; ++yc) {
        for (int xc = x; xc < x + w; ++xc) {
            gl_pixel(xc, yc, color);
        }
    }
}

inline void gl_pixel(uint32_t x, uint32_t y, uint32_t color) {
    anim_framebuf[x+y*DISPLAY_WIDTH] = color;
}


// From Adafruit_NeoPixel
/*!
  @brief   Convert hue, saturation and value into a packed 32-bit RGB color
           that can be passed to setPixelColor() or other RGB-compatible
           functions.
  @param   hue  An unsigned 16-bit value, 0 to 65535, representing one full
                loop of the color wheel, which allows 16-bit hues to "roll
                over" while still doing the expected thing (and allowing
                more precision than the wheel() function that was common to
                prior NeoPixel examples).
  @param   sat  Saturation, 8-bit value, 0 (min or pure grayscale) to 255
                (max or pure hue). Default of 255 if unspecified.
  @param   val  Value (brightness), 8-bit value, 0 (min / black / off) to
                255 (max or full brightness). Default of 255 if unspecified.
  @return  Packed 32-bit RGB with the most significant byte set to 0 -- the
           white element of WRGB pixels is NOT utilized. Result is linearly
           but not perceptually correct, so you may want to pass the result
           through the gamma32() function (or your own gamma-correction
           operation) else colors may appear washed out. This is not done
           automatically by this function because coders may desire a more
           refined gamma-correction function than the simplified
           one-size-fits-all operation of gamma32(). Diffusing the LEDs also
           really seems to help when using low-saturation colors.
*/
uint32_t gl_color_hsv_nogamma(uint16_t hue, uint8_t sat, uint8_t val) {

    uint8_t r, g, b;

    // Remap 0-65535 to 0-1529. Pure red is CENTERED on the 64K rollover;
    // 0 is not the start of pure red, but the midpoint...a few values above
    // zero and a few below 65536 all yield pure red (similarly, 32768 is the
    // midpoint, not start, of pure cyan). The 8-bit RGB hexcone (256 values
    // each for red, green, blue) really only allows for 1530 distinct hues
    // (not 1536, more on that below), but the full unsigned 16-bit type was
    // chosen for hue so that one's code can easily handle a contiguous color
    // wheel by allowing hue to roll over in either direction.
    hue = (hue * 1530L + 32768) / 65536;
    // Because red is centered on the rollover point (the +32768 above,
    // essentially a fixed-point +0.5), the above actually yields 0 to 1530,
    // where 0 and 1530 would yield the same thing. Rather than apply a
    // costly modulo operator, 1530 is handled as a special case below.

    // So you'd think that the color "hexcone" (the thing that ramps from
    // pure red, to pure yellow, to pure green and so forth back to red,
    // yielding six slices), and with each color component having 256
    // possible values (0-255), might have 1536 possible items (6*256),
    // but in reality there's 1530. This is because the last element in
    // each 256-element slice is equal to the first element of the next
    // slice, and keeping those in there this would create small
    // discontinuities in the color wheel. So the last element of each
    // slice is dropped...we regard only elements 0-254, with item 255
    // being picked up as element 0 of the next slice. Like this:
    // Red to not-quite-pure-yellow is:        255,   0, 0 to 255, 254,   0
    // Pure yellow to not-quite-pure-green is: 255, 255, 0 to   1, 255,   0
    // Pure green to not-quite-pure-cyan is:     0, 255, 0 to   0, 255, 254
    // and so forth. Hence, 1530 distinct hues (0 to 1529), and hence why
    // the constants below are not the multiples of 256 you might expect.

    // Convert hue to R,G,B (nested ifs faster than divide+mod+switch):
    if(hue < 510) {         // Red to Green-1
        b = 0;
        if(hue < 255) {       //   Red to Yellow-1
            r = 255;
            g = hue;            //     g = 0 to 254
        } else {              //   Yellow to Green-1
            r = 510 - hue;      //     r = 255 to 1
            g = 255;
        }
    } else if(hue < 1020) { // Green to Blue-1
        r = 0;
        if(hue <  765) {      //   Green to Cyan-1
            g = 255;
            b = hue - 510;      //     b = 0 to 254
        } else {              //   Cyan to Blue-1
            g = 1020 - hue;     //     g = 255 to 1
            b = 255;
        }
    } else if(hue < 1530) { // Blue to Red-1
        g = 0;
        if(hue < 1275) {      //   Blue to Magenta-1
            r = hue - 1020;     //     r = 0 to 254
            b = 255;
        } else {              //   Magenta to Red-1
            r = 255;
            b = 1530 - hue;     //     b = 255 to 1
        }
    } else {                // Last 0.5 Red (quicker than % operator)
        r = 255;
        g = b = 0;
    }

    // Apply saturation and value to R,G,B, pack into 32-bit result:
    uint32_t v1 =   1 + val; // 1 to 256; allows >>8 instead of /255
    uint16_t s1 =   1 + sat; // 1 to 256; same reason
    uint8_t  s2 = 255 - sat; // 255 to 0
    return ((((((r * s1) >> 8) + s2) * v1) & 0xff00) << 8) |
           (((((g * s1) >> 8) + s2) * v1) & 0xff00)       |
           ( ((((b * s1) >> 8) + s2) * v1)           >> 8);
}