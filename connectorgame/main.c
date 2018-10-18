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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>
#include "mcp.h"
#include "leds.h"
#include "audio.h"
#include "game.h"
#include "comm.h"
#include "main.h"

static int running = 1;
int debugging = 0;

void pdebug(const char *format, ...)
{
    va_list args;
    va_start (args, format);
    if (debugging) {
        fprintf(stdout, "DBG: ");
        vfprintf(stdout, format, args);
        fprintf(stdout, "\n");
        fflush(stdout);
    }
    va_end(args);
}

static void ctrl_c_handler(int signum)
{
    (void)(signum);
    running = 0;
}

/* Ctrl-C afvangen */
static void setup_handlers(void)
{
    struct sigaction sa =
    {
        .sa_handler = ctrl_c_handler,
    };

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/* Tijd ophalen in microseconden */
int64_t getutime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((((int64_t)tv.tv_sec) * 1000000) + (int64_t)tv.tv_usec);
}

int main(int argc, char *argv[])
{
    if (argc > 1 && argv[1][0] == 'd') {
        debugging = 1;
    }

    srandom(time(NULL));

    /* Alles initen */
    setup_handlers();
    init_mcps();
    init_leds();
    init_audio();
    init_game();

    /* Opstarten */
    running = 1;
    int scanrate = SCANRATE;
    while (running) {
        int64_t timertime = getutime(); // Om de framerate gelijk te houden
        if (--scanrate <= 0) {
            scanrate = SCANRATE;
            clist_t *conns = find_connections(); // Altijd uitlezen
            game_mainloop(conns);
            comm_write_connections(conns);
            free(conns);
        }
        leds_mainloop();
        audio_mainloop();
        int64_t sleeptime = SLEEPTIME - (getutime() - timertime);
        if (sleeptime > 0) usleep(sleeptime);
    }
    fini_leds();
    fini_mcps();
    fini_audio();
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
