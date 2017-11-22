#ifndef __CLIENT__
#define __CLIENT__

#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>

#ifdef __linux__
  #include <arpa/inet.h>
  #include <sys/time.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/select.h>
  #include <openssl/ssl.h>
  #include <openssl/err.h>
  #include <openssl/ocsp.h>
  #include <openssl/x509v3.h>

	#include <SDL2/SDL.h>
	#include <SDL2/SDL_net.h>
	#include <SDL2/SDL_image.h>
#elif __APPLE__
  #include <arpa/inet.h>
  #include <sys/time.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/select.h>
  #include <openssl/ssl.h>
  #include <openssl/err.h>
  #include <openssl/ocsp.h>
  #include <openssl/x509v3.h>

	#include <SDL2/SDL.h>
	#include <SDL2_net/SDL_net.h>
	#include <SDL2_image/SDL_image.h>
#elif __unix__
  #include <arpa/inet.h>
  #include <sys/time.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/select.h>
  #include <openssl/ssl.h>
  #include <openssl/err.h>
  #include <openssl/ocsp.h>
  #include <openssl/x509v3.h>

	#include <SDL2/SDL.h>
	#include <SDL2_net/SDL_net.h>
	#include <SDL2_image/SDL_image.h>
#elif _WIN32
#endif

#define LOGIN_PORT     "30000"
#define LOGIN_HOSTNAME "localhost"

#define BUFSIZE         1024

#define HEADER_PING     "0" 
#define HEADER_LOGIN    "1"

struct b_connection {
  int s;
  char buffer[BUFSIZE+1];
  struct timeval last, latency;
  SSL *ssl;
};

struct b_client {
  fd_set fds;
  struct b_connection *chat, *game;
  SSL_CTX *ctx;
};

/* initialization */
void b_initialize(void);
void b_initialize_openssl(void);
void b_initialize_graphics(void);

/* cleanup */
void b_cleanup(void);
void b_cleanup_openssl(void);
void b_cleanup_graphics(void);

/* the magic */
struct b_connection* b_open_connection(const char *hostname, const char *port);
int b_open_socket(const char *hostname, const char *port);
int b_read_connection(struct b_connection *connection, char *buf);
int b_write_connection(struct b_connection *connection, int count, ...);
int b_client_select(void);
int b_client_handle(void);
int b_client_refresh(void);
int b_client_login(void);
void b_close_connection(struct b_connection **connection);

int ocsp_resp_cb(SSL *s, void *arg);

#endif /* __CLIENT__ */
