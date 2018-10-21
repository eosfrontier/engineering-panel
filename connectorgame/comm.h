/* Definities voor communicatie met buitenwereld */

#define COMM_PATH "/home/pi/shuttle-panels/www/"
#define COMM_CMD_PATH COMM_PATH "cmd/"

int init_comm(void);
int comm_read_commands(clist_t *conns);
int comm_write_connections(clist_t *conns);
