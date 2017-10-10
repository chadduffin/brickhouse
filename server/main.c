#include "login.h"
#include "globals.h"

int main(int argc, char **argv) {
  const char buffer[] = { "hello" };

  b_initialize();

  while (1) {
    struct b_connection *connection = b_open_connection();
    
    SSL_write(connection->ssl, buffer, sizeof(buffer)+1);

    b_close_connection(&connection);
  }

  b_cleanup();
}
