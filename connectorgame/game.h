/* Game functions */

enum gamestates {
    GAME_BOOT,
    GAME_BOOTING,
    GAME_OK,
    GAME_OKING,
    GAME_BREAK,
    GAME_BREAKING,
    GAME_FIX,
    GAME_FIXING,
    GAME_FIXED,
    GAME_FIXEDING
};

#define CONNECTOR_COLORS \
	"BZGYR"	\
	"GYRBZ"	\
	"ZRYGB"	\
	"YBZRG"	\
	"RGYBZ"	\
	"BRGZY"	\
	"YZRGB"	\
	"GBZYR"	\
	"RYBGZ"	\
	"ZGRBY"	\
	"RGYZB" \
	"GZBYR" \
	"ZRGBY" \
	"BYRGZ" \
	"YGBZR" \
	"RBZGY" \
	"ZRGYB" \
	"GYZBR" \
	"BZGRY" \
	"RGYZB"

int game_mainloop(int gamestate, clist_t *conns);
