#include "game.h"
#include "globals.h"

// INITIALIZATION //

void b_initialize(int argc, char **argv) {
  printf("%s\n\n", TITLE);

  FD_ZERO(&(connections.fds));

  FD_SET(STDIN_FILENO, &(connections.fds));

  connections.list.size = 0;

  b_initialize_socket();
  b_initialize_openssl();
  b_prompt();

  action.sa_handler = b_signal_handler;
  sigaction(SIGINT, &action, NULL);
}

void b_initialize_socket(void) {
  struct addrinfo hints, *result;
  struct b_connection *connection;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  
  if ((listener.s = getaddrinfo(NULL, PORT, &hints, &result)) != 0) {
    perror("getaddrinfo\n");
    exit(EXIT_FAILURE);
  }

	for (listener.rp = result; listener.rp; listener.rp = listener.rp->ai_next) {
		if ((listener.s = socket(listener.rp->ai_family, listener.rp->ai_socktype, listener.rp->ai_protocol)) == -1 ) {
			continue;
		}

		if (!bind(listener.s, listener.rp->ai_addr, listener.rp->ai_addrlen)) {
			break;
		}

		close(listener.s);
	}

	if (!listener.rp) {
		perror("bind\n");
		exit(EXIT_FAILURE);
	}

	if (listen(listener.s, MAXPENDING) == -1) {
		perror("listen\n");
		close(listener.s);
		exit(EXIT_FAILURE);
	}

  connection = (struct b_connection*)malloc(sizeof(struct b_connection));

  connection->s = listener.s;
  connection->ssl = NULL;

  b_connection_set_add(&connections, connection);
}

void b_initialize_openssl(void) {
  const SSL_METHOD *method;

  SSL_load_error_strings();	
  OpenSSL_add_ssl_algorithms();

  method = TLSv1_2_server_method();

  listener.ctx = SSL_CTX_new(method);

  if (!listener.ctx) {
    perror("SSL_CTX_new\n");
    exit(EXIT_FAILURE);
  }

  SSL_CTX_set_ecdh_auto(listener.ctx, 1);

  if (SSL_CTX_use_PrivateKey_file(listener.ctx, KEYPATH, SSL_FILETYPE_PEM) <= 0) {
    perror("SSL_CTX_use_PrivateKey_file\n");
    exit(EXIT_FAILURE);
  }

  if (SSL_CTX_use_certificate_file(listener.ctx, CERTPATH, SSL_FILETYPE_PEM) <= 0) {
    perror("SSL_CTX_use_certificate_file\n");
    exit(EXIT_FAILURE);
  }
}

// CLEANUP //

void b_exit(void) {
  b_cleanup();
  exit(EXIT_SUCCESS);
}

void b_cleanup(void) {
  struct b_list_entry *entry = connections.list.head;
  
  while (entry) {
    b_connection_set_remove(&connections, (struct b_connection*)(entry->data));
    entry = connections.list.head;
  }

  b_cleanup_openssl();

  FD_ZERO(&(connections.fds));
}

void b_cleanup_openssl(void) {
	freeaddrinfo(listener.rp);
  SSL_CTX_free(listener.ctx);
  EVP_cleanup();
}

// THE MAGIC //

struct b_connection* b_open_connection(void) {
  struct b_connection *connection = (struct b_connection*)malloc(sizeof(struct b_connection));

  if ((connection->s = accept(listener.s, listener.rp->ai_addr, &(listener.rp->ai_addrlen))) < 0) {
    perror("accept\n");
    exit(EXIT_FAILURE);
  }

  memset(connection->buffer, 0, BUFSIZE+1);
  
  connection->ssl = SSL_new(listener.ctx);

  if (!connection->ssl) {
    perror("SSL_new\n");
    close(connection->s);
    exit(EXIT_FAILURE);
  }

  if (!SSL_set_fd(connection->ssl, connection->s)) {
    perror("SSL_set_fd\n");
    SSL_free(connection->ssl);
    exit(EXIT_FAILURE);
  }

  if (SSL_accept(connection->ssl)) {
    gettimeofday(&(connection->tv), NULL);
    b_connection_set_add(&connections, connection);

    // update players of this addition
    b_player_create(connection);
  } else {
    b_close_connection(&connection);
  }
  
  return connection;
}

