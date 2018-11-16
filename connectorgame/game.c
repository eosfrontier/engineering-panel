#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "mcp.h"
#include "leds.h"
#include "audio.h"
#include "main.h"
#include "comm.h"
#include "game.h"

#define SPINUP_SEC (settings.spinup)
#define SPINDOWN_SEC (settings.spindown)
#define SPINUP_SPEED (1.0/(SPINUP_SEC*FRAMERATE/SCANRATE))
#define SPINDOWN_SPEED (1.0/(SPINDOWN_SEC*FRAMERATE/SCANRATE))
#define SPINUP_LOW1 (settings.spinlow1)
#define SPINUP_LOW2 (settings.spinlow2)
#define SPINUP_FREQ1 (settings.spinfreq1)
#define SPINUP_FREQ2 (settings.spinfreq2)
#define SPINUP_VOL1 (settings.spinvol1)
#define SPINUP_VOL2 (settings.spinvol2)
#define SPINUP_WAVE SYNTH_TRIANGLE
#define SPINUP_RINGSPEED1 (settings.spinspeed1)
#define SPINUP_RINGSPEED2 (settings.spinspeed2)

#define ENGINE_FREQ (settings.humfreq)
#define ENGINE_TBFREQ (settings.turbinefreq)
#define ENGINE_RPFREQ (settings.repairfreq)
#define ENGINE_VOL (settings.humvol)
#define ENGINE_HIVOL (settings.humvolhi)
#define ENGINE_BEAT (settings.humbeat)
#define ENGINE_HIBEAT (settings.hibeat)
#define ENGINE_BASEVAR (1.0 + (settings.humbasevar))

#define BREAKDOWN (settings.breakdown)

#define DIFFICULTY (settings.difficulty)

#define STATIC_SPEED (repairlevel*20)
#define STATIC_VARIANCE (repairlevel*20)
#define STATIC_MINCOLOR 0x440000
#define STATIC_MAXCOLOR 0x884444

#define COLOR_FILE COMM_PATH "colors.txt"

/* Complete degeneration in 4 hours */
#define DECAYTIME (settings.decaytime)
#define REPAIR_DECAY (1.0 / (DECAYTIME * 60 * 60 * (FRAMERATE/SCANRATE)))

/* Ophouden met repair-mode na 3 minuten */
#define REPAIR_TIMEOUT ((FRAMERATE/SCANRATE) * 3 * 60)

static int spinup_ring[3] = { 0, 2, 3 };
static int spinup_color[3] = { 0x0000ff, 0xff0000, 0x00ff00 };

static int plasma_color[4][5] = {
    { 3, 0x00cc00, 0x008844, 0x0000cc, 0 },
    { 3, 0x008888, 0x008800, 0x000088, 0 },
    { 3, 0x008888, 0x000088, 0x008800, 0 },
    { 4, 0x0000cc, 0x00cc00, 0x0044cc, 0x00cc44 }
};

static char c_colors[NUM_PINS];

static int animdelay = 0;
static int flashdelay = 0;
static int repairing = 0;
static int running = 0;
static int booting = 10;
double turbines[3] = {0.0, 0.0, 0.0};
double repairlevel = 1.0;

puzzle_t puzzle;

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

