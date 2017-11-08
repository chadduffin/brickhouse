#include "client.h"
#include "globals.h"

int main(int argc, char **argv) {
  b_initialize();

  client.chat = b_open_connection(LOGIN_HOSTNAME, LOGIN_PORT);

  b_client_login();

  while (1) {
    int ready = 0;

    if ((ready = b_client_select()) == -1) {
      perror("b_client_select\n");
      exit(1);
    } else if ((ready) && (b_client_handle())) {
      perror("b_client_handle\n");
      exit(1);
    }

    b_client_refresh();
  }
  
  b_cleanup();

  return 0;
}
