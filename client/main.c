#include "client.h"
#include "globals.h"

int main(int argc, char **argv) {
  b_initialize();

  client.chat = b_open_connection(LOGIN_HOSTNAME, LOGIN_PORT);

  b_write_connection(client.chat, "test@test.com\0test\0", 19);

  b_client_select();

  b_client_handle();
  
  b_cleanup();

  return 0;
}