static void engine_hum(double lowvol, double basefreq, double beatstep, double beatvar, double hibeat, double hivar, double hivol, int fade, int fadevar, int fadehi, int fadehivar)
{
    audio_synth_freq_vol(0, 0, vary(basefreq * (1.0 + (beatstep * 0.0)), beatvar), lowvol, randint(fade-fadevar, fade+fadevar));
    audio_synth_freq_vol(0, 2, vary(basefreq * (1.0 + (beatstep * 1.0)), beatvar), lowvol, randint(fade-fadevar, fade+fadevar));
    audio_synth_freq_vol(0, 1, vary(basefreq * (1.0 + (beatstep * 2.0)), beatvar), lowvol, randint(fade-fadevar, fade+fadevar));
    audio_synth_freq_vol(0, 3, vary(basefreq * (1.0 + (beatstep * 3.0)), beatvar), lowvol, randint(fade-fadevar, fade+fadevar));
    audio_synth_freq_vol(0, 5, vary(basefreq * hibeat * (1.0 + (beatstep * 0.0)), hivar), lowvol*hivol, randint(fade-fadehivar, fade+fadehivar));
    audio_synth_freq_vol(0, 7, vary(basefreq * hibeat * (1.0 + (beatstep * 1.0)), hivar), lowvol*hivol, randint(fade-fadehivar, fade+fadehivar));
    audio_synth_freq_vol(0, 4, vary(basefreq * hibeat * (1.0 + (beatstep * 2.0)), hivar), lowvol*hivol, randint(fade-fadehivar, fade+fadehivar));
    audio_synth_freq_vol(0, 6, vary(basefreq * hibeat * (1.0 + (beatstep * 3.0)), hivar), lowvol*hivol, randint(fade-fadehivar, fade+fadehivar));
}

static void engine_volume(void)
{
    double lowvol = ENGINE_VOL;
    double turbinefade = (turbines[0]+turbines[1]+turbines[2]);
    if (turbinefade < 1.0) lowvol *= turbinefade;
    double hivol = ENGINE_HIVOL;
    audio_synth_freq_vol(0, 0, 0, lowvol, 2);
    audio_synth_freq_vol(0, 2, 0, lowvol, 2);
    audio_synth_freq_vol(0, 1, 0, lowvol, 2);
    audio_synth_freq_vol(0, 3, 0, lowvol, 2);
    audio_synth_freq_vol(0, 5, 0, lowvol*hivol, 2);
    audio_synth_freq_vol(0, 7, 0, lowvol*hivol, 2);
    audio_synth_freq_vol(0, 4, 0, lowvol*hivol, 2);
    audio_synth_freq_vol(0, 6, 0, lowvol*hivol, 2);
}

static void engine_hum_set(int fade, int fadevar, int fadehi, int fadehivar)
{
            if (repairlevel <= BREAKDOWN) {
                /* Helemaal stuk, engine valt uit */
                if (turbines[0]+turbines[1]+turbines[2] > 0.1) {
                    engine_hum(ENGINE_VOL, 1.0, 1.0, 1.0, 1.0, 1.0, ENGINE_HIVOL, FRAMERATE*7, FRAMERATE*4, FRAMERATE*6, FRAMERATE*4);
                    led_set_blink(0, 2, FRAMERATE/2, 0xff0000, FRAMERATE/3, 0x000000);
                    led_set_blink(1, 1, FRAMERATE, 0x000000);
                    led_set_blink(2, 2, FRAMERATE/2, 0xff0000, FRAMERATE/3, 0x000000);
                    led_set_blink(3, 2, FRAMERATE/2, 0xff0000, FRAMERATE/3, 0x000000);
                    led_set_blank(0, FRAMERATE);
                    led_set_blank(1, FRAMERATE);
                    led_set_blank(2, FRAMERATE);
                    led_set_blank(3, FRAMERATE);
                } else {
                    engine_hum(0.0, 1.0, 1.0, 1.0, 1.0, 1.0, ENGINE_HIVOL, FRAMERATE*7, FRAMERATE*4, FRAMERATE*6, FRAMERATE*4);
                    led_set_static(0, -1, 0, 0, 0); /* Remove static */
                    led_set_static(1, -1, 0, 0, 0); /* Remove static */
                    led_set_static(2, -1, 0, 0, 0); /* Remove static */
                    led_set_static(3, -1, 0, 0, 0); /* Remove static */
                }
            } else {
                double lowvol = ENGINE_VOL;
                double turbinefade = 5.0 * (turbines[0]+turbines[1]+turbines[2]);
                if (turbinefade < 1.0) lowvol *= turbinefade;
                engine_hum(lowvol, ENGINE_FREQ - (ENGINE_TBFREQ*(3-running)) - ENGINE_RPFREQ*(1.0-repairlevel), ENGINE_BEAT, 0.2 * (ENGINE_BASEVAR-repairlevel), ENGINE_HIBEAT, 0.2 * (ENGINE_BASEVAR-repairlevel), ENGINE_HIVOL, fade, fadevar, fadehi, fadehivar);
                if (repairlevel <= 0.9) {
                    led_set_static(0, (int)(STATIC_SPEED), (int)(STATIC_VARIANCE), STATIC_MINCOLOR, STATIC_MAXCOLOR);
                    led_set_static(1, (int)(STATIC_SPEED), (int)(STATIC_VARIANCE), STATIC_MINCOLOR, STATIC_MAXCOLOR);
                    led_set_static(2, (int)(STATIC_SPEED), (int)(STATIC_VARIANCE), STATIC_MINCOLOR, STATIC_MAXCOLOR);
                    led_set_static(3, (int)(STATIC_SPEED), (int)(STATIC_VARIANCE), STATIC_MINCOLOR, STATIC_MAXCOLOR);
                } else {
                    led_set_static(0, -1, 0, 0, 0); /* Remove static */
                    led_set_static(1, -1, 0, 0, 0); /* Remove static */
                    led_set_static(2, -1, 0, 0, 0); /* Remove static */
                    led_set_static(3, -1, 0, 0, 0); /* Remove static */
                }
            }
}

