/* Game functions */

enum event {
    ENGINE_OFF = 1,
    ENGINE_ON = 2,
    REPAIR = 4,
    HUMSETTING = 8
};

typedef struct {
    int solution[NUM_ROWS];
    int current[NUM_ROWS];
    int solcount[5];
    int curcount[5];
} puzzle_t;

#define BLACK      0
#define BLUE       1
#define GREEN      2
#define YELLOW     3
#define RED        4
#define GOOD       0x20
#define BAD        0x40

void init_game(void);
void game_mainloop(clist_t *conns);
/* vim: ai:si:expandtab:ts=4:sw=4
 */
