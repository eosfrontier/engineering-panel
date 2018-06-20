/* Definities voor MCP23017 chip aansturing */

#define NUM_PINS 100
#define PINS_PER_MCP 15
#define PINS_PER_ROW 5
#define PIN_ROW(p) (((p) / PINS_PER_ROW) % 20)
#define NUM_ROWS ((NUM_PINS+PINS+PER_ROW-1)/PINS_PER_ROW)
#define NUM_MCPS ((NUM_PINS+PINS_PER_MCP-1)/PINS_PER_MCP)

#define PIN_DEBOUNCE 8
#define PIN_ON ((1 << (PIN_DEBOUNCE)) - 1)
#define PIN_CHANGE_ON ((1 << (PIN_DEBOUNCE-1)) - 1)
#define PIN_CHANGE_OFF ((1 << (PIN_DEBOUNCE)) - 2)

typedef struct connection {
    unsigned char p1, p2;
} connection_t;

typedef struct clist {
    int on, newon, off;
    connection_t pins[0];
} clist_t;

// extern unsigned char connections[NUM_PINS][NUM_PINS];
void init_mcps(void);
void fini_mcps(void);
clist_t *find_connections(void);

/* vim: ai:si:expandtab:ts=4:sw=4
 */
