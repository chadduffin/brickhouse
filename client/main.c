#include "client.h"
#include "globals.h"

int main(int argc, char **argv) {
  b_initialize();

  client.chat = b_open_connection(LOGIN_HOSTNAME, LOGIN_PORT);

  b_client_select();

  b_client_handle();

  b_write_connection(client.chat, "Hello!", 6);

  b_close_connection(&(client.chat));

  b_cleanup();

  return 0;
}
