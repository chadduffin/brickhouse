#include "client.h"
#include "globals.h"

// INITIALIZATION //

void b_initialize(void) {
  FD_ZERO(&(client.fds));

  client.chat = NULL;
  client.game = NULL;

  b_initialize_openssl();
}

void b_initialize_openssl(void) {
  const SSL_METHOD *method;

  SSL_load_error_strings();	
  OpenSSL_add_ssl_algorithms();

  method = TLSv1_2_client_method();

  client.ctx = SSL_CTX_new(method);

  if (!client.ctx) {
    perror("SSL_CTX_net\n");
    exit(EXIT_FAILURE);
  }

  SSL_CTX_set_verify(client.ctx, SSL_VERIFY_PEER, NULL);
  SSL_CTX_set_verify_depth(client.ctx, 1);
  SSL_CTX_load_verify_locations(client.ctx, "../secrets/certifications", "../secrets/");
  SSL_CTX_set_options(client.ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION);
}

void b_initialize_graphics(void) {
  window = SDL_CreateWindow("Client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);

  if (window) {
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
      SDL_DestroyWindow(window);
      b_cleanup();
      exit(EXIT_FAILURE);
    }
  }
}

// CLEANUP //

void b_cleanup(void) {
  if (client.chat) {
    b_close_connection(&(client.chat));
  }

  if (client.game) {
    b_close_connection(&(client.game));
  }

  b_cleanup_openssl();

  b_cleanup_graphics();

  FD_ZERO(&(client.fds));
}

void b_cleanup_openssl(void) {
  SSL_CTX_free(client.ctx);
  EVP_cleanup();
}

void b_cleanup_graphics(void) {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

// THE MAGIC //

struct b_connection* b_open_connection(const char *hostname, const char *port) {
  struct b_connection *connection = (struct b_connection*)malloc(sizeof(struct b_connection));
  X509_VERIFY_PARAM *param;
  BIO *arg = BIO_new_fp(stdout, BIO_NOCLOSE);

  memset(connection->buffer, 0, BUFSIZE+1);

  connection->s = b_open_socket(hostname, port);

  connection->ssl = SSL_new(client.ctx);

  param = SSL_get0_param(connection->ssl);

  X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
  X509_VERIFY_PARAM_set1_host(param, "", 0);

  // Online Certificate Status Protocol
  SSL_set_tlsext_status_type(connection->ssl, TLSEXT_STATUSTYPE_ocsp);
  SSL_CTX_set_tlsext_status_cb(client.ctx, ocsp_resp_cb);
  SSL_CTX_set_tlsext_status_arg(client.ctx, arg);

  SSL_set_fd(connection->ssl, connection->s);

  if (SSL_connect(connection->ssl) <= 0) {
    perror("SSL_connect\n");
    exit(EXIT_FAILURE);
  }

  return connection;
}

int b_open_socket(const char *hostname, const char *port) {
  int s;
	struct addrinfo hints, *result, *rp;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((s = getaddrinfo(hostname, port, &hints, &result))) {
		printf("getaddrinfo\n");
		exit(EXIT_FAILURE);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1 ) {
			continue;
		}

		if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;
		}

		close(s);
	}

	if (!rp) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

  FD_SET(s, &(client.fds));

	freeaddrinfo(result);

  return s;
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

int b_client_select(void) {
  int max = -1;
  struct timeval tv;

  tv.tv_sec = 2;
  tv.tv_usec = 0;

  if (client.chat) {
    max = client.chat->s;
  }

  if (client.game) {
    max = (max > client.game->s) ? max : client.game->s;
  }

  return (max == -1) ? -1 : select(max+1, &(client.fds), NULL, NULL, &tv);
}

int b_client_handle(void) {
  int s;

  if ((client.chat) && (FD_ISSET(STDIN_FILENO, &(client.fds)))) {
    char buffer[32];

    fgets(buffer, 32, stdin);

    buffer[strlen(buffer)-1] = 0;
    
    if (buffer[0] != 0) {
      b_write_connection(client.chat, 2, username, buffer);
    }

    FD_CLR(STDIN_FILENO, &(client.fds));
  }

  if ((client.chat) && (FD_ISSET(client.chat->s, &(client.fds)))) {
    if ((s = b_read_connection(client.chat, client.chat->buffer)) < 0) {
      perror("b_read_connection\n");
      exit(EXIT_FAILURE);
    } else if (s == 0) {
      printf("Connection with chat server closed.\n");
      return -1;
    } else {
      int i = strlen(client.chat->buffer)+1;

      printf("%s: %s\n", client.chat->buffer, client.chat->buffer+i);
    }

    FD_CLR(client.chat->s, &(client.fds));
  }

  if ((client.game) && (FD_ISSET(client.game->s, &(client.fds)))) {
    if ((s = b_read_connection(client.game, client.game->buffer)) < 0) {
      perror("b_read_connection\n");
      exit(EXIT_FAILURE);
    } else if (s == 0) {
      printf("Connection with game server closed.\n");
      return -1;
    } else {
      b_handle_client_buffer(client.game);
    }

    FD_CLR(client.game->s, &(client.fds));
  }

  return 0;
}

