#include "game.h"
#include "globals.h"

int main(int argc, char **argv) {
  struct timeval tv;

  b_initialize(argc, argv);

  while (1) {
    int ready = 0;

    if ((ready = b_connection_set_select(&connections)) == -1) {
      perror("b_connection_set_select\n");
      exit(1);
    } else if (b_connection_set_handle(&connections, ready) == -1) {
      perror("b_connection_set_handle\n");
      exit(1);
    }

    gettimeofday(&tv, NULL);

    if ((tv.tv_sec != listener.tv.tv_sec) ||
        (tv.tv_usec > listener.tv.tv_usec+500000) ||
        (listener.tv.tv_usec > tv.tv_usec+500000)) {
      b_player_buffer_update();
      gettimeofday(&(listener.tv), NULL);
    }
 
    b_connection_set_refresh(&connections);
  }

  b_cleanup();

  return 0;
}
