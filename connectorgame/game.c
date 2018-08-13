#include <stdlib.h>
#include <string.h>
#include "mcp.h"
#include "leds.h"
#include "audio.h"
#include "main.h"
#include "game.h"

static char c_colors[NUM_PINS] = CONNECTOR_COLORS;

static int bootcount = 0;
static int flashcount = 0;

void init_game(void)
{
    for (int i = 0; i < NUM_PINS; i++) {
        switch (c_colors[i]) {
            case 'Z': c_colors[i] = BLACK;  break;
            case 'B': c_colors[i] = BLUE;   break;
            case 'G': c_colors[i] = GREEN;  break;
            case 'Y': c_colors[i] = YELLOW; break;
            case 'R': c_colors[i] = RED;    break;
        }
    }
}

static int randint(int from, int to)
{
    return from + ((random() % (to - from + 1)));
}

static double randdbl(double from, double to)
{
    return from + ((random() * (1.0 / RAND_MAX) * (to - from)));
}

static double vary(double val, double var)
{
    return val * randdbl(1.0 - var, 1.0 + var);
}

static void engine_hum(double basefreq, double beatstep, double beatvar, double hibeat, double hivar, double hivol, int fade, int fadevar, int fadehi, int fadehivar)
{
    double lowvol = 0.45;
    audio_play_synth(0, 0, SYNTH_SINE, vary(basefreq * (1.0 + (beatstep * 0.0)), beatvar), lowvol, randint(fade-fadevar, fade+fadevar));
    audio_play_synth(0, 2, SYNTH_SINE, vary(basefreq * (1.0 + (beatstep * 1.0)), beatvar), lowvol, randint(fade-fadevar, fade+fadevar));
    audio_play_synth(0, 1, SYNTH_SINE, vary(basefreq * (1.0 + (beatstep * 2.0)), beatvar), lowvol, randint(fade-fadevar, fade+fadevar));
    audio_play_synth(0, 3, SYNTH_SINE, vary(basefreq * (1.0 + (beatstep * 3.0)), beatvar), lowvol, randint(fade-fadevar, fade+fadevar));
    audio_play_synth(0, 5, SYNTH_SINE, vary(basefreq * hibeat * (1.0 + (beatstep * 0.0)), hivar), lowvol*hivol, randint(fade-fadehivar, fade+fadehivar));
    audio_play_synth(0, 7, SYNTH_SINE, vary(basefreq * hibeat * (1.0 + (beatstep * 1.0)), hivar), lowvol*hivol, randint(fade-fadehivar, fade+fadehivar));
    audio_play_synth(0, 4, SYNTH_SINE, vary(basefreq * hibeat * (1.0 + (beatstep * 2.0)), hivar), lowvol*hivol, randint(fade-fadehivar, fade+fadehivar));
    audio_play_synth(0, 6, SYNTH_SINE, vary(basefreq * hibeat * (1.0 + (beatstep * 3.0)), hivar), lowvol*hivol, randint(fade-fadehivar, fade+fadehivar));
}

static void flash_spark(void)
{
    audio_play_file(1, WAV_SPARK);
    led_set_flash(0, 5, 0, randint(2,5), 0xffffff, randint(3,8), randint(2,5), 0xccccff, randint(3,6), randint(2,4), 0xffcccc, randint(2,4), randint(5,10), 0x000000, randint(0,3)*5, randint(2,4), 0x000000);
    led_set_flash(3, 5, 0, randint(2,5), 0xffccff, randint(3,8), randint(2,5), 0xffccff, randint(3,6), randint(2,4), 0xffffff, randint(2,4), randint(5,10), 0x000000, randint(0,3)*6, randint(2,4), 0x000000);
    led_set_flash(1, 3, 0, randint(2,5), 0xff8888, randint(3,8), randint(2,5), 0xffffff, randint(6,12), randint(10,25), 0x000000);
    led_set_flash(2, 3, 0, randint(2,5), 0xff8888, randint(3,8), randint(2,5), 0xffffff, randint(6,12), randint(10,25), 0x000000);
}

static int game_booting(clist_t *conns)
{
    if (bootcount > 0) {
        bootcount--;
        return GAME_BOOTING;
    } else if (conns->on >= 10) {
        return GAME_OK;
    } else {
        return GAME_BREAK;
    }
}

static int game_oking(clist_t *conns)
{
    if ((conns->buttons[0].status & BUTTON_CLICKS) >= 3) {
        return GAME_BREAK;
    }
    if (conns->off > 0) {
        return GAME_BREAK;
    }
    return GAME_OKING;
}

static int lastokcnt = 20;
static struct puzzle {
    int solution[NUM_ROWS];
} puzzle;

