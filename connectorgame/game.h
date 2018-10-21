/* Game functions */

enum gamestates {
    GAME_START,
    GAME_STARTING,
    GAME_BOOT,
    GAME_BOOTING,
    GAME_OK,
    GAME_OKING,
    GAME_BREAK,
    GAME_BREAKING,
    GAME_COLOR,
    GAME_COLORING,
    GAME_MASTERMIND,
    GAME_MASTERMINDING,
    GAME_BALANCE,
    GAME_BALANCEING,
    GAME_FIXED,
    GAME_FIXEDING,
    GAME_RESTART
};

enum event {
    ENGINE_OFF = 1,
    ENGINE_ON = 2,
    REPAIR = 4
};

#define BLACK      0x01
#define BLUE       0x02
#define GREEN      0x04
#define YELLOW     0x08
#define RED        0x10
#define GOOD       0x20
#define BAD        0x40

void init_game(void);
void game_mainloop(clist_t *conns);
/* vim: ai:si:expandtab:ts=4:sw=4
 */
