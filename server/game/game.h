#ifndef __GAME__
#define __GAME__

#include <mysql.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <regex.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>

#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define KEYPATH       "/Users/chadduffin/Desktop/GitHub/brickhouse/secrets/key.pem"
#define CERTPATH      "/Users/chadduffin/Desktop/GitHub/brickhouse/secrets/cert.pem"

#define PORT          "30002"
#define HOSTNAME      "localhost"
#define MAXPENDING    32

#define TIMEOUT       5
#define BUFSIZE       1024

#define PLAYER_INFO   "2"
#define PLAYER_UPDATE "3"

struct b_listener {
  int s;
  unsigned int next_id;
  struct addrinfo *rp;
  SSL_CTX *ctx;
};

struct b_list_entry {
  int key;
  void *data;
  struct b_list_entry *next;
};

struct b_list {
  int max, size;
  struct b_list_entry *head, *tail;
};

struct b_player {
  unsigned int id;
  unsigned short x, y;
};

struct b_connection {
  int s;
  char buffer[BUFSIZE+1];
  struct timeval tv;
  struct b_player player;
  SSL *ssl;
};

struct b_connection_set {
  fd_set fds;
  struct b_list list;
};

/* initialization */
void b_initialize(int argc, char **argv);
void b_initialize_socket(void);
void b_initialize_openssl(void);

/* cleanup */
void b_exit(void);
void b_cleanup(void);
void b_cleanup_openssl(void);

/* the magic */
struct b_connection* b_open_connection(void);
void b_close_connection(struct b_connection **connection);
int b_read_connection(struct b_connection *connection, char *buf);
int b_write_connection(struct b_connection *connection, int count, ...);

void b_list_add(struct b_list *list, void *data, int key);
void b_list_remove(struct b_list *list, int key);
struct b_list_entry* b_list_find(struct b_list *list, int key);

int b_connection_set_select(struct b_connection_set *set);
int b_connection_set_handle(struct b_connection_set *set, unsigned int ready);
void b_connection_set_add(struct b_connection_set *set, struct b_connection *connection);
void b_connection_set_remove(struct b_connection_set *set, struct b_connection *connection);
void b_connection_set_refresh(struct b_connection_set *set);
void b_connection_set_broadcast(struct b_connection_set *set, struct b_connection *source, int count, ...);
void b_connection_initialize(struct b_connection *connection);

void b_prompt(void);
void b_signal_handler(int signal);
void b_command_handler(char *command);

#endif /* __GAME__ */
