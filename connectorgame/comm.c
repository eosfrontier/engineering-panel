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
#include "main.h"
#include "mcp.h"
#include "comm.h"
#include "game.h"

#define COMM_CONNECTION_FILE COMM_PATH "connections.json"
#define COMM_CONNECTION_FILE_NEW COMM_CONNECTION_FILE ".new"

#define COMM_REPAIR_FILE COMM_CMD_PATH "repair.txt"

extern double turbines[3];
extern double repairlevel;
extern puzzle_t puzzle;

static double lastturbines[3];
static double lastrepairlevel = -1.0;

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
    if (repairlevel != lastrepairlevel) {
        ch = 1;
    }
    lastrepairlevel = repairlevel;
    memcpy(lastturbines, turbines, sizeof(lastturbines));
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
        double rval = strtol(buf, NULL, 10);
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

int comm_read_commands(clist_t *conns)
{
    read_repair_file(conns);
    return 0;
}


/* vim: ai:si:expandtab:ts=4:sw=4
 */