void b_close_connection(struct b_connection **connection) {
  b_connection_set_remove(&connections, *connection);

  if ((*connection)->ssl) {
    SSL_free((*connection)->ssl);
  }

  b_player_destroy(*connection);

  close((*connection)->s);
  free((*connection));

  *connection = NULL;
}

int b_read_connection(struct b_connection *connection, char *buf) {
  return SSL_read(connection->ssl, buf, BUFSIZE);
}

int b_write_connection(struct b_connection *connection, int count, ...) {
  int i = 0, len = 0;
  char buffer[BUFSIZE+1];
  va_list list;

  va_start(list, count); 
  
  for (i = 0; i < count; i += 1) {
    len += 1+snprintf(buffer+len, BUFSIZE-len,"%s", va_arg(list, const char *));
  }

  va_end(list);

  return SSL_write(connection->ssl, buffer, len);
}

void b_list_add(struct b_list *list, void *data, int key) {
  struct b_list_entry *entry = (struct b_list_entry*)malloc(sizeof(struct b_list_entry)), *indirect = entry;

  entry->key = key;
  entry->data = data;
  entry->next = list->head;

  while ((indirect->next) && (indirect->next->key < key)) {
    indirect = indirect->next;
  }

  entry->next = indirect->next;

  if (indirect->next == list->head) {
    list->head = entry;
  } else {
    indirect->next = entry; 
  }

  list->max = (list->max > key) ? list->max : key;

  connections.list.size += 1;
}

void b_list_remove(struct b_list *list, int key) {
  int max = 0;
  struct b_list_entry *entry, **indirect;

  if (!list->head) {
    return;
  }

  indirect = &(list->head);

  while ((*indirect)->key != key) {
    max = (*indirect)->key;
    indirect = &((*indirect)->next);

    if (!(*indirect)) {
      return;
    }
  }

  if (list->max == key) {
    list->max = max;
  }
 
  entry = *indirect;

  *indirect = entry->next;

  list->size -= 1;

  free(entry);
}

struct b_list_entry* b_list_find(struct b_list *list, int key) {
  struct b_list_entry *head = list->head;

  while (head) {
    if (head->key == key) {
      break;
    }
    
    head = head->next;
  }

  return head;
}

int b_connection_set_select(struct b_connection_set *set) {
  struct timeval tv;

  tv.tv_sec = 2;
  tv.tv_usec = 0;

  return select(set->list.max+1, &(set->fds), NULL, NULL, &tv);
}

int b_connection_set_handle(struct b_connection_set *set, unsigned int ready) {
  int s;
  char buffer[BUFSIZE+1];
  struct b_connection *connection;
  struct b_list_entry *entry = set->list.head;

  if (FD_ISSET(listener.s, &(set->fds))) {
    connection = b_open_connection();

    FD_CLR(listener.s, &(set->fds));

    ready -= 1;
  }

  if (FD_ISSET(STDIN_FILENO, &(set->fds))) {
    fgets(buffer, BUFSIZE, stdin);

    buffer[strcspn(buffer, "\n\r")] = 0;

    b_command_handler(buffer);

    FD_CLR(STDIN_FILENO, &(set->fds));

    ready -= 1;
  }

  while ((entry) && (ready)) {
    connection = (struct b_connection*)(entry->data);

    entry = entry->next;

    if (FD_ISSET(connection->s, &(set->fds))) {
      FD_CLR(connection->s, &(set->fds));

      if ((s = b_read_connection(connection, connection->buffer)) > 0) {
        connection->buffer[BUFSIZE] = 0;

        b_connection_set_broadcast_raw(&connections, connection, connection->buffer, s);

        memset(connection->buffer, 0, BUFSIZE+1);
      } else if (s < 0) {
        perror("b_read_connection\n");
        exit(EXIT_FAILURE);
      } else if (!s) {
        printf("connection %i closed.\n", connection->s);
        b_close_connection(&connection);
        b_prompt();
      }

      ready -= 1;
    }
  }

  return 0;
}

void b_connection_set_add(struct b_connection_set *set, struct b_connection *connection) {
  b_list_add(&(set->list), (void*)connection, connection->s);

  FD_SET(connection->s, &(set->fds));
}

void b_connection_set_remove(struct b_connection_set *set, struct b_connection *connection) {
  b_list_remove(&(set->list), connection->s);

  FD_CLR(connection->s, &(set->fds));
}

