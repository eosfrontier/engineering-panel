/*
 * Shuttle spel: connector game
 *
 * Doet de hardware-aansturing van het shuttle paneel met de connectors
 *
 * Shuttlepaneel bevat: 4 led-ringen van 24 rgb-pizels
 *                      100 connectors verbonden aan 7 MCP23017 chips (GPIO)
 *                      Geluidskaartje met speakers
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include "mcp.h"
#include "leds.h"
#include "audio.h"

#define FRAMERATE 50
#define SLEEPTIME (1000000/FRAMERATE)

static uint8_t running = 1;

static void ctrl_c_handler(int signum)
{
    (void)(signum);
    running = 0;
}

static void setup_handlers(void)
{
    struct sigaction sa =
    {
        .sa_handler = ctrl_c_handler,
    };

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int main(int argc, char *argv[])
{
    setup_handlers();
    init_mcps();
    init_leds();
    init_audio();

    running = 1;
    while (running) {
        int timertime = clock();
        clist_t *conns = find_connections();
        int colcnts[2] = {0,0};
        int poscnts[2] = {0,0};
        for (int i = 0; i < conns->on; i++) {
            int s1 = (PIN_ROW(conns->pins[i].p1) >= 10);
            int s2 = (PIN_ROW(conns->pins[i].p2) >= 10);
            if (s1 == s2) {
                poscnts[s1]++;
                poscnts[s2]++;
            } else {
                colcnts[s1]++;
                colcnts[s2]++;
            }
        }
        ledshow_mastermind(0, colcnts[0], poscnts[0]);
        ledshow_mastermind(1, colcnts[1], poscnts[1]);
        if (conns->newon > 0) {
            audio_play_file("on.raw");
        } else if (conns->off > 0) {
            audio_play_file("off.raw");
        }
        free(conns);
        audio_mainloop();
        usleep(SLEEPTIME - (((clock() - timertime) * 1000000) / CLOCKS_PER_SEC));
    }
    fini_leds();
    fini_mcps();
    fini_audio();
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
