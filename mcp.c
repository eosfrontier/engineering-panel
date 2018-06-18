/*
 * Functies om met de MCP23017 te communiceren
 */

#define MCP_IODIR 0x00
#define MCP_GPPU 0x0c
#define MCP_GPIO 0x12

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <wiringPiI2C.h>
#include "mcp.h"

static int mcps[NUM_MCPS];

unsigned char connections[NUM_PINS][NUM_PINS];

static inline void pdebug(const char *format, ...)
{
    va_list args;
    va_start (args, format);
#if REPEATS == 1
    fprintf(stdout, "DBG: ");
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
#endif
    va_end(args);
}

/* Alle MCPs initialiseren */
void init_mcps(void)
{
    memset(connections, 0, sizeof(connections));
    for (int m = 0; m < NUM_MCPS; m++) {
        pdebug("Setting up MCP %d", m);
        mcps[m] = wiringPiI2CSetup(0x20 + m);
        if (mcps[m] < 0) {
            fprintf(stderr, "Unable to start I2C %d: %s\n", m, strerror(errno));
            exit(1);
        }
        /* Alle pins naar input */
        if (wiringPiI2CWriteReg16(mcps[m], MCP_IODIR, 0xffff) < 0) {
            fprintf(stderr, "Unable to setup IODIR I2C %d: %s\n", m, strerror(errno));
            // exit(1);
            continue;
        }
        /* Pullups activeren */
        if (wiringPiI2CWriteReg16(mcps[m], MCP_GPPU, 0xffff) < 0) {
            fprintf(stderr, "Unable to setup PULLUP I2C %d: %s\n", m, strerror(errno));
            exit(1);
        }
        /* Output latch op 0 zetten */
        if (wiringPiI2CWriteReg16(mcps[m], MCP_GPIO, 0x0000) < 0) {
            fprintf(stderr, "Unable to setup OUTPUT I2C %d: %s\n", m, strerror(errno));
            exit(1);
        }
    }
}

void fini_mcps(void)
{
}

