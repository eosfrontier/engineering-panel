/*
 * Leds aansturen met rpi_ws2811 library
 */

#define LED_COUNT               96
#define BRIGHTNESS              255

#define NUM_COLORS              10
#define COL_COLOR               0x00202080
#define POS_COLOR               0x0040ff40
#define NONE_COLOR              0x00100202

#define CHSPEED                 0x10

#define NUM_GROUPS              4
#define GROUP_STARTS            {1,23,1,23}
#define GROUP_RINGS             {48,48,24,24}
#define RING_SIZE               24
#define GROUP_DIRS              {1,-1,1,-1}

int init_leds(void);
int fini_leds(void);
int ledshow_mastermind(int side, int colors, int correct);
int led_set_blobs(int offset, unsigned int color1, unsigned int color2, unsigned int color3);
int leds_mainloop(void);

/* vim: ai:si:expandtab:ts=4:sw=4
 */