int b_client_refresh(void) {
  FD_ZERO(&(client.fds));

  FD_SET(STDIN_FILENO, &(client.fds));

  if (client.chat) {
    FD_SET(client.chat->s, &(client.fds));
  }
  
  if (client.game) {
    FD_SET(client.game->s, &(client.fds));
  }

  return 0;
}

int b_client_login(void) {
  int i, j;
  char email[64],
       password[64];
  struct b_connection *connection;

  printf("email: ");
  scanf("%s", email);
  printf("password: ");
  scanf("%s", password);

  b_write_connection(client.chat, 3, HEADER_LOGIN, email, password);

  b_client_select();

  b_client_handle();

  connection = client.chat;

  if (!strcmp("err", connection->buffer)) {
    perror("invalid login\n");
    return -1;
  }

  i = strlen(connection->buffer)+1;
  j = strlen(connection->buffer+i)+i+1;

  if (!(client.chat = b_open_connection(connection->buffer+i, connection->buffer+j))) {
    b_close_connection(&(client.chat));
    b_close_connection(&connection);
    return -1;
  }

  printf("Connected to %s:%s (chat).\n", connection->buffer+i, connection->buffer+j);

  i = strlen(connection->buffer+j)+j+1;
  j = strlen(connection->buffer+i)+i+1;

  if (!(client.game = b_open_connection(connection->buffer+i, connection->buffer+j))) {
    b_close_connection(&(client.chat));
    b_close_connection(&(client.game));
    b_close_connection(&connection);
    return -1;
  }

  printf("Connected to %s:%s (game).\n", connection->buffer+i, connection->buffer+j);

  b_close_connection(&connection);

  email[strcspn(email, "@")] = 0;

  username = (char*)malloc(strlen(email)+1);

  strncpy(username, email, strlen(email)+1);

  b_initialize_graphics();

  return 0;
}

void b_close_connection(struct b_connection **connection) {
  if ((!connection) || (!(*connection))) {
    return;
  }

  if ((*connection)->ssl) {
    SSL_free((*connection)->ssl);
  }

  close((*connection)->s);
  free(*connection);

  *connection = NULL;
}

void b_handle_client_buffer(struct b_connection *connection) {
  const char *buf = client.game->buffer;

  if (!strcmp(PLAYER_UPDATE, buf)) {
    
  } else if (!strcmp(PLAYER_INFO, buf)) {
    unsigned int len = ntohl(*((unsigned int*)(connection->buffer+2)));

    for (unsigned int i = 0; i < len; i++) {
      b_add_player(ntohl(*((unsigned int*)(connection->buffer+(i*8)+6))));
      client.tail->x = ntohs(*((unsigned short*)(connection->buffer+(i*8)+10)));
      client.tail->y = ntohs(*((unsigned short*)(connection->buffer+(i*8)+12)));
    }
  }
}

void b_add_player(unsigned int id) {
  struct b_player *player = (struct b_player*)malloc(sizeof(struct b_player));

  player->id = id;
  player->x = id%640;
  player->y = id%480;
  player->next = NULL;

  if (!client.tail) {
    client.head = client.tail = player;
  } else {
    client.tail->next = player;
    client.tail = player;
  }
}

struct b_player* b_find_player(unsigned int id) {
  struct b_player *player = client.head;

  while ((player) && (player->id != id)) {
    player = player->next;
  }

  return player;
}

void b_remove_player(unsigned int id) {
  struct b_player *player = client.head,
                  **indirect = &(client.head);

  while ((*indirect) && ((*indirect)->id != id)) {
    player = player->next;
    indirect = &player;
  }

  *indirect = (*indirect)->next;

  free(player);
}

void b_update_player(unsigned int id, unsigned short x, unsigned short y) {
  struct b_player *player = b_find_player(id);

  if (!player) {
    b_add_player(id);
    player = client.tail;
  }

  player->x = x;
  player->y = y;
}

int ocsp_resp_cb(SSL *s, void *arg) {
  const unsigned char *p;
  int len;
  OCSP_RESPONSE *rsp;
  len = SSL_get_tlsext_status_ocsp_resp(s, &p);
  //BIO_puts(arg, "OCSP response: ");
  if (!p) {
    //BIO_puts(arg, "no response sent\n");
    return 1;
  }

  rsp = d2i_OCSP_RESPONSE(NULL, &p, len);

  if (!rsp) {
    BIO_puts(arg, "response parse error\n");
    BIO_dump_indent(arg, (char *)p, len, 4);
    return 0;
  }

  BIO_puts(arg, "\n======================================\n");
  OCSP_RESPONSE_print(arg, rsp, 0);
  BIO_puts(arg, "======================================\n");
  OCSP_RESPONSE_free(rsp);

  return 1;
}