static void game_set_mastermind(clist_t *conns, int from, int to)
{
    pdebug("game_set_puzzle(%d/%d/%d, %d, %d)", conns->on, conns->newon, conns->off, from, to);
    for (int i = 0; i < NUM_ROWS; i++) {
        if (i < from || i >= to) {
            puzzle.solution[i] = -1;
        } else {
            puzzle.solution[i] = randint(i*5, (i+1)*5-1);
            pdebug("Solution[%d] = %d", i, puzzle.solution[i]);
        }
    }
    /* Midden reserveren voor indicaties */
    led_remove_animation(1);
    led_remove_animation(2);
    lastokcnt = 20;
}

static int level = 0;

static int game_breaking(clist_t *conns)
{
    level = 1;
    return GAME_COLOR;
}

static int game_coloring(clist_t *conns)
{
    if (conns->buttons[0].status >= BUTTON_ON + 5) {
        return GAME_BOOT;
    }
    if (conns->newon > 0) {
        audio_play_file(1, WAV_ON);
        for (int i = (conns->on - conns->newon); i < conns->on; i++) {
            pdebug("New connection: %d - %d", conns->pins[i].p1, conns->pins[i].p2);
        }
    } else if (conns->off > 0) {
        audio_play_file(1, WAV_OFF);
    } else if (--flashcount <= 0) {
        flash_spark();
        flashcount = (int)(((double)(FRAMERATE/10 + (random() % (FRAMERATE * 4)))) * (1.0 + (((double)conns->on)/4)));
    }
    int colors[40] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int *correct = &colors[20];
    /* Kijken voor juiste posities */
    int okcnt = 0;
    for (int i = 0; i < conns->on; i++) {
        int s[2] = { conns->pins[i].p1, conns->pins[i].p2 };
        if (level > 1) {
            if ((s[0] >= 50) == (s[1] >= 50)) {
                for (int cc = 0; cc < 2; cc++) {
                    int r = PIN_ROW(s[cc]);
                    correct[r] |= BAD;
                }
                continue;
            }
        }
        for (int cc = 0; cc < 2; cc++) {
            int r = PIN_ROW(s[cc]);
            /* Kijken of de positie klopt */
            if (puzzle.solution[r] == s[cc]) {
                okcnt++;
                colors[r] |= GOOD;
                correct[r] |= GOOD;
            } else {
                /* Kleur zetten */
                colors[r] |= c_colors[s[cc]];
                correct[r] |= c_colors[puzzle.solution[r]];
            }
        }
    }
    ledshow_colors(colors);
    if (okcnt < 20) {
        if (okcnt != lastokcnt) {
            lastokcnt = okcnt;
            engine_hum(95.0 - (1.0 * (20-okcnt)), 0.25, 0.01 * (20-okcnt), 2.0, 0.01 * (20-okcnt), 0.1, FRAMERATE, FRAMERATE/2, FRAMERATE*2, FRAMERATE);
        }
        return GAME_COLORING;
    } else {
        return GAME_FIXED;
    }
}

static int game_masterminding(clist_t *conns)
{
    if (conns->newon > 0) {
        audio_play_file(1, WAV_ON);
        for (int i = (conns->on - conns->newon); i < conns->on; i++) {
            pdebug("New connection: %d - %d", conns->pins[i].p1, conns->pins[i].p2);
        }
    } else if (conns->off > 0) {
        audio_play_file(1, WAV_OFF);
    } else if (--flashcount <= 0) {
        flash_spark();
        flashcount = (int)(((double)(FRAMERATE/10 + (random() % (FRAMERATE * 4)))) * (1.0 + (((double)conns->on)/4)));
    }
    int colcnts[2] = {0,0};
    int poscnts[2] = {0,0};
    char seenpos[NUM_ROWS];
    for (int i = 0; i < NUM_ROWS; i++) {
        /* -1 is altijd goed */
        if (puzzle.solution[i] < 0) {
            seenpos[i] = 1;
            poscnts[(i >= 10)]++;
        } else {
            seenpos[i] = 0;
        }
    }
    /* Eerst kijken voor juiste posities */
    for (int i = 0; i < conns->on; i++) {
        int s[2] = { conns->pins[i].p1, conns->pins[i].p2 };
        for (int cc = 0; cc < 2; cc++) {
            int r = PIN_ROW(s[cc]);
            /* Kijken of de positie klopt */
            if (!seenpos[r]) {
                if (puzzle.solution[r] == s[cc]) {
                    seenpos[r] = 1;
                    poscnts[(r >= 10)]++;
                }
            }
        }
    }
    /* Dan kijken voor de resterende juiste kleuren */
    for (int i = 0; i < conns->on; i++) {
        int s[2] = { conns->pins[i].p1, conns->pins[i].p2 };
        for (int cc = 0; cc < 2; cc++) {
            int r = PIN_ROW(s[cc]);
            /* Kijken of de kleur klopt */
            int side = (r >= (NUM_ROWS/2)) ? (NUM_ROWS/2) : 0;
            for (int cr = side; cr < side + NUM_ROWS/2; cr++) {
                if (!seenpos[cr]) {
                    if (c_colors[puzzle.solution[cr]] == c_colors[s[cc]]) {
                        seenpos[cr] = 1;
                        colcnts[(r >= 10)]++;
                        break;
                    }
                }
            }
        }
    }
    ledshow_mastermind(0, colcnts[0], poscnts[0]);
    ledshow_mastermind(1, colcnts[1], poscnts[1]);
    if (poscnts[0] < 10 || poscnts[1] < 10) {
        return GAME_MASTERMINDING;
    } else {
        return GAME_FIXED;
    }
}

