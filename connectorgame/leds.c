/*
 * Leds aansturen met rpi_ws2811 library
 */

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include "main.h"
#include "leds.h"
#include "../../source/rpi_ws281x/ws2811.h"

enum led_animation_types {
    ANIMATION_PLASMA,
    ANIMATION_FLASH,
    ANIMATION_SWIPE,
    ANIMATION_IDLE,
    ANIMATION_SPIN,
    ANIMATION_COLORS,
    ANIMATION_BLANK,
};

#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                12
#define DMA                     10
#define STRIP_TYPE              WS2811_STRIP_GRB		// WS2812/SK6812RGB integrated chip+leds

#define COLOR_FADE (FRAMERATE/SCANRATE)

static ws2811_t ledstring =
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

typedef struct ledanim {
    struct ledanim *next;
    int type;
    int offset;
    int size;
    int speed;
    int fadein;
    int fadepos;
    int pos;
    int data[0];
} ledanim_t;

static ledanim_t *led_animations = NULL;

static int col_fade(double val, int num, ...)
{
    unsigned int col1, col2;
    if (val < 0.0) val = 0.0;
    if (val > 1.0) val = 1.0;
    val *= (num-1);
    va_list argp;
    va_start(argp, num);
    while (val > 1.0) {
        unsigned int dummy = va_arg(argp, unsigned int);
        val -= 1.0;
    }
    col1 = va_arg(argp, unsigned int);
    col2 = va_arg(argp, unsigned int);
    va_end(argp);
    int col = 0;
    for (int m = 0; m < 24; m += 8) {
        double cp1 = (double)((col1 >> m) & 0xff);
        double cp2 = (double)((col2 >> m) & 0xff);
        int cp = (int)((cp2 * val) + (cp1 * (1.0 - val)));
        col |= cp << m;
    }
    return col;
}