void b_connection_set_refresh(struct b_connection_set *set) {
  struct timeval tv;
  struct b_list_entry *entry = set->list.head;
  struct b_connection *connection;

  gettimeofday(&tv, NULL);

  FD_ZERO(&(set->fds));

  FD_SET(STDIN_FILENO, &(set->fds));

  while (entry) {
    connection = (struct b_connection*)(entry->data);

    entry = entry->next;

    FD_SET(connection->s, &(set->fds));
  }
}

void b_connection_set_broadcast(struct b_connection_set *set, struct b_connection *source, int count, ...) {
  int i = 0, len = 0;
  char buffer[BUFSIZE+1];
  struct b_list_entry *entry = set->list.head;
  struct b_connection *connection = NULL;
  va_list list;

  va_start(list, count); 

  for (i = 0; i < count; i += 1) {
    len += 1+snprintf(buffer+len, BUFSIZE-len,"%s", va_arg(list, const char *));
  }

  va_end(list);

  while (entry) {
    connection = (struct b_connection*)(entry->data);

    if ((connection->s != listener.s) && (connection->s != source->s) &&
        ((len = SSL_write(connection->ssl, buffer, len)) <= 0)) {
     
    }

    entry = entry->next;
  }
}

void b_connection_set_broadcast_raw(struct b_connection_set *set, struct b_connection *source, void *buffer, int len) {
  struct b_list_entry *entry = set->list.head;
  struct b_connection *connection = NULL;

  while (entry) {
    connection = (struct b_connection*)(entry->data);

    if ((connection->s != listener.s) && (connection->s != source->s) &&
        ((len = SSL_write(connection->ssl, buffer, len)) <= 0)) {

    }

    entry = entry->next;
  }
}

void b_player_create(struct b_connection *connection) {
  int i = 0, len = ((connections.list.size-1)*8)+4;
  char *data = calloc(1, len);
  struct b_list_entry *entry = connections.list.head;
  struct b_player *player = &(connection->player);

  player->id = listener.next_id++;
  player->x = player->id%640;
  player->y = player->id%480;

  // add the header
  strcpy(data, PLAYER_INFO);

  // add # of players
  *((unsigned int*)(data+i+2)) = htonl((unsigned int)(connections.list.size-1));
  
  // add the player data
  while (player) {
    *((unsigned int*)(data+i+6)) = htonl(player->id);
    *((unsigned short*)(data+i+10)) = htonl(player->x);
    *((unsigned short*)(data+i+12)) = htonl(player->y);

    i += 8;

    do {
      entry = entry->next;
    } while ((entry) && ((entry->key == listener.s) || (((struct b_connection*)(entry->data))->player.id == listener.next_id-1)));

    player = (entry) ? &(((struct b_connection*)(entry->data))->player) : NULL;
  }

  if (SSL_write(connection->ssl, data, len) <= 0) {
    b_close_connection(&connection);
    free(data);
    return;
  }

  data = realloc(data, 15);

  strcpy(data, PLAYER_INFO);

  unsigned int size = 1;

  *((unsigned int*)(data+2)) = htonl(size);
  *((unsigned int*)(data+6)) = htonl(connection->player.id);
  *((unsigned short*)(data+10)) = htonl(connection->player.x);
  *((unsigned short*)(data+12)) = htonl(connection->player.y);

  b_connection_set_broadcast_raw(&connections, connection, data, 15);

  free(data);
}

void b_player_destroy(struct b_connection *connection) {
  char *data = (char*)calloc(1, 7);

  strcpy(data, PLAYER_REMOVE);

  *((unsigned int*)(data+2)) = htonl(connection->player.id);

  b_connection_set_broadcast_raw(&connections, connection, data, 7);

  free(data);
}

void b_prompt(void) {
  printf("game$ ");
  fflush(stdout);
}

void b_signal_handler(int signal) {
  perror("b_signal_handler\n");
  b_exit();
}

void b_command_handler(char *command) {
  int i, len, argc = 0;

  if (!strcmp(command, "exit")) {
    b_exit();
  } else if (!strcmp(command, "version")) {
    printf("Game 0.0.1 November 7 2017\n");
  } else {
    len = strlen(command); 

    while (len > 0) {
      i = strcspn(command, " ");

      *(command+i) = 0;

      command = command+i+1;

      len -= i+1;

      argc += 1;
    }
  }

  b_prompt();
}
