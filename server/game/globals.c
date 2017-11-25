#include "game.h"

struct sigaction action;

struct b_listener listener = { .s = -1, .next_id = 0, .rp = NULL, .ctx = NULL };
struct b_connection_set connections;