static int scale(int color, int value)
{
    int r = (color >> 16) & 0xff;
    int g = (color >> 8) & 0xff;
    int b = (color >> 0) & 0xff;
    r = r * value / 255;
    g = g * value / 255;
    b = b * value / 255;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
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
static int gdirs[]   = GROUP_DIRS;
static int gblanks[] = GROUP_BLANKS;
static int grings[]  = GROUP_RINGS;

static unsigned int colstep(unsigned int from, unsigned int to)
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

static int blinkstep = 0;
static unsigned int colorlist[] = {
    BLACK_COLOR,
    BLUE_COLOR,
    GREEN_COLOR,
    YELLOW_COLOR,
    RED_COLOR,
    GOOD_COLOR,
    BAD_COLOR
};

int led_set_colors(int *colors)
{
    ledanim_t *newan = NULL, **an;
    /* Bestaande anim zoeken */
    for (an = &led_animations; *an;) {
        if ((*an)->type == ANIMATION_COLORS) {
            newan = *an;
            if (newan->next) {
                /* Altijd naar het einde van de lijst zetten */
                *an = newan->next;
                newan->next = NULL;
                while (*an) {
                    an = &((*an)->next);
                }
                *an = newan;
            }
            break;
        } else {
            an = &((*an)->next);
        }
    }
    if (!newan) {
        newan = malloc(sizeof(ledanim_t) + (2*NUM_COLORS*NUM_GROUPS+1)*sizeof(int));
        if (!newan) {
            fprintf(stderr, "Allocation for animation failed!\n");
            return -1;
        }
        newan->next = NULL;
        newan->type = ANIMATION_COLORS;
        newan->offset = RING_SIZE;
        newan->size = RING_SIZE*2;
        newan->fadein = COLOR_FADE;
        newan->fadepos = 0;
        newan->pos = 0;
        newan->data[0] = NUM_COLORS*NUM_GROUPS;
        for (int i = 0; i < NUM_COLORS*NUM_GROUPS; i++) {
            newan->data[2*i+2] = 0;
        }
        *an = newan;
    }
    if (colors) {
        for (int i = 0; i < NUM_COLORS*NUM_GROUPS; i++) {
            newan->data[2*i+1] = colors[i];
        }
        newan->fadein = COLOR_FADE;
    } else {
        newan->fadein = 0;
    }
    return 0;
}

static int led_animate_colors(ledanim_t *an)
{
    int blinklist[7];
    if (an->fadepos < an->fadein) an->fadepos += 1;
    if (an->fadepos > an->fadein) an->fadepos -= 1;
    if (an->fadepos <= 0) return 0;
    if (++(an->pos) >= 60) {
        an->pos = 0;
    }
    for (int g = 0; g < NUM_GROUPS; g++) {
        int group_start = gstarts[g];
        int group_dir = gdirs[g];
        int group_ring = grings[g];
        for (int c = 0; c < NUM_COLORS; c++) {
            int pos = ((RING_SIZE + group_start + group_dir * c) % RING_SIZE) + group_ring;
            unsigned int curcol = an->data[(g*10+c)*2+2];
            unsigned int newcol = 0x00;
            int thecol = an->data[(g*10+c)*2+1];
            int colcnt = 0;
            for (int l = 0; l < 7; l++) {
                if (thecol & (1 << l)) {
                    blinklist[colcnt++] = colorlist[l];
                }
            }
            if (colcnt > 0) {
                newcol = blinklist[(an->pos * colcnt)/60];
            }
            newcol = colstep(curcol, newcol);
            an->data[(g*10+c)*2+2] = newcol;
            ledstring.channel[0].leds[pos] = col_fade((double)an->fadepos / COLOR_FADE, 2, ledstring.channel[0].leds[pos], newcol);
        }
    }
    for (unsigned int b = 0; b < sizeof(gblanks)/sizeof(gblanks[0]); b++) {
        ledstring.channel[0].leds[gblanks[b]] = col_fade((double)an->fadepos / COLOR_FADE, 2, ledstring.channel[0].leds[gblanks[b]], 0);
    }
    return 0;
}

static int prevcolors[] = {0,0,0,0};
static int prevcorrect[] = {0,0,0,0};

int ledshow_mastermind(int side, int colors, int correct)
{
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
    return 0;
}

static int led_animate_plasma(ledanim_t *an)
{
    an->pos += 1;
    if (an->pos > (an->speed * an->size)) an->pos = 0;
    if (an->fadepos < an->fadein) an->fadepos += 1;
    int fade = 255;
    if (an->fadein > 0) fade = (255 * an->fadepos) / an->fadein;
    for (int ip = 0; ip < an->size; ip++) {
        double r = 0, g = 0, b = 0;
        for (int dp = 1; dp < an->data[0]; dp += 3) {
            unsigned int dcol = an->data[dp];
            double dr = (double)((dcol >> 16) & 0xff) / 4.0;
            double dg = (double)((dcol >>  8) & 0xff) / 4.0;
            double db = (double)((dcol      ) & 0xff) / 4.0;
            double ddis = fmod((an->data[dp+1] + (((double)an->pos / an->speed) * (an->data[dp+2])) + (an->size * an->size) - ip), (double)an->size);
            if (ddis >= (an->size/2)) ddis = an->size - ddis;
            r += fmax(floor(dr - (fmax(ddis-1.0,0.001)*(3*dr/an->size))), 0);
            g += fmax(floor(dg - (fmax(ddis-1.0,0.001)*(3*dg/an->size))), 0);
            b += fmax(floor(db - (fmax(ddis-1.0,0.001)*(3*db/an->size))), 0);
        }
        if (r < 0) r = 0;
        if (g < 0) g = 0;
        if (b < 0) b = 0;
        if (r > 0xff) r = 0xff;
        if (g > 0xff) g = 0xff;
        if (b > 0xff) b = 0xff;
        int col = (((int)r) << 16) | (((int)g) << 8) | ((int)b);
        ledstring.channel[0].leds[an->offset+ip] = scale(col, fade);
    }
    return 0;
}

static int led_animate_flash(ledanim_t *an)
{
    an->pos += 1;
    int dpos = 0;
    for (int dp = 1; dp < an->data[0]; dp += 3) {
        dpos += an->data[dp+0];
        if (an->pos <= dpos) return 0;
        dpos += an->data[dp+1];
        int col = an->data[dp+2];
        if (an->pos <= dpos) {
            for (int ip = 0; ip < an->size; ip++) {
                ledstring.channel[0].leds[an->offset+ip] = col;
            }
            return 0;
        }
    }
    return 1;
}

static int led_animate_swipe(ledanim_t *an)
{
    an->pos += 1;
    int spos, onoff;

    if (an->pos <= an->speed/2) {
        onoff = 0;
        spos = an->pos;
    } else if (an->pos <= an->speed) {
        onoff = 1;
        spos = an->pos - an->speed/2;
    } else {
        return 1;
    }
    for (int dp = 2; dp < an->data[0]; dp++) {
        int p1 = (an->size * (dp-2)) / (an->data[0]-2) + an->data[1];
        int p2 = (an->size * (dp-1)) / (an->data[0]-2) + an->data[1];
        int p = p1 + ((p2 - p1) * spos) / (an->speed/2);
        int f = (((p2 - p1) * spos) % (an->speed/2)) * 255 / (an->speed/2);
        for (int ip = p1; ip < p; ip++) ledstring.channel[0].leds[an->offset+(ip % an->size)] = onoff ? 0 : an->data[dp];
        if (p1 <= p && p < p2) ledstring.channel[0].leds[an->offset+(p % an->size)] = scale(an->data[dp], onoff ? 255-f : f);
        for (int ip = p+1; ip < p2; ip++) ledstring.channel[0].leds[an->offset+(ip % an->size)] = onoff ? an->data[dp] : 0;
    }
    return 0;
}

static int led_animate_idle(ledanim_t *an)
{
    an->pos += 1;
    if (an->pos >= (an->speed * an->size)) an->pos = 0;
    for (int ip = 0; ip < an->size; ip++) {
        ledstring.channel[0].leds[an->offset+ip] = 0;
    }
    ledstring.channel[0].leds[an->offset+(an->pos/an->speed)] = an->data[0];
    return 0;
}

static int led_animate_spin(ledanim_t *an)
{
    an->pos = (an->pos + 1000000) % (an->speed * an->size * 1000);
    ledstring.channel[0].leds[an->offset+(0+(an->pos/(an->speed * 1000)))%an->size] = scale(an->data[0], 64);
    ledstring.channel[0].leds[an->offset+(1+(an->pos/(an->speed * 1000)))%an->size] = scale(an->data[0], 128);
    ledstring.channel[0].leds[an->offset+(2+(an->pos/(an->speed * 1000)))%an->size] = an->data[0];
    return 0;
}

static int led_animate_blank(ledanim_t *an)
{
    if (an->fadepos < an->fadein) {
        an->fadepos += 1;
        int fade = 255 - ((255 * an->fadepos) / an->fadein);
        for (int ip = 0; ip < an->size; ip++) {
            ledstring.channel[0].leds[an->offset+ip] = scale(an->data[ip], fade);
        }
    } else {
        for (int ip = 0; ip < an->size; ip++) {
            ledstring.channel[0].leds[an->offset+ip] = 0;
        }
        return 1;
    }
    return 0;
}

int led_animate(ledanim_t *an) {
    switch (an->type) {
        case ANIMATION_PLASMA:
            return led_animate_plasma(an);
        case ANIMATION_FLASH:
            return led_animate_flash(an);
        case ANIMATION_SWIPE:
            return led_animate_swipe(an);
        case ANIMATION_IDLE:
            return led_animate_idle(an);
        case ANIMATION_SPIN:
            return led_animate_spin(an);
        case ANIMATION_COLORS:
            return led_animate_colors(an);
        case ANIMATION_BLANK:
            return led_animate_blank(an);
        default:
            fprintf(stderr, "Unknown animation type %d\n", an->type);
            return -1;
    }
    return 0;
}

/* ring, fadein, num, [color]... */
int led_set_plasma(int ring, int fadein, int num, ...)
{
    pdebug("led_set_plasma(%d, %d, %d)", ring, fadein, num);
    /* Find previous plasma or other animations */
    ledanim_t *newan = NULL, **an;
    for (an = &led_animations; *an;) {
        if (((*an)->offset == ring*RING_SIZE) && ((*an)->type == ANIMATION_PLASMA) && ((*an)->data[0] == (3*num+1))) {
            newan = *an;
            break;
        } else {
            an = &((*an)->next);
        }
    }
    if (!newan) {
        newan = malloc(sizeof(ledanim_t) + (1+(num*3))*sizeof(int));
        if (!newan) {
            fprintf(stderr, "Allocation for animation failed!\n");
            return -1;
        }
        newan->next = NULL;
        newan->type = ANIMATION_PLASMA;
        newan->offset = ring*RING_SIZE;
        newan->size = RING_SIZE;
        newan->speed = 128;
        newan->fadein = fadein;
        newan->fadepos = 0;
        newan->pos = 0;
        newan->data[0] = 3*num+1;
        float spd = 5;
        for (int i = 0; i < num; i++) {
            newan->data[3*i+2] = RING_SIZE*i/num;
            newan->data[3*i+3] = (int)spd;
            spd = (-spd)*1.4;
        }
        *an = newan;
    }
    va_list argp;
    va_start(argp, num);
    for (int i = 0; i < num; i++) {
        newan->data[3*i+1] = va_arg(argp, unsigned int);
    }
    va_end(argp);
    *an = newan;
    return 0;
}

int led_remove_animation(int ring)
{
    pdebug("led_remove_animation(%d)", ring);
    for (ledanim_t **an = &led_animations; *an;) {
        if (((*an)->offset == ring*RING_SIZE) && ((*an)->type != ANIMATION_FLASH)) {
            ledanim_t *f = *an;
            *an = (*an)->next;
            free(f);
        } else {
            an = &((*an)->next);
        }
    }
    return 0;
}

/* ring, num, [offtime, ontime, color]... */
int led_set_flash(int ring, int num, ...)
{
    va_list argp;
    va_start(argp, num);
    ledanim_t **an = &led_animations;
    while (*an) an = &((*an)->next);
    ledanim_t *newan = malloc(sizeof(ledanim_t) + (1+(num*3))*sizeof(int));
    if (!newan) {
        fprintf(stderr, "Allocation for animation failed!\n");
        return -1;
    }
    newan->next = NULL;
    newan->type = ANIMATION_FLASH;
    newan->offset = ring*RING_SIZE;
    newan->size = RING_SIZE;
    newan->speed = 32;
    newan->fadein = 0;
    newan->fadepos = 0;
    newan->pos = 0;
    newan->data[0] = 3*num+1;
    for (int i = 0; i < num; i++) {
        newan->data[3*i+1] = va_arg(argp, unsigned int);
        newan->data[3*i+2] = va_arg(argp, unsigned int);
        newan->data[3*i+3] = va_arg(argp, unsigned int);
    }
    va_end(argp);
    *an = newan;
    return 0;
}

/* ring, speed, offset, num, [color]... */
int led_set_swipe(int ring, int speed, int offset, int num, ...)
{
    va_list argp;
    va_start(argp, num);
    ledanim_t **an = &led_animations;
    while (*an) an = &((*an)->next);
    ledanim_t *newan = malloc(sizeof(ledanim_t) + (2+(num))*sizeof(int));
    if (!newan) {
        fprintf(stderr, "Allocation for animation failed!\n");
        return -1;
    }
    newan->next = NULL;
    newan->type = ANIMATION_SWIPE;
    newan->offset = ring*RING_SIZE;
    newan->size = RING_SIZE;
    newan->speed = speed;
    newan->fadein = 0;
    newan->fadepos = 0;
    newan->pos = 0;
    newan->data[0] = num+2;
    newan->data[1] = offset;
    for (int i = 0; i < num; i++) {
        newan->data[i+2] = va_arg(argp, unsigned int);
    }
    va_end(argp);
    *an = newan;
    return 0;
}

/* ring, speed, color */
int led_set_idle(int ring, int speed, unsigned int color)
{
    ledanim_t **an = &led_animations;
    while (*an) an = &((*an)->next);
    ledanim_t *newan = malloc(sizeof(ledanim_t) + ((1))*sizeof(int));
    if (!newan) {
        fprintf(stderr, "Allocation for animation failed!\n");
        return -1;
    }
    newan->next = NULL;
    newan->type = ANIMATION_IDLE;
    newan->offset = ring*RING_SIZE;
    newan->size = RING_SIZE;
    newan->speed = speed;
    newan->fadein = 0;
    newan->fadepos = 0;
    newan->pos = 0;
    newan->data[0] = color;
    *an = newan;
    return 0;
}

/* ring, speed, color */
int led_set_spin(int ring, int speed, unsigned int color)
{
    pdebug("led_set_spin(%d, %d, %06x)", ring, speed, color);
    /* Find previous spin */
    ledanim_t *newan = NULL, **an;
    for (an = &led_animations; *an;) {
        if (((*an)->offset == ring*RING_SIZE) && ((*an)->type == ANIMATION_SPIN)) {
            if (speed == 0) {
                ledanim_t *f = *an;
                *an = (*an)->next;
                free(f);
                return 0;
            }
            newan = *an;
            break;
        } else {
            an = &((*an)->next);
        }
    }
    if (speed == 0) return 0;
    if (!newan) {
        newan = malloc(sizeof(ledanim_t) + ((1))*sizeof(int));
        if (!newan) {
            fprintf(stderr, "Allocation for animation failed!\n");
            return -1;
        }
        newan->next = NULL;
        newan->type = ANIMATION_SPIN;
        newan->offset = ring*RING_SIZE;
        newan->size = RING_SIZE;
        newan->fadein = 0;
        newan->fadepos = 0;
        newan->pos = 0;
        *an = newan;
    }
    newan->speed = speed;
    newan->data[0] = color;
    return 0;
}

/* ring, fadeout */
int led_set_blank(int ring, int fadein)
{
    ledanim_t **an = &led_animations;
    while (*an) an = &((*an)->next);
    ledanim_t *newan = malloc(sizeof(ledanim_t) + ((RING_SIZE))*sizeof(int));
    if (!newan) {
        fprintf(stderr, "Allocation for animation failed!\n");
        return -1;
    }
    newan->next = NULL;
    newan->type = ANIMATION_BLANK;
    newan->offset = ring*RING_SIZE;
    newan->size = RING_SIZE;
    newan->speed = 0;
    newan->fadein = fadein;
    newan->fadepos = 0;
    newan->pos = 0;
    for (int i = 0; i < RING_SIZE; i++) {
        newan->data[i] = ledstring.channel[0].leds[newan->offset+i];
    }
    *an = newan;
    return 0;
}

int leds_mainloop(void)
{
    for (ledanim_t **an = &led_animations; *an;) {
        if (led_animate(*an) != 0) {
            ledanim_t *f = *an;
            *an = (*an)->next;
            free(f);
        } else {
            an = &((*an)->next);
        }
    }
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
