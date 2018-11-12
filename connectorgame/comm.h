/* Definities voor communicatie met buitenwereld */

#define COMM_PATH "/home/pi/shuttle-panels/www/"
#define COMM_CMD_PATH COMM_PATH "cmd/"

extern struct settings {
    double difficulty;
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
} settings;

int init_comm(void);
int comm_read_commands(clist_t *conns);
int comm_write_connections(clist_t *conns);
/* vim: ai:si:expandtab:ts=4:sw=4
 */
