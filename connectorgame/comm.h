/* Definities voor communicatie met buitenwereld */

#define COMM_PATH "/home/pi/shuttle-panels/www/"

int init_comm(void);
int comm_write_connections(clist_t *conns);