static void init_engine_hum(void)
{
    for (int i = 0; i < 8; i++) {
        audio_synth_wave(0, i, SYNTH_SINE);
    }
    for (int i = 0; i < 6; i++) {
        audio_synth_wave(0, i+8, SPINUP_WAVE);
        audio_synth_modulate(0, i+8, i);
    }
    engine_hum(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.05, 1, 0, 1, 0);
}

void init_game(void)
{
    init_engine_hum();
    repairlevel = 1.0;
    booting = 10;
    led_set_idle(0, FRAMERATE/4, 0, 0x010002);
    led_set_idle(1, FRAMERATE/4, 6*(FRAMERATE/4), 0x010002);
    led_set_idle(2, FRAMERATE/4, 12*(FRAMERATE/4), 0x010002);
    led_set_idle(3, FRAMERATE/4, 18*(FRAMERATE/4), 0x010002);
    FILE *f = fopen(COLOR_FILE, "r");
    if (!f) {
        fprintf(stderr, "Failed to open colors file %s: %s\n", COLOR_FILE, strerror(errno));
        return;
    }
    if (fread(c_colors, 1, NUM_PINS, f) < NUM_PINS) {
        fprintf(stderr, "Failed to read colors file %s: %s\n", COLOR_FILE, strerror(errno));
        return;
    }
    fclose(f);
    for (int i = 0; i < NUM_PINS; i++) {
        switch (c_colors[i]) {
            case 'Z': c_colors[i] = BLACK;  break;
            case 'B': c_colors[i] = BLUE;   break;
            case 'G': c_colors[i] = GREEN;  break;
            case 'Y': c_colors[i] = YELLOW; break;
            case 'R': c_colors[i] = RED;    break;
        }
    }
    puzzle.type = 0;
    for (int i = 0; i < NUM_ROWS; i++) {
        puzzle.solution[i] = 1 << randint(0,4);
        puzzle.current[i] = 0;
    }
}

