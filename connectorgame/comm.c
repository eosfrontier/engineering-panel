/*
 * Communiceren met de buitenwereld
 */

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#include "main.h"
#include "mcp.h"
#include "comm.h"
#include "game.h"

#define COMM_CONNECTION_FILE COMM_PATH "connections.json"
#define COMM_CONNECTION_FILE_NEW COMM_CONNECTION_FILE ".new"

#define COMM_REPAIR_FILE COMM_CMD_PATH "repair.txt"
#define COMM_SETTINGS_FILE COMM_PATH "settings.txt"

extern double turbines[3];
extern double repairlevel;
extern puzzle_t puzzle;

static double lastturbines[3];
static double lastrepairlevel = -1.0;
static puzzle_t lastpuzzle;

int init_comm(void)
{
    return 0;
}

int comm_write_connections(clist_t *conns)
{
    int ch = 0;
    for (int sw = 0; sw < NUM_BUTTONS; sw++) {
        if (conns->buttons[sw].status & BUTTON_CHANGED) {
            ch = 1;
            break;
        }
    }
    if (ch == 0 && memcmp(lastturbines, turbines, sizeof(lastturbines))) {
        ch = 1;
    }
    if (ch == 0 && memcmp(&lastpuzzle, &puzzle, sizeof(lastpuzzle))) {
        ch = 1;
    }
    if (fabs(repairlevel - lastrepairlevel) > 0.01) {
        ch = 1;
    }
    lastrepairlevel = repairlevel;
    memcpy(lastturbines, turbines, sizeof(lastturbines));
    memcpy(&lastpuzzle, &puzzle, sizeof(lastpuzzle));
    if (ch == 0 && conns->newon == 0 && conns->off == 0) {
        return 0;
    }
    FILE *f = fopen(COMM_CONNECTION_FILE_NEW, "w");
    fprintf(f, "{\"timestamp\":%llu,\"switches\":[", getutime());
    for (int sw = 0; sw < NUM_BUTTONS; sw++) { fprintf(f, "%s%d", sw > 0 ? "," : "", conns->buttons[sw].status); }
    fprintf(f, "],\"turbines\":[");
    for (int sw = 0; sw < 3; sw++) { fprintf(f, "%s%f", sw > 0 ? "," : "", turbines[sw]); }
    fprintf(f, "],\"repairlevel\":%f", repairlevel);
    fprintf(f, ",\"num_connections\":%d,\"connections\":[", conns->on);
    for (int cn = 0; cn < conns->on; cn++) {
        fprintf(f, "%s[%d,%d]", cn > 0 ? "," : "", conns->pins[cn].p[0], conns->pins[cn].p[1]);
    }
    fprintf(f, "],\"rows\":[");
    for (int s = 0; s < NUM_ROWS; s++) {
        fprintf(f, "%s[%d,%d]", s > 0 ? "," : "", puzzle.current[s], puzzle.solution[s]);
    }
    fprintf(f, "]}");
    fclose(f);
    pdebug("Wrote file %s", COMM_CONNECTION_FILE_NEW);
    rename(COMM_CONNECTION_FILE_NEW, COMM_CONNECTION_FILE);
    return 0;
}

static int read_repair_file(clist_t *conns)
{
    FILE *f = fopen(COMM_REPAIR_FILE, "r");
    if (!f) return 0;
    char buf[100];
    int r;
    r = fread(buf, 1, sizeof(buf)-1, f);
    fclose(f);
    if (r > 0) {
        buf[r] = 0; 
        errno = 0;
        double rval = strtod(buf, NULL);
        if (!errno) {
            if (rval < 0.0) rval = 0.0;
            if (rval > 1.0) rval = 1.0;
            repairlevel = rval;
            conns->event |= REPAIR;
            unlink(COMM_REPAIR_FILE);
            pdebug("Read file %s, setting repair level to %f", COMM_REPAIR_FILE, repairlevel);
        }
    }
    return 0;
}

struct settings settings;

static int read_settings_file(clist_t *conns)
{
    FILE *f = fopen(COMM_SETTINGS_FILE, "r");
    if (!f) {
        fprintf(stderr, "Failed to read settings file %s: %s\n", COMM_SETTINGS_FILE, strerror(errno));
        return -1;
    }
    char key[101];
    double value;
    setting_t *settinglist = settings;
    while (fscanf(f, " %100s = %lf", key, &value) == 2) {
        for (int s = 0; s < sizeof(settings)/sizeof(setting_t); s++) {
            if (!strcmp(key, settinglist[s].key)) {
                if (value < settinglist[s].min) value = settinglist[s].min;
                if (value > settinglist[s].max) value = settinglist[s].max;
                *(settinglist[s].value) = value;
            }
        }
    }
    fclose(f);

    return 0;
}

int comm_read_commands(clist_t *conns)
{
    read_repair_file(conns);
    return 0;
}


/* vim: ai:si:expandtab:ts=4:sw=4
 */
