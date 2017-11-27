#include "client.h"
#include "globals.h"

int main(int argc, char **argv) {
  b_initialize();

  client.chat = b_open_connection(LOGIN_HOSTNAME, LOGIN_PORT);

  if (!b_client_login()) {
    while (1) {
      int ready = 0;

      if ((ready = b_client_select()) == -1) {
        perror("b_client_select\n");
        exit(1);
      } else if ((ready) && (b_client_handle())) {
        break;
      }

      b_client_refresh();

      b_render_players();

      if ((client.head) && (client.head->id == 0)) {
        if (client.head->x < 500) {
          client.head->x += 1;
          b_send_player_state(client.head->id, client.head->x, client.head->y);
        }
      }
    }
  }

  b_cleanup();

  return 0;
}
