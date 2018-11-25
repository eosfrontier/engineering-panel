/* Definities voor communicatie met buitenwereld */

#define COMM_PATH "/home/pi/shuttle-panels/www/"
#define COMM_CMD_PATH COMM_PATH "cmd/"

extern struct settings {
    double difficulty;
    double gamemode;
    double spinup;
    double spindown;
    double humfreq;
    double turbinefreq;
    double repairfreq;
    double humvol;
    double humvolhi;
    double humbeat;
    double hibeat;
    double humbasevar;
    double spinvol1;
    double spinvol2;
    double spinlow1;
    double spinlow2;
    double spinfreq1;
    double spinfreq2;
    double spinspeed1;
    double spinspeed2;
    double decaytime;
    double breakdown;
    double breakspeed;
} settings;

int init_comm(void);
int comm_read_commands(clist_t *conns);
int comm_write_connections(clist_t *conns);
/* vim: ai:si:expandtab:ts=4:sw=4
 */