/* Van alle io pins kijken welke er met elkaar zijn verbonden */
clist_t *find_connections(void)
{
    for (int ip = 0; ip < NUM_PINS-1; ip++) {
        for (int op = ip; op < NUM_PINS; op++) {
            connections[ip][op] = (connections[ip][op] << 1) & ((1 << PIN_DEBOUNCE) - 1);
        }
    }
    for (int odev = 0; odev < NUM_MCPS; odev++) {
        int vallist[NUM_MCPS];
        if (odev < NUM_MCPS-1) {
            /* Stap 1: Hele MCP op output, andere MCPs checken */
            /* Alle pins naar output */
            pdebug("Outputting all-zeros MCP %d", odev);
            if (wiringPiI2CWriteReg16(mcps[odev], MCP_IODIR, 0x0000) < 0) {
                // fprintf(stderr, "Unable to setup output I2C %d: %s\n", odev, strerror(errno));
                // exit(1);
                continue;
            }
            for (int idev = odev+1; idev < NUM_MCPS; idev++) {
                int numpins = PINS_PER_MCP;
                if (idev == NUM_MCPS-1) {
                    numpins = (NUM_PINS-1) % PINS_PER_MCP + 1;
                }
                pdebug("Scanning all MCP %d", idev);
                if (numpins > 8) {
                    vallist[idev] = wiringPiI2CReadReg16(mcps[idev], MCP_GPIO);
                } else {
                    vallist[idev] = wiringPiI2CReadReg8(mcps[idev], MCP_GPIO);
                }
                if (vallist[idev] < 0) {
                    // fprintf(stderr, "Failed to read pins from mcp %d: %s\n", idev, strerror(errno));
                    // exit(1);
                    vallist[idev] = 0xffff;
                }
                pdebug("  Input MCP %d = %x", idev, vallist[idev]);
            }
            /* Alle pins terug naar input */
            if (wiringPiI2CWriteReg16(mcps[odev], MCP_IODIR, 0xffff) < 0) {
                fprintf(stderr, "Unable to setup input I2C %d: %s\n", odev, strerror(errno));
                exit(1);
            }
            /* Omgekeerd kijken */
            for (int idev = odev+1; idev < NUM_MCPS; idev++) {
                int numpins = PINS_PER_MCP;
                if (idev == NUM_MCPS-1) {
                    numpins = (NUM_PINS-1) % PINS_PER_MCP + 1;
                }
                for (int ireg = 0; ireg < ((numpins+7)/8); ireg++) {
                    int np = 8;
                    if (numpins <= 8 || ireg == 1) {
                        np = (numpins-1) % 8 + 1;
                    }
                    int hit = 0;
                    pdebug("Scanning MCP %d/%d (%d-%d)", idev, ireg, 0, np);
                    for (int ipin = 0; ipin < np; ipin++) {
                        if (!(vallist[idev] & (1 << (ipin + 8*ireg)))) {
                            hit = 1;
                            pdebug("Hit MCP %d/%d/%d, outputting zero", idev, ireg, ipin);
                            if (wiringPiI2CWriteReg8(mcps[idev], MCP_IODIR+ireg, (0x01 << ipin) ^ 0xff) < 0) {
                                fprintf(stderr, "Failed to set output pin Direction (%d/%d/%d): %s\n", idev, ireg, ipin, strerror(errno));
                                exit(1);
                            }
                            pdebug("Scanning MCP %d for hit", odev);
                            int vals = wiringPiI2CReadReg16(mcps[odev], MCP_GPIO);
                            if (vals < 0) {
                                fprintf(stderr, "Failed to read pins from mcp %d: %s\n", odev, strerror(errno));
                                exit(1);
                            }
                            pdebug("  Input MCP %d = %x", odev, vals);
                            for (int op = 0; op < PINS_PER_MCP; op++) {
                                if (!(vals & (1 << op))) {
                                    pdebug("  Hit back MCP %d/%d (from %d/%d/%d)", odev, op, idev, ireg, ipin);
                                    connections[op + (odev * PINS_PER_MCP)][8*ireg + ipin + (idev * PINS_PER_MCP)] |= 0x1;
                                }
                            }
                        }
                    }
                    if (hit) {
                        pdebug("Setting %d/%d back to input", idev, ireg);
                        if (wiringPiI2CWriteReg8(mcps[idev], MCP_IODIR+ireg, 0xff) < 0) {
                            fprintf(stderr, "Failed to set input pin Direction (%d/%d): %s\n", idev, ireg, strerror(errno));
                            exit(1);
                        }
                    }
                }
            }
        }
        /* Op dezelfde MCP scannen */
        int scanpatterns[] = { 0xff00, 0xf0f0, 0xcccc, 0xaaaa };
        int numpins = PINS_PER_MCP;
        if (odev == NUM_MCPS-1) {
            numpins = (NUM_PINS-1) % PINS_PER_MCP + 1;
        }
        pdebug("Scanning intradevice %d (%d pins)", odev, numpins);
        for (int s = (numpins <= 8 ? 1 : 0); s < 4; s++) {
            pdebug("Setting output on %d to pattern %x", odev, scanpatterns[s]);
            if (numpins <= 8) {
                if (wiringPiI2CWriteReg8(mcps[odev], MCP_IODIR, scanpatterns[s] & 0xff) < 0) {
                    fprintf(stderr, "Failed to set IO pins Direction (%d): %s\n", odev, strerror(errno));
                    exit(1);
                }
            } else {
                if (wiringPiI2CWriteReg16(mcps[odev], MCP_IODIR, scanpatterns[s]) < 0) {
                    fprintf(stderr, "Failed to set IO pins Direction (%d): %s\n", odev, strerror(errno));
                    exit(1);
                }
            }
            int vals;
            pdebug("  Scanning input on %d", odev, scanpatterns[s]);
            if (numpins > 8) {
                vals = wiringPiI2CReadReg16(mcps[odev], MCP_GPIO);
            } else {
                vals = wiringPiI2CReadReg8(mcps[odev], MCP_GPIO);
            }
            if (vals < 0) {
                fprintf(stderr, "Failed to read pins from mcp %d: %s\n", odev, strerror(errno));
                exit(1);
            }
            pdebug("Setting %d back to input", odev);
            if (numpins <= 8) {
                if (wiringPiI2CWriteReg8(mcps[odev], MCP_IODIR, 0xff) < 0) {
                    fprintf(stderr, "Failed to set IO pins Direction (%d): %s\n", odev, strerror(errno));
                    exit(1);
                }
            } else {
                if (wiringPiI2CWriteReg16(mcps[odev], MCP_IODIR, 0xffff) < 0) {
                    fprintf(stderr, "Failed to set IO pins Direction (%d): %s\n", odev, strerror(errno));
                    exit(1);
                }
            }
            pdebug("  Input MCP %d = %x", odev, vals);
            /* optput pins op 1 zetten zodat die niet scannen */
            vals |= (scanpatterns[s] ^ 0xffff);
            pdebug("  Input MCP %d (masked) = %x", odev, vals);
            for (int ip = 0; ip < numpins; ip++) {
                if (!(vals & (1 << ip))) {
                    int ipin = ip;
                    int ireg = ip / 8;
                    ipin = ipin % 8;
                    pdebug("Hit on pin %d/%d, setting to output", ireg, ipin);
                    if (wiringPiI2CWriteReg8(mcps[odev], MCP_IODIR+ireg, (0x01 << ipin) ^ 0xff) < 0) {
                        fprintf(stderr, "Failed to set output pin Direction (%d/%d/%d %d): %s\n", ip, odev, ireg, ipin, strerror(errno));
                        exit(1);
                    }
                    int rvals;
                    if (s == 0) {
                        rvals = wiringPiI2CReadReg8(mcps[odev], MCP_GPIO+(1-ireg)) << (8 * (1-ireg));
                        if (rvals < 0) {
                            fprintf(stderr, "Failed to read pins from mcp %d/%d: %s\n", odev, 1-ireg, strerror(errno));
                            exit(1);
                        }
                    } else {
                        rvals = wiringPiI2CReadReg8(mcps[odev], MCP_GPIO+ireg) << (8 * ireg);
                        if (rvals < 0) {
                            fprintf(stderr, "Failed to read pins from mcp %d/%d: %s\n", odev, ireg, strerror(errno));
                            exit(1);
                        }
                    }
                    pdebug("  Input MCP %d = %x", odev, rvals);
                    int tpin = (ip & (0xff << (3-s)));
                    int fpin = (tpin - (1 << (3-s)));
                    pdebug("  Scanning pins %d to %d (for %d step %d)", fpin, tpin, ip, s);
                    for (int op = fpin; op < tpin; op++) {
                        if (!(rvals & (1 << op))) {
                            pdebug("  Hit back MCP %d/%d (from %d/%d)", odev, op, odev, ip);
                            connections[op + (odev * PINS_PER_MCP)][ip + (odev * PINS_PER_MCP)] |= 0x1;
                        }
                    }
                    pdebug("Set pin %d/%d/%d back to input", odev, ireg, ipin);
                    if (wiringPiI2CWriteReg8(mcps[odev], MCP_IODIR+ireg, 0xff) < 0) {
                        fprintf(stderr, "Failed to set input pin Direction (%d/%d/%d): %s\n", odev, ireg, ipin, strerror(errno));
                        exit(1);
                    }
                }
            }
        }
    }
    int on = 0, newon = 0, off = 0;
    /* Ronde 1: alles tellen voor de malloc */
    for (int ip = 0; ip < NUM_PINS; ip++) {
        for (int op = 0; op < NUM_PINS; op++) {
            switch (connections[ip][op]) {
                case PIN_ON:
                    on++;
                    break;
                case PIN_CHANGE_ON:
                    newon++;
                    on++;
                    break;
                case PIN_CHANGE_OFF:
                    off++;
                    break;
            }
        }
    }
    clist_t *conns = malloc(sizeof(clist_t) + (off+on)*sizeof(connection_t));
    conns->on = on;
    conns->off = off;
    conns->newon = newon;
    /* Ronde 2: invullen */
    /* De pinnen zitten op volgorde on, newon, off */
    off = on;
    newon = on - newon; /* = newon-off */
    on = 0;
    for (int ip = 0; ip < NUM_PINS; ip++) {
        for (int op = 0; op < NUM_PINS; op++) {
            switch (connections[ip][op]) {
                case PIN_ON:
                    conns->pins[on].p1 = ip;
                    conns->pins[on].p2 = op;
                    on++;
                    break;
                case PIN_CHANGE_ON:
                    conns->pins[newon].p1 = ip;
                    conns->pins[newon].p2 = op;
                    newon++;
                    break;
                case PIN_CHANGE_OFF:
                    conns->pins[off].p1 = ip;
                    conns->pins[off].p2 = op;
                    off++;
                    break;
            }
        }
    }
    return conns;
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
