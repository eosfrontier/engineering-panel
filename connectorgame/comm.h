/* Definities voor communicatie met buitenwereld */

#define COMM_PATH "/home/pi/shuttle-panels/www/"
#define COMM_CMD_PATH COMM_PATH "cmd/"

typedef struct {
    char *key;
    double value;
    double min;
    double max;
} setting_t;

extern struct settings {
    setting_t difficulty = { "difficulty", 1, 1, 5 },
    setting_t spinup     = { "spinup",     10, 1, 100 },
    setting_t spindown   = { "spindown",   10, 1, 100 },
    setting_t humvol     = { "humvol",     0.2, 0.01, 1.0 },
    setting_t humvolhi   = { "humvolhi",   0.8, 0.01, 1.0 },
    setting_t humbeat    = { "humbeat",    0.25136, 0.01, 10.0 },
    setting_t hibeat     = { "hibeat",     0.996, 0.01, 10.0 },
    setting_t humbasevar = { "humbasevar", 0.01, 0.0, 5.0 },
} settings;

int init_comm(void);
int comm_read_commands(clist_t *conns);
int comm_write_connections(clist_t *conns);
/* vim: ai:si:expandtab:ts=4:sw=4
 */
