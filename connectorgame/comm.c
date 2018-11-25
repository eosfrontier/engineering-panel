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

#define COMM_PROFILE_FILE COMM_PATH "profile.json"
#define COMM_PROFILE_FILE_NEW COMM_PROFILE_FILE ".new"

#define COMM_SETTINGS_FILE COMM_PATH "settings.json"
#define COMM_SETTINGS_FILE_NEW COMM_SETTINGS_FILE ".new"

#define COMM_REPAIR_FILE COMM_CMD_PATH "repair.txt"
#define COMM_SETTINGS_PATH COMM_PATH "settings/"

extern double turbines[3];
extern double repairlevel;
extern puzzle_t puzzle;

static double lastturbines[3];
static double lastrepairlevel = -1.0;
static puzzle_t lastpuzzle;

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
    if (!f) {
        fprintf(stderr, "Failed to open %s: %s\n", COMM_CONNECTION_FILE_NEW, strerror(errno));
        return -1;
    }
    fprintf(f, "{\"timestamp\":%llu,\"switches\":[", getutime());
    for (int sw = 0; sw < NUM_BUTTONS; sw++) { fprintf(f, "%s%d", sw > 0 ? "," : "", conns->buttons[sw].status); }
    fprintf(f, "],\"turbines\":[");
    for (int sw = 0; sw < 3; sw++) { fprintf(f, "%s%g", sw > 0 ? "," : "", turbines[sw]); }
    fprintf(f, "],\"repairlevel\":%g", repairlevel);
    fprintf(f, ",\"num_connections\":%d,\"connections\":[", conns->on);
    for (int cn = 0; cn < conns->on; cn++) {
        fprintf(f, "%s[%d,%d]", cn > 0 ? "," : "", conns->pins[cn].p[0], conns->pins[cn].p[1]);
    }
    fprintf(f, "],\"rows\":[");
    for (int s = 0; s < NUM_ROWS; s++) {
        fprintf(f, "%s[%d,%d]", s > 0 ? "," : "", puzzle.current[s], puzzle.solution[s]);
    }
    fprintf(f, "]}");
    if (fclose(f) < 0) {
        fprintf(stderr, "Failed to write %s: %s\n", COMM_CONNECTION_FILE_NEW, strerror(errno));
        return -1;
    }
    pdebug("Wrote file %s", COMM_CONNECTION_FILE_NEW);
    if (rename(COMM_CONNECTION_FILE_NEW, COMM_CONNECTION_FILE) < 0) {
        fprintf(stderr, "Failed to rename %s: %s\n", COMM_CONNECTION_FILE_NEW, strerror(errno));
        return -1;
    }
    return 0;
}

int comm_write_profile(double profiling[16])
{
    FILE *f = fopen(COMM_PROFILE_FILE_NEW, "w");
    if (!f) {
        fprintf(stderr, "Failed to open %s: %s\n", COMM_PROFILE_FILE_NEW, strerror(errno));
        return -1;
    }
    fprintf(f, "{\"timestamp\":%llu,\"profile\":[", getutime());
    for (int i = 1; i < 8; i++) {
        fprintf(f, "%s{\"time\":%g,\"cpu\":%g}", (i > 1 ? "," : ""), profiling[i*2], profiling[i*2+1]);
    }
    fprintf(f, "]}");
    if (fclose(f) < 0) {
        fprintf(stderr, "Failed to write %s: %s\n", COMM_PROFILE_FILE_NEW, strerror(errno));
        return -1;
    }
    pdebug("Wrote file %s", COMM_PROFILE_FILE_NEW);
    if (rename(COMM_PROFILE_FILE_NEW, COMM_PROFILE_FILE) < 0) {
        fprintf(stderr, "Failed to rename %s: %s\n", COMM_PROFILE_FILE_NEW, strerror(errno));
        return -1;
    }
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
            pdebug("Read file %s, setting repair level to %g", COMM_REPAIR_FILE, repairlevel);
        }
    }
    return 0;
}

struct settings settings;

