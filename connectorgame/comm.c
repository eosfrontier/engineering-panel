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
#include "main.h"
#include "mcp.h"

#define COMM_CONNECTION_FILE "/home/pi/shuttle-panels/www/connections.json"
#define COMM_CONNECTION_FILE_NEW (COMM_CONNECTION_FILE ".new")

int init_comm(void)
{
    return 0;
}

int comm_write_connections(clist_t *conns)
{
    if (conns->newon == 0 && conns->off == 0) {
        return 0;
    }
    FILE *f = fopen(COMM_CONNECTION_FILE_NEW, 'w');
    fprintf(f, "{\"switches\":[");
    for (int sw = 0; sw < 3; sw++) { fprintf(f, "%s%d", sw > 0 ? ',' : '', conns->buttons[sw].status); }
    fprintf(f, "],\"num_connections\":%d,\"ok_connections\":%d,\"connections\":[", conns->okcnt);
    for (int cn = 0; cn < conns->on; cn++) {
        fprintf(f, "%s[%d,%d]", conns->pins[cn].p1, conns->pins[cn].p2, cn > 0 ? ',' : '');
    }
    fprintf(f, "]}");
    fclose(f);
    rename(COMM_CONNECTION_FILE_NEW, COMM_CONNECTION_FILE);
    return 0;
}


/* vim: ai:si:expandtab:ts=4:sw=4
 */
