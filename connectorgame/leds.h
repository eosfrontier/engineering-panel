/*
 * Leds aansturen met rpi_ws2811 library
 */

#define LED_COUNT               96
#define BRIGHTNESS              255

#define NUM_COLORS              10
#define COL_COLOR               0x00202080
#define POS_COLOR               0x0040ff40
#define NONE_COLOR              0x00100202

#define BLACK_COLOR             0x00101010
#define BLUE_COLOR              0x000000ff
#define GREEN_COLOR             0x0000ff00
#define YELLOW_COLOR            0x00cccc00
#define RED_COLOR               0x00ff0000
#define GOOD_COLOR              0x00ffffff
#define BAD_COLOR               0x00000000

#define CHSPEED                 0x10

#define NUM_GROUPS              4
#define GROUP_STARTS            {10,14,10,14}
#define GROUP_RINGS             {48,48,24,24}
#define RING_SIZE               24
#define GROUP_DIRS              {-1,1,-1,1}
#define GROUP_BLANKS            {24,35,36,37,48,59,60,61}

int init_leds(void);
int fini_leds(void);
int led_set_colors(int *colors);
int ledshow_mastermind(int side, int colors, int correct);
int led_set_plasma(int ring, int fadein, int num, ...);
int led_remove_animation(int ring);
int led_set_flash(int ring, int num, ...);
int led_set_blink(int ring, int num, ...);
int led_set_swipe(int ring, int speed, int offset, int num, ...);
int led_set_idle(int ring, int speed, int pos, unsigned int color);
int led_set_spin(int ring, int speed, unsigned int color);
int led_set_static(int ring, int speed, int variance, unsigned int mincol, unsigned int maxcol);
int led_set_blank(int ring, int fade);
int leds_mainloop(void);

/* vim: ai:si:expandtab:ts=4:sw=4
 */
