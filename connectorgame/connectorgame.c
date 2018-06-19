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
#include <math.h>
#include <sys/time.h>
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

static int start_time = 0;

int getutime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (start_time == 0) start_time = tv.tv_sec;
    return (((tv.tv_sec - start_time) * 1000000) + tv.tv_usec);
}

int main(int argc, char *argv[])
{
    int cycles = 0;
    int timers[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    setup_handlers();
    init_mcps();
    init_leds();
    init_audio();

    led_set_blobs(0, 3, 0x003300, 0x002211, 0x000033);
    led_set_blobs(3, 4, 0x000033, 0x003300, 0x001133, 0x003311);

    running = 1;
    audio_play_file("booting.wav");
    for (int booting = 0; booting < 10; booting++) {
        int timertime = getutime();
        clist_t *conns = find_connections();
        free(conns);
        leds_mainloop();
        audio_mainloop();
        int sleeptime = SLEEPTIME - (getutime() - timertime);
        if (sleeptime > 0) usleep(sleeptime);
    }
    int flashcount = FRAMERATE*2;
    while (running) {
        int timertime = getutime();
        int subtime = timertime;
        clist_t *conns = find_connections();
        timers[0] += (getutime() - subtime);
        subtime = getutime();
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
        timers[1] += (getutime() - subtime);
        subtime = getutime();
        ledshow_mastermind(0, colcnts[0], poscnts[0]);
        timers[2] += (getutime() - subtime);
        subtime = getutime();
        ledshow_mastermind(1, colcnts[1], poscnts[1]);
        timers[3] += (getutime() - subtime);
        subtime = getutime();
        if (conns->newon > 0) {
            audio_play_file("on.wav");
        } else if (conns->off > 0) {
            audio_play_file("off.wav");
        } else {
            flashcount--;
            if (flashcount <= 0) {
                audio_play_file("spark.wav");
                led_set_flash(0, 5, 0, 2, 0xffffff, 3, 4, 0xccccff, 4, 3, 0xffcccc, 2, 10, 0x000000, 8, 3, 0x000000);
                led_set_flash(3, 5, 0, 3, 0xffccff, 3, 2, 0xffccff, 5, 2, 0xffffff, 2, 6, 0x000000, 12, 2, 0x000000);
                led_set_flash(1, 3, 0, 4, 0xff8888, 6, 2, 0xffffff, 6, 15, 0x000000);
                led_set_flash(2, 3, 0, 3, 0xff8888, 4, 4, 0xffffff, 5, 12, 0x000000);
                flashcount = (int)(((double)(FRAMERATE/10 + (random() % (FRAMERATE * 4)))) * (1.0 + (((double)conns->on)/4)));
            }
        }
        timers[4] += (getutime() - subtime);
        subtime = getutime();
        free(conns);
        timers[5] += (getutime() - subtime);
        subtime = getutime();
        audio_mainloop();
        timers[6] += (getutime() - subtime);
        subtime = getutime();
        leds_mainloop();
        timers[7] += (getutime() - subtime);
        subtime = getutime();
        int sleeptime = SLEEPTIME - (getutime() - timertime);
        if (sleeptime > 0) usleep(sleeptime);
        timers[8] += (getutime() - subtime);
        subtime = getutime();
        timers[9] += (getutime() - timertime);
        cycles++;
    }
    printf("Timer times (%d cycles):\n", cycles);
    printf(" find_connections(): %3.6f\n", (double)timers[0]/cycles/CLOCKS_PER_SEC);
    printf(" count:              %3.6f\n", (double)timers[1]/cycles/CLOCKS_PER_SEC);
    printf(" ledset(0):          %3.6f\n", (double)timers[2]/cycles/CLOCKS_PER_SEC);
    printf(" ledset(1):          %3.6f\n", (double)timers[3]/cycles/CLOCKS_PER_SEC);
    printf(" audio_play_file():  %3.6f\n", (double)timers[4]/cycles/CLOCKS_PER_SEC);
    printf(" free(conns):        %3.6f\n", (double)timers[5]/cycles/CLOCKS_PER_SEC);
    printf(" audio_mainloop():   %3.6f\n", (double)timers[6]/cycles/CLOCKS_PER_SEC);
    printf(" leds_mainloop():    %3.6f\n", (double)timers[7]/cycles/CLOCKS_PER_SEC);
    printf(" usleep():           %3.6f\n", (double)timers[8]/cycles/CLOCKS_PER_SEC);
    printf(" TOTAL:              %3.6f\n", (double)timers[9]/cycles/CLOCKS_PER_SEC);
    fini_leds();
    fini_mcps();
    fini_audio();
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
