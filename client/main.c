#include "client.h"
#include "globals.h"

int main(int argc, char **argv) {
  b_initialize();

  client.chat = b_open_connection(LOGIN_HOSTNAME, LOGIN_PORT);

  b_client_login();
  
  b_cleanup();

  return 0;
}
