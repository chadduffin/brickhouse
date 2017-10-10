#ifndef __LOGIN__
#define __LOGIN__

#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef __linux__
  #include <arpa/inet.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <openssl/ssl.h>
  #include <openssl/err.h>
#elif __APPLE__
  #include <arpa/inet.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <openssl/ssl.h>
  #include <openssl/err.h>
#elif __unix__
  #include <arpa/inet.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <openssl/ssl.h>
  #include <openssl/err.h>
#elif _WIN32
#endif

#define KEYPATH     "../secrets/key.pem"
#define CERTPATH    "../secrets/cert.pem"

#define PORT        "30000"
#define HOSTNAME    "localhost"
#define MAXPENDING  32

struct b_listener {
  int s;
  struct addrinfo *rp;
  SSL_CTX *ctx;
};

struct b_connection {
  int s;
  SSL *ssl;
};

/* initialization */
void b_initialize(void);
void b_initialize_socket(void);
void b_initialize_openssl(void);

/* cleanup */
void b_cleanup(void);
void b_cleanup_socket(void);
void b_cleanup_openssl(void);

/* the magic */
struct b_connection* b_open_connection(void);
void b_close_connection(struct b_connection **connection);

#endif /* __LOGIN__ */