static int game_fixeding(clist_t *conns)
{
    if (bootcount > 0) {
        bootcount--;
        return GAME_FIXEDING;
    } else if (conns->on >= 10) {
        return GAME_OK;
    } else {
        return GAME_BREAK;
    }
}

static int game_starting(clist_t *conns)
{
    if (conns->buttons[0].status >= BUTTON_ON + 3) {
        return GAME_BOOT;
    }
    if (debugging) {
        return GAME_BOOT;
    }
    return GAME_STARTING;
}

int game_mainloop(int gamestate, clist_t *conns)
{
    switch (gamestate) {
        default: /* Fallthrough to boot */
        case GAME_START:
            pdebug("GAME_START");
            led_set_idle(2, FRAMERATE/4, 0x020004);
            engine_hum(0.0, 0.0, 0.0, 0.0, 0.0, 0.05, 1, 0, 1, 0);
        case GAME_STARTING:
            return game_starting(conns);
        case GAME_BOOT:
            pdebug("GAME_BOOT");
            // audio_play_file(1, WAV_BOOTING);
            led_set_swipe(0, FRAMERATE*2, 12, 3, 0xff0000, 0xff0000, 0xff0000);
            led_set_swipe(1, FRAMERATE*2, 0, 3, 0x00ff00, 0x00ff00, 0x00ff00);
            led_set_swipe(2, FRAMERATE*2, 0, 3, 0x888800, 0x888800, 0x888800);
            led_set_swipe(3, FRAMERATE*2, 0, 3, 0x0000ff, 0x0000ff, 0x0000ff);
            /* Synth hum */
            engine_hum(100.0, 0.25, 0.0, 2.0, 0.0, 0.05, FRAMERATE*2, FRAMERATE, FRAMERATE*3, FRAMERATE);
            bootcount = FRAMERATE*2/SCANRATE;
        case GAME_BOOTING:
            return game_booting(conns);
        case GAME_OK:
            pdebug("GAME_OK");
            /* Wat leuke animaties */
            led_set_blobs(0, FRAMERATE*2, 3, 0x003300, 0x002211, 0x000033);
            led_set_blobs(3, FRAMERATE*2, 4, 0x000033, 0x003300, 0x001133, 0x003311);

            led_set_blobs(1, FRAMERATE*2, 3, 0x002222, 0x002200, 0x000022);
            led_set_blobs(2, FRAMERATE*2, 3, 0x002222, 0x000022, 0x002200);
            // audio_play_file(1, WAV_READY);
        case GAME_OKING:
            return game_oking(conns);
        case GAME_BREAK:
            pdebug("GAME_BREAK");
            flashcount = 0;
            /* Rodere animaties */
            led_set_blobs(0, 0, 3, 0x330000, 0x221100, 0x000011);
            led_set_blobs(3, 0, 4, 0x330000, 0x003300, 0x331100, 0x113300);
            /* Synth hum */
            engine_hum(80.0, 0.25, 0.1, 2.0, 0.2, 0.1, FRAMERATE*3, FRAMERATE*2, FRAMERATE*5, FRAMERATE*3);
        case GAME_BREAKING:
            /* TODO: Broken modus, wachten tot iemand begint met oplossen */
            return game_breaking(conns);
        case GAME_COLOR:
            pdebug("GAME_COLOR");
            game_set_mastermind(conns,0,20);
        case GAME_COLORING:
            return game_coloring(conns);
        case GAME_MASTERMIND:
            pdebug("GAME_MASTERMIND");
            game_set_mastermind(conns,0,20);
        case GAME_MASTERMINDING:
            return game_masterminding(conns);
        case GAME_FIXED:
            pdebug("GAME_FIXED");
            led_set_swipe(0, FRAMERATE*3, 12, 3, 0xff0000, 0xff0000, 0xff0000);
            led_set_swipe(1, FRAMERATE*3, 0, 3, 0x00ff00, 0x00ff00, 0x00ff00);
            led_set_swipe(2, FRAMERATE*3, 0, 3, 0x888800, 0x888800, 0x888800);
            led_set_swipe(3, FRAMERATE*3, 0, 3, 0x0000ff, 0x0000ff, 0x0000ff);
            engine_hum(100.0, 0.25, 0.0, 2.0, 0.0, 0.05, FRAMERATE*4, FRAMERATE*2, FRAMERATE*6, FRAMERATE*2);
            bootcount = FRAMERATE*3/SCANRATE;
        case GAME_FIXEDING:
            return game_fixeding(conns);
    }
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