static void flash_spark(void)
{
    audio_play_file(1, WAV_SPARK);
    led_set_flash(0, 5, 0, randint(2,5), 0xffffff, randint(3,8), randint(2,5), 0xccccff, randint(3,6), randint(2,4), 0xffcccc, randint(2,4), randint(5,10), 0x000000, randint(0,3)*5, randint(2,4), 0x000000);
    led_set_flash(3, 5, 0, randint(2,5), 0xffccff, randint(3,8), randint(2,5), 0xffccff, randint(3,6), randint(2,4), 0xffffff, randint(2,4), randint(5,10), 0x000000, randint(0,3)*6, randint(2,4), 0x000000);
    led_set_flash(1, 3, 0, randint(2,5), 0xff8888, randint(3,8), randint(2,5), 0xffffff, randint(6,12), randint(10,25), 0x000000);
    led_set_flash(2, 3, 0, randint(2,5), 0xff8888, randint(3,8), randint(2,5), 0xffffff, randint(6,12), randint(10,25), 0x000000);
}

static inline int bitcnt(int bits)
{
    int cnt = 0;
    for (; bits > 0; bits >>= 1) cnt += (bits & 1);
    return cnt;
}

static void game_checklevel(clist_t *conns)
{
    if (booting > 0) {
        /* Pinnen laten stabiliseren */
        booting--;
        return;
    }
    if ((conns->event & REPAIR) && repairlevel > 0.9) {
        /* Iemand heeft extern een 'fix' gedaan.  Zorg dat het fixed-effect afgaat */
        repairing = (FRAMERATE/SCANRATE)*1;
    }
    if (repairlevel > 0.0 && (turbines[0]+turbines[1]+turbines[2]) > 2.0) {
        /* Als de boel draait, gaat het langzaam stuk. (Helemaal stuk na X uur) */
        if (DECAYTIME > 0) {
            repairlevel -= REPAIR_DECAY;
        }
    }
    int okcnt = 0;         // Telt hoeveel goede connectors er zijn
    int okcnts[conns->on]; // Hoeveel connecties er goed zijn per kabel
    int okpc[3] = {0,0,0}; // Aantal connecties met 0, 1 of 2 goede stekkers
    for (int i = 0; i < NUM_ROWS; i++) {
        puzzle.current[i] = 0;
    }
    /* Tellen hoeveel conecties er goed zijn */
    for (int i = 0; i < conns->on; i++) {
        okcnts[i] = 0;
        for (int cc = 0; cc < 2; cc++) {
            int p = conns->pins[i].p[cc];
            int r = PIN_ROW(p);
            p = p % 5;
            puzzle.current[r] |= (1 << p);
            if (puzzle.solution[r] & (1 << p)) {
                okcnt++;
                okcnts[i]++;
            }
        }
        okpc[okcnts[i]]++;
    }
    if (conns->event & HUMSETTING) {
        /* Iemand heeft geluids settings aangepast (tweak.php) */
        engine_hum_set(FRAMERATE, FRAMERATE/2, FRAMERATE*2, FRAMERATE);
    }
    int slbtn = conns->buttons[BUTTON_SL].status;
    if ((slbtn & (BUTTON_CLICKED|BUTTON_HOLD)) && (slbtn & BUTTON_CLICKS) > 2) {
        /* Het kleine knopje achter de power connector als backup voor admin interface */
        pdebug("SL-Button pressed %d times, flags = %x", (slbtn & BUTTON_CLICKS), slbtn >> 8);
        if (slbtn & BUTTON_HOLD) {
            if ((slbtn & BUTTON_CLICKS) >= 4) {
                repairlevel = 1.0;
                repairing = (FRAMERATE/SCANRATE)*1;
            }
        } else {
            repairlevel -= 0.1 * (slbtn & BUTTON_CLICKS);
            if (repairlevel < 0.0) repairlevel = 0.0;
        }
    }
    for (int sw = 0; sw < 3; sw++) {
        /* Als iemand een schakelaar snel aan en uit zet dan gaat het stuk */
        int clicks = conns->buttons[sw].status & BUTTON_CLICKS;
        if (clicks > 4) {
            pdebug("Switch toggled %d times", clicks);
            if (repairlevel > (1.0 - 0.1 * (clicks-2))) {
                repairlevel = (1.0 - 0.1 * clicks);
                if (repairlevel < 0.0) repairlevel = 0.0;
                pdebug("Switch toggled break level %g", repairlevel);
            }
        }
    }
    /* Hoeveel connecties er goed zouden moeten zijn volgens repairlevel */
    int wantok = ((int)((20.0*repairlevel)+0.5));
    if (!(conns->event & REPAIR) && (conns->newon + conns->off) > 0) {
        /* Iemand heeft iets losgetrokken of ingestoken, solution checken en repairlevel aanpassen */
        int newrl = REPAIR_TIMEOUT;
        repairlevel = ((double)okcnt / 20.0);
        if (okcnt != wantok) {
            engine_hum_set(FRAMERATE, FRAMERATE/2, FRAMERATE*2, FRAMERATE);
            if (okcnt == 20) {
                newrl = (FRAMERATE/SCANRATE)*1;
            }
        }
        static connection_t lastconn;
        if (conns->newon > 0) {
            /* Laast vastgestoken kabel onthouden ivm debounce */
            lastconn = conns->pins[conns->on-1];
        }
        /* Checken of het een goed-goed verbinding was die is losgegaan, zo ja de oplossing veranderen */
        for (int i = conns->on; i < conns->on + conns->off; i++) {
            int nokcnt = 0;
            for (int cc = 0; cc < 2; cc++) {
                int p = conns->pins[i].p[cc];
                int r = PIN_ROW(p);
                p = p % 5;
                if (puzzle.solution[r] & (1 << p)) {
                    nokcnt++;
                }
            }
            if ((nokcnt == 2) && memcmp(&lastconn, &(conns->pins[i]), sizeof(lastconn))) {
                /* Goed-goed verbinding los, extra stukmaken.  Maar alleen als het niet de laatst vastgestoken kabel was, ivm bounce */
                flash_spark();
                for (int cc = 0; cc < 2; cc++) {
                    int p = conns->pins[i].p[cc];
                    int r = PIN_ROW(p);
                    p = p % 5;
                    int pc = puzzle.solution[r];
                    int ccnt = bitcnt(pc ^ 0x1f);
                    /* Nieuwe 'goede' connector in deze rij kiezen */
                    if (ccnt > 0) {
                        ccnt = randint(0, ccnt-1);
                        for (int np = 0; np < 5; np++) {
                            if (!(pc & (1 << np))) {
                                if (ccnt-- <= 0) {
                                    pdebug("Breaking just broken connection %d: %d -> %d", i, conns->pins[i].p[0], conns->pins[i].p[1]);
                                    puzzle.solution[r] = (pc & ~(1 << p)) | (1 << np);
                                    newrl = repairing;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        /* Geeft aan of er iemand met de kabels bezig is wordt, dan tonen we de 'puzzel' voor een bepaalde tijd */
        repairing = newrl;
    } else {
        if (repairing > 0) {
            /* Na bepaalde tijd hint-kleuren weer weghalen */
            if (--repairing <= 0) {
                if (okcnt == 20) {
                    /* 'Felicitatie' rainbow-swipe om te tonen dat alles weer OK is */
                    led_set_swipe(0, FRAMERATE*2, 0, 3, 0x0000ff, 0x0000ff, 0x0000ff);
                    led_set_swipe(1, FRAMERATE*2, 0, 3, 0x888800, 0x888800, 0x888800);
                    led_set_swipe(2, FRAMERATE*2, 12, 3, 0xff0000, 0xff0000, 0xff0000);
                    led_set_swipe(3, FRAMERATE*2, 0, 3, 0x00ff00, 0x00ff00, 0x00ff00);
                }
                led_set_colors(NULL);
            }
        }
        /* Kijken of repairlevel gedaald of gestegen is, evt solution aanpassen */
        if (okcnt > wantok) {
            if (repairlevel <= 0.9) {
                flash_spark(); /* TODO: Small spark */
            }
        }
        if (okcnt != wantok) {
            /* Geluid aanpassen aan hoe stuk het is */
            engine_hum_set(FRAMERATE, FRAMERATE/2, FRAMERATE*2, FRAMERATE);
            pdebug("okcnt: %d <> %d : %d, %d, %d", okcnt, wantok, okpc[0], okpc[1], okpc[2]);
        }
        while (okcnt > wantok) {
            int didbreak = 0;
            /* Een connectie stukmaken */
            /* Liefst een met 1 connectie stukmaken, anders een met 2 connecties */
            for (int okwc = 1; okwc <= 2; okwc++) {
                int bcon = -1;
                pdebug("okcnt: %d > %d : %d, %d, %d okpc[%d] = %d", okcnt, wantok, okpc[0], okpc[1], okpc[2], okwc, okpc[okwc]);
                /* okpc houdt bij hoeveel kabels er X goede verbindingen hebben,
                 * hiermee kunnen we er een random kiezen met een gericht aantal goede verbindingen */
                if (okpc[okwc] > 0) {
                    /* Random een connectie kiezen die okwc (1 of 2) juiste connecties heeft */
                    int ri = randint(0, okpc[okwc]-1);
                    for (int i = 0; i < conns->on; i++) {
                        if (okcnts[i] == okwc) {
                            /* Aftellen van random naar 0, pakt de zoveelste */
                            if (ri > 0) {
                                ri--;
                            } else {
                                bcon = i;
                                break;
                            }
                        }
                    }
                }
                if (bcon >= 0) {
                    pdebug("Breaking good connection %d: %d -> %d", bcon, conns->pins[bcon].p[0], conns->pins[bcon].p[1]);
                    /* Random links of rechts beginnen, mod2 voor wrap */
                    for (int cc = randint(0,1); cc < 3; cc++) {
                        int p = conns->pins[bcon].p[cc % 2];
                        int r = PIN_ROW(p);
                        p = p % 5;
                        if (puzzle.solution[r] & (1 << p)) {
                            /* Tellers bijwerken */
                            okpc[okwc]--;
                            okpc[okwc-1]++;
                            okcnts[bcon]--;
                            /* Over alle rijen gaan voor het geval een rij helemaal vol zit */
                            for (int rr = 0; rr < NUM_ROWS; rr++) {
                                int rrr = (rr + r) % NUM_ROWS;
                                /* Deze gaat stuk: Niet verbonden pin kiezen als nieuwe oplossing */
                                int pc = puzzle.current[rrr] ^ 0x1f;
                                int ccnt = bitcnt(pc);
                                if (ccnt > 0) {
                                    ccnt = randint(0, ccnt-1);
                                    for (int i = 0; i < 5; i++) {
                                        if (pc & (1 << i)) {
                                            if (ccnt > 0) {
                                                ccnt--;
                                            } else {
                                                /* Deze gaat stuk */
                                                puzzle.solution[r] &= ~(1 << p);
                                                /* EN de nieuwe goede wordt dze */
                                                puzzle.solution[rrr] |= 1 << i;
                                                didbreak = 1;
                                                break;
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            if (!didbreak) {
                /* Safety: Eindeloze lus voorkomen */
                wantok = okcnt;
                repairlevel = ((double)okcnt / 20.0);
                break;
            }
            okcnt--;
        }
        while (okcnt < wantok) {
            int didfix = 0;
            /* Een connectie heelmaken */
            /* Liefst een met 1 connectie heelmaken, anders een met 0 connecties */
            for (int okwc = 1; okwc >= 0; okwc--) {
                int bcon = -1;
                pdebug("okcnt: %d < %d : %d, %d, %d okpc[%d] = %d", okcnt, wantok, okpc[0], okpc[1], okpc[2], okwc, okpc[okwc]);
                if (okpc[okwc] > 0) {
                    /* Random een connectie kiezen die okwc (1 of 0) juiste connecties heeft */
                    int ri = randint(0, okpc[okwc]-1);
                    for (int i = 0; i < conns->on; i++) {
                        if (okcnts[i] == okwc) {
                            /* Aftellen van random naar 0, pakt de zoveelste */
                            if (ri > 0) {
                                ri--;
                            } else {
                                bcon = i;
                                break;
                            }
                        }
                    }
                }
                if (bcon >= 0) {
                    pdebug("(%d < %d) Fixing bad connection %d: %d -> %d", okcnt, wantok, bcon, conns->pins[bcon].p[0], conns->pins[bcon].p[1]);
                    /* Random links of rechts beginnen, mod2 voor wrap */
                    for (int cc = randint(0,1); cc < 3; cc++) {
                        int p = conns->pins[bcon].p[cc % 2];
                        int r = PIN_ROW(p);
                        p = p % 5;
                        if (!(puzzle.solution[r] & (1 << p))) {
                            pdebug("  Fixing bad connection %d(%d-%d): %d -> %d", bcon, r, p, conns->pins[bcon].p[0], conns->pins[bcon].p[1]);
                            okpc[okwc]--;
                            okpc[okwc+1]++;
                            okcnts[bcon]++;
                            /* Deze wordt goed */
                            puzzle.solution[r] |= (1 << p);
                            didfix = 1;
                            /* Over alle rijen gaan voor het geval een rij helemaal leeg zit */
                            for (int rr = 0; rr < NUM_ROWS; rr++) {
                                int rrr = (rr + r) % NUM_ROWS;
                                /* Deze oplossing geldt niet meer: Zoek een pin die niet in current zit en wel in solution */
                                int pc = (puzzle.current[rrr] ^ 0x1f) & puzzle.solution[rrr];
                                int ccnt = bitcnt(pc);
                                if (ccnt > 0) {
                                    ccnt = randint(0, ccnt-1);
                                    for (int i = 0; i < 5; i++) {
                                        if (pc & (1 << i)) {
                                            if (ccnt > 0) {
                                                ccnt--;
                                            } else {
                                                /* En deze niet-goede oplossing is niet meer */
                                                puzzle.solution[rrr] &= ~(1 << i);
                                                break;
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            if (!didfix) {
                /* Safety: Eindeloze lus voorkomen */
                wantok = okcnt;
                repairlevel = ((double)okcnt / 20.0);
                break;
            }
            okcnt++;
        }
    }
}

static void sort_colors(int *colors, int *correct, int count)
{
    for (int i = count-1; i > 0; i--) {
        for (int j = 0; j < i; j++) {
            if (correct[j] < correct[j+1]) {
                int s = correct[j];
                correct[j] = correct[j+1];
                correct[j+1] = s;
                s = colors[j];
                colors[j] = colors[j+1];
                colors[j+1] = s;
            }
        }
    }
}

/* Hint-kleuren laten zien */
static void game_show_colors(clist_t *conns)
{
    int colors[40];
    int *correct = &colors[20];
    /* Kijken voor juiste posities */
    int okcnt = 0;
    for (int r = 0; r < NUM_ROWS; r++) {
        colors[r] = 0;
        correct[r] = 0;
        for (int p = 0; p < 5; p++) {
            /* Kijken of de positie klopt */
            if (puzzle.current[r] & (1 << p)) {
                if (puzzle.solution[r] & (1 << p)) {
                    okcnt++;
                    colors[r]  |= GOOD;
                    correct[r] |= GOOD;
                } else {
                    /* Kleur zetten */
                    colors[r]  |= c_colors[5*r + p];
                }
            } else if (puzzle.solution[r] & (1 << p)) {
                correct[r] |= c_colors[5*r + p];
            }
        }
    }
    if (DIFFICULTY >= 3) {
        sort_colors(colors, correct, 20);
    } else if (DIFFICULTY >= 2) {
        sort_colors(colors, correct, 10);
        sort_colors(colors+10, correct+10, 10);
    }
    led_set_colors(colors);
}

static void game_show_mastermind(clist_t *conns)
{
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
        unsigned char *s = conns->pins[i].p;
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
        unsigned char *s = conns->pins[i].p;
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
}

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

static void plasma_turbine(int ring, double val)
{
    led_set_plasma(ring, 0, plasma_color[ring][0],
            col_fade(val, 4, 0x000000, 0x110000, 0x221100, plasma_color[ring][1]),
            col_fade(val, 4, 0x000000, 0x110000, 0x221100, plasma_color[ring][2]),
            col_fade(val, 4, 0x000000, 0x110000, 0x221100, plasma_color[ring][3]),
            col_fade(val, 4, 0x000000, 0x110000, 0x221100, plasma_color[ring][4]));
}

static void game_setturbine(int sw)
{
    audio_synth_freq_vol(0, 8+sw*2, SPINUP_LOW1 + (SPINUP_FREQ1-SPINUP_LOW1) * turbines[sw], turbines[sw]*(pow(1.0+SPINUP_VOL1, (1.0 - turbines[sw]))-1.0), 1);
    audio_synth_freq_vol(0, 9+sw*2, SPINUP_LOW2 + (SPINUP_FREQ2-SPINUP_LOW2) * turbines[sw], turbines[sw]*(pow(1.0+SPINUP_VOL2, (1.0 - turbines[sw]))-1.0), 1);
    plasma_turbine(spinup_ring[sw], turbines[sw]);
    if (spinup_ring[sw] == 2) plasma_turbine(1, turbines[sw]); /* Middelste is dubbel (2 -> 1)*/
    if (turbines[sw] >= 1.0 || turbines[sw] <= 0.0) {
        led_set_spin(spinup_ring[sw], 0, 0);
    } else {
        led_set_spin(spinup_ring[sw], (int)(turbines[sw] * (double)(SPINUP_RINGSPEED2-SPINUP_RINGSPEED1))+SPINUP_RINGSPEED1, col_fade(turbines[sw], 4, 0x000000, spinup_color[sw], spinup_color[sw], 0x33ffff));
    }
    engine_volume();
}

static void game_doturbines(clist_t *conns)
{
    /* Engine switches */
    int reached = 0;
    int spinning = 0;
    static int prev_running = 0;
    running = 0;
    /* Kijken of de schakelaars zijn omgezet */
    for (int sw = 0; sw < 3; sw++) {
        if (conns->buttons[sw].status & BUTTON_ON) {
            running++;
            if (turbines[sw] < 1.0) {
                /* Turbine spinup: omhooggaand geluid */
                turbines[sw] += SPINUP_SPEED;
                if (turbines[sw] >= 1.0) {
                    turbines[sw] = 1.0;
                    reached = 1;
                } else {
                    spinning = 1;
                }
                game_setturbine(sw);
            }
        } else {
            if (turbines[sw] > 0.0) {
                /* Turbine spindown: omlaaggaand geluid */
                turbines[sw] -= SPINDOWN_SPEED;
                if (turbines[sw] <= 0.0) {
                    turbines[sw] = 0.0;
                    reached = 1;
                } else {
                    spinning = 1;
                }
                game_setturbine(sw);
            }
        }
    }
    if (running != prev_running) {
        prev_running = running;
        engine_hum_set(FRAMERATE*6, FRAMERATE*2, FRAMERATE*9, FRAMERATE*4);
    }
    if (reached == 1 && !spinning) {
        /* Een turbine is net bij zijn eindpunt geraakt, en geen turbine is niet bij zijn eindpunt */
        if (running == 0) {
            /* Alles uit */
            audio_play_file(1, WAV_ENGINE_OFF);
            conns->event |= ENGINE_OFF;
        } else if (running == 3) {
            /* Alles aan */
            audio_play_file(1, WAV_ENGINE_ON);
            conns->event |= ENGINE_ON;
        }
    }
}

void game_mainloop(clist_t *conns)
{
    game_doturbines(conns);
    game_checklevel(conns);
    if (repairing > 0) {
        game_show_colors(conns);
    }
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
