#include "login.h"

struct sigaction action;

struct b_mysql mysql = { .con = NULL, .result = NULL };
struct b_listener listener = { .s = -1, .rp = NULL, .ctx = NULL };
struct b_connection_set connections;
