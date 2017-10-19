#include "login.h"

struct b_connection_set connections;
struct b_listener listener = { .s = -1, .rp = NULL, .ctx = NULL };
