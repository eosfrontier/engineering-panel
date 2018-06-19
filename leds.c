/*
 * Leds aansturen met rpi_ws2811 library
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "leds.h"
#include "../source/rpi_ws281x/ws2811.h"

#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                12
#define DMA                     10
#define STRIP_TYPE              WS2811_STRIP_GRB		// WS2812/SK6812RGB integrated chip+leds

ws2811_t ledstring =
{
    .freq = TARGET_FREQ,
    .dmanum = DMA,
    .channel =
    {
        [0] =
        {
            .gpionum = GPIO_PIN,
            .count = LED_COUNT,
            .invert = 0,
            .brightness = BRIGHTNESS,
            .strip_type = STRIP_TYPE,
        },
        [1] =
        {
            .gpionum = 0,
            .count = 0,
            .invert = 0,
            .brightness = 0,
        },
    },
};

int degrade(int color)
{
    int r = (color >> 16) & 0xff;
    int g = (color >> 8) & 0xff;
    int b = (color >> 0) & 0xff;
    r = r * 100 / 110;
    g = g * 100 / 110;
    b = b * 100 / 110;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    return (r << 16) + (g << 8) + b;
}

int init_leds(void)
{
    int ret;
    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS)
    {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        return ret;
    }
    for (int i = 0; i < LED_COUNT; i++) {
        ledstring.channel[0].leds[i] = 0;
    }
    ws2811_render(&ledstring);
    return 0;
}

int fini_leds(void)
{
    for (int i = 0; i < LED_COUNT; i++) {
        ledstring.channel[0].leds[i] = 0;
    }
    ws2811_render(&ledstring);
    ws2811_fini(&ledstring);
    return 0;
}

static int gstarts[] = GROUP_STARTS;
static int gdirs[] = GROUP_STARTS;
static int grings[] = GROUP_RINGS;

static int prevcolors[] = {0,0,0,0};
static int prevcorrect[] = {0,0,0,0};

unsigned int colstep(unsigned int from, unsigned int to)
{
    unsigned int rescol = 0;
    for (unsigned int c = 0; c < 3; c++) {
        unsigned int fcol = (from >> (c * 8)) & 0xff;
        unsigned int tcol = (to >> (c * 8)) & 0xff;
        if (tcol > fcol + CHSPEED) {
            fcol = fcol + CHSPEED;
        } else if (tcol + CHSPEED < fcol) {
            fcol = fcol - CHSPEED;
        } else {
            fcol = tcol;
        }
        rescol |= fcol << (c*8);
    }
    return rescol;
}

int ledshow_mastermind(int side, int colors, int correct)
{
    int ret;
    if (prevcolors[side] != colors || prevcorrect[side] != correct) {
        for (int g = side+2; g < NUM_GROUPS; g++) {
            prevcolors[g] = prevcolors[g-2];
            prevcorrect[g] = prevcorrect[g-2];
        }
        prevcolors[side] = colors;
        prevcorrect[side] = correct;
    }
    for (int g = side; g < NUM_GROUPS; g += 2) {
        int group_start = gstarts[g];
        int group_dir = gdirs[g];
        int group_ring = grings[g];
        for (int c = 0; c < NUM_COLORS; c++) {
            int pos = ((RING_SIZE + group_start + group_dir * c) % RING_SIZE) + group_ring;
            unsigned int curcol = ledstring.channel[0].leds[pos];
            if (c < prevcorrect[g]) {
                ledstring.channel[0].leds[pos] = colstep(curcol, POS_COLOR);
            } else if (c < (prevcorrect[g] + prevcolors[g])) {
                ledstring.channel[0].leds[pos] = colstep(curcol, COL_COLOR);
            } else {
                ledstring.channel[0].leds[pos] = colstep(curcol, NONE_COLOR);
            }
        }
    }
}

int leds_mainloop(void)
{
    int ret;
    if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
    {
        fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
        return ret;
    }
    return 0;
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