struct {
    char *key;
    double *variable;
    double min;
    double max;
    double dflt;
    int event;
    char *description;
} settingfiles[] = {
    { "difficulty", &settings.difficulty, 1, 5, 1, 0, "Puzzle Difficulty Level" },
    { "gamemode",   &settings.gamemode, 1, 2, 1, 0, "Puzzle Game Mode" },
    { "spinup",     &settings.spinup, 1, 100, 10, 0, "Spinup time (sec)" },
    { "spindown",   &settings.spindown, 1, 100, 20, 0, "Spindown time (sec)" },
    { "humfreq",    &settings.humfreq, 1, 500, 100, HUMSETTING, "Base hum frequency" },
    { "turbinefreq",&settings.turbinefreq, 1, 500, 10, HUMSETTING, "Frequency step for off switch" },
    { "repairfreq", &settings.repairfreq, 1, 500, 5, HUMSETTING, "Max frequency step for breakage" },
    { "humvol",     &settings.humvol, 0.01, 1.0, 0.2, HUMSETTING, "Base hum volume" },
    { "humvolhi",   &settings.humvolhi, 0.01, 1.0, 0.8, HUMSETTING, "Harmonics hum volume" },
    { "humbeat",    &settings.humbeat, 0.01, 10.0, 0.2503, HUMSETTING, "Frequency step for base hum" },
    { "hibeat",     &settings.hibeat, 0.01, 10.0, 0.996, HUMSETTING, "Frequency multiplier for harmonics" },
    { "humbasevar", &settings.humbasevar, 0.0, 5.0, 0.01, HUMSETTING, "Variance in base frequencies" },
    { "spinvol1"  , &settings.spinvol1, 0.0, 3.0, 2.0, 0, "Spinup volume 1" },
    { "spinvol2",   &settings.spinvol2, 0.0, 3.0, 2.0, 0, "Spinup volume 2" },
    { "spinlow1",   &settings.spinlow1, 0.0, 500.0, 90, 0, "Spinup start frequency 1" },
    { "spinlow2",   &settings.spinlow2, 0.0, 500.0, 80, 0, "Spinup start frequency 2" },
    { "spinfreq1",  &settings.spinfreq1, 0.0, 2000.0, 400, 0, "Spinup end frequency 1" },
    { "spinfreq2",  &settings.spinfreq2, 0.0, 2000.0, 410, 0, "Spinup end frequency 2" },
    { "spinspeed1", &settings.spinspeed1, 1.0, 20000.0, 2000, 0, "Spinup ledspin start speed" },
    { "spinspeed2", &settings.spinspeed2, 1.0, 20000.0, 200, 0, "Spinup ledspin end speed" },
    { "decaytime",  &settings.decaytime, 0.0, 500.0, 4.0, 0, "Hours to 100% breakdown (0 = off)" },
    { "breakdown",  &settings.breakdown, 0.0, 1.0, 0.2, 0, "Total breakdown at level" },
    { "breakspeed", &settings.breakspeed, 0.001, 100.0, 1.0, 0, "Turbine spindown time at breakdown (sec)" },
};

static int write_settings(void)
{
    FILE *f = fopen(COMM_SETTINGS_FILE_NEW, "w");
    if (!f) {
        fprintf(stderr, "Failed to open %s: %s\n", COMM_SETTINGS_FILE_NEW, strerror(errno));
        return -1;
    }
    fprintf(f, "{\"settings\":[");
    for (unsigned int s = 0; s < sizeof(settingfiles)/sizeof(*settingfiles); s++) {
        fprintf(f, "%s{\"key\":\"%s\",\"value\":%g,\"min\":%g,\"max\":%g,"
                "\"default\":%g,\"description\":\"%s\"}",
                (s>0?",":""), settingfiles[s].key,
                *(settingfiles[s].variable),
                settingfiles[s].min, settingfiles[s].max,
                settingfiles[s].dflt, settingfiles[s].description);
    }

    fprintf(f, "]}");
    if (fclose(f) < 0) {
        fprintf(stderr, "Failed to write %s: %s\n", COMM_SETTINGS_FILE_NEW, strerror(errno));
        return -1;
    }
    pdebug("Wrote file %s", COMM_SETTINGS_FILE_NEW);
    if (rename(COMM_SETTINGS_FILE_NEW, COMM_SETTINGS_FILE) < 0) {
        fprintf(stderr, "Failed to rename %s: %s\n", COMM_SETTINGS_FILE_NEW, strerror(errno));
        return -1;
    }
    return 0;
}

static int read_settings_file(clist_t *conns)
{
    static int reread = 0;
    if (reread > 0) {
        reread--;
        return 0;
    }
    reread = FRAMERATE/4;
    char pathname[strlen(COMM_SETTINGS_PATH)+100];
    int changed = 0;
    for (unsigned int s = 0; s < sizeof(settingfiles)/sizeof(*settingfiles); s++) {
        strcpy(pathname, COMM_SETTINGS_PATH);
        strcat(pathname, settingfiles[s].key);
        FILE *f = fopen(pathname, "r");
        double value;
        if (fscanf(f, "%lf", &value) == 1) {
            if (value < settingfiles[s].min) value = settingfiles[s].min;
            if (value > settingfiles[s].max) value = settingfiles[s].max;
            if (*(settingfiles[s].variable) != value) {
                changed = 1;
                pdebug("Changed setting %s to %g", settingfiles[s].key, value);
                *(settingfiles[s].variable) = value;
                conns->event |= settingfiles[s].event;
            }
        } else {
            fprintf(stderr, "Error reading setting file %s: %s", pathname, strerror(errno));
        }
        fclose(f);
    }
    if (changed) write_settings();
    return 0;
}

int comm_read_commands(clist_t *conns)
{
    read_repair_file(conns);
    read_settings_file(conns);
    return 0;
}

int init_comm(void)
{
    return 0;
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
