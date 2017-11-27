#include "client.h"

char *username = NULL;

struct b_client client = { .chat = NULL, .game = NULL, .head = NULL, .tail = NULL, .ctx = NULL };

SDL_Event event;
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
