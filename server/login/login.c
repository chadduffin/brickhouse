#include "login.h"
#include "globals.h"

// INITIALIZATION //

void b_initialize(void) {
  FD_ZERO(&(connections.fds));

  b_initialize_socket();
  b_initialize_openssl();
}

void b_initialize_socket(void) {
  struct addrinfo hints, *result;
  struct b_connection *connection;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  
  if ((listener.s = getaddrinfo(NULL, PORT, &hints, &result)) != 0) {
    perror("getaddrlistener\n");
    exit(1);
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
		exit(1);
	}

	if (listen(listener.s, MAXPENDING) == -1) {
		perror("listen\n");
		close(listener.s);
		exit(1);
	}

  connection = (struct b_connection*)malloc(sizeof(struct b_connection));

  connection->s = listener.s;
  connection->ssl = NULL;

  b_connection_set_add(&connections, connection);

	freeaddrinfo(result);
}

void b_initialize_openssl(void) {
  const SSL_METHOD *method;

  SSL_load_error_strings();	
  OpenSSL_add_ssl_algorithms();

  method = TLSv1_2_server_method();

  listener.ctx = SSL_CTX_new(method);

  if (!listener.ctx) {
    perror("SSL_CTX_new\n");
    exit(1);
  }

  SSL_CTX_set_ecdh_auto(listener.ctx, 1);

  if (SSL_CTX_use_PrivateKey_file(listener.ctx, KEYPATH, SSL_FILETYPE_PEM) <= 0) {
    perror("SSL_CTX_use_PrivateKey_file\n");
    exit(1);
  }

  if (SSL_CTX_use_certificate_file(listener.ctx, CERTPATH, SSL_FILETYPE_PEM) <= 0) {
    perror("SSL_CTX_use_certificate_file\n");
    exit(1);
  }
}

// CLEANUP //

void b_cleanup(void) {
  struct b_list_entry *entry = connections.list.head;
  
  while (entry) {
    b_close_connection((struct b_connection**)(&(entry->data)));
    entry = connections.list.head;
  }

  b_cleanup_openssl();

  FD_ZERO(&(connections.fds));
}

void b_cleanup_openssl(void) {
  SSL_CTX_free(listener.ctx);
  EVP_cleanup();
}

// THE MAGIC //

struct b_connection* b_open_connection(void) {
  struct b_connection *connection = (struct b_connection*)malloc(sizeof(struct b_connection));

  if ((connection->s = accept(listener.s, listener.rp->ai_addr, &(listener.rp->ai_addrlen))) < 0) {
    perror("accept\n");
    exit(1);
  }
  
  connection->ssl = SSL_new(listener.ctx);

  if (!connection->ssl) {
    perror("SSL_new\n");
    close(connection->s);
    exit(1);
  }

  if (!SSL_set_fd(connection->ssl, connection->s)) {
    perror("SSL_set_fd\n");
    SSL_free(connection->ssl);
    exit(1);
  }
  
  if (!SSL_accept(connection->ssl)) {
    perror("SSL_accept\n");
    SSL_free(connection->ssl);
    close(connection->s);
    exit(1);
  }

  gettimeofday(&(connection->tv), NULL);

  b_connection_set_add(&connections, connection);
  
  return connection;
}

void b_close_connection(struct b_connection **connection) {
  b_connection_set_remove(&connections, *connection);
  SSL_free((*connection)->ssl);
  close((*connection)->s);
  free((*connection));

  *connection = NULL;
}

int b_read_connection(struct b_connection *connection, char *buf) {
  return SSL_read(connection->ssl, buf, BUFSIZE);
}

int b_write_connection(struct b_connection *connection, const char *buf, unsigned int len) {
  return SSL_write(connection->ssl, buf, len);
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
}

void b_list_remove(struct b_list *list, int key) {
  int max = -1;
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
  struct timeval tv;
  struct b_connection *connection;
  struct b_list_entry *entry = set->list.head;

  gettimeofday(&tv, NULL);

  if (FD_ISSET(listener.s, &(set->fds))) {
    connection = b_open_connection();

    b_write_connection(connection, "You are now connected.\n", 23);
    
    FD_CLR(listener.s, &(set->fds));

    ready -= 1;
  }

  while ((entry) && (ready)) {
    connection = (struct b_connection*)(entry->data);

    entry = entry->next;

    if (FD_ISSET(connection->s, &(set->fds))) {
      FD_CLR(connection->s, &(set->fds));

      if ((s = b_read_connection(connection, connection->buffer)) > 0) {
        connection->buffer[BUFSIZE] = 0;
        printf("%s\n", connection->buffer);
      } else if (s < 0) {
        perror("b_read_connection\n");
        exit(1);
      } else if (!s) {
        printf("connection %i closed.\n", connection->s);
        b_close_connection(&connection);
      }

      ready -= 1;
    } else if ((connection->s != listener.s) && (connection->tv.tv_sec+TIMEOUT < tv.tv_sec)) {
      printf("connection %i timed out.\n", connection->s);
      b_close_connection(&connection);
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
  struct b_list_entry *entry = set->list.head;
  struct b_connection *connection;

  FD_ZERO(&(set->fds));

  while (entry) {
    connection = (struct b_connection*)(entry->data);

    FD_SET(connection->s, &(set->fds));

    entry = entry->next;
  }
}
