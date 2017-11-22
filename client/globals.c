#include "client.h"

char *username = NULL;

struct b_client client = { .chat = NULL, .game = NULL, .ctx = NULL };

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
