#include "login.h"
#include "globals.h"

int main(int argc, char **argv) {
  b_initialize();

  while (1) {
    int ready = 0;

    if ((ready = b_connection_set_select(&connections)) == -1) {
      perror("b_connection_set_select\n");
      exit(1);
    } else if (b_connection_set_handle(&connections, ready) == -1) {
      perror("b_connection_set_handle\n");
      exit(1);
    }

    b_connection_set_refresh(&connections);
  }

  b_cleanup();
}
