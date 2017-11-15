#include "login.h"
#include "globals.h"

// INITIALIZATION //

void b_initialize(int argc, char **argv) {
  if (argc < 4) {
    perror("argv\n");
    exit(EXIT_FAILURE);
  }

  printf("%s\n\n", TITLE);

  FD_ZERO(&(connections.fds));

  FD_SET(STDIN_FILENO, &(connections.fds));

  b_initialize_mysql(argv);
  b_initialize_socket();
  b_initialize_openssl();
  b_prompt();

  action.sa_handler = b_signal_handler;
  sigaction(SIGINT, &action, NULL);
}

void b_initialize_mysql(char **argv) {
  mysql.con = mysql_init(NULL);

  if (!mysql.con) {
    fprintf(stderr, "mysql_init: %s\n", mysql_error(mysql.con));
    exit(EXIT_FAILURE);
  }

  if (!mysql_real_connect(mysql.con, argv[1], argv[2], argv[3], "brickhouse", 3306, NULL, 0)) {
    fprintf(stderr, "mysql_real_connect: %s\n", mysql_error(mysql.con));
    mysql_close(mysql.con);
    exit(EXIT_FAILURE);
  }

  /*
    b_mysql_query("CREATE DATABASE brickhouse");
    b_mysql_query("CREATE TABLE accounts(Id INT PRIMARY KEY AUTO_INCREMENT, Email TEXT, Password TEXT, Salt TEXT, Token TEXT)");
    b_mysql_query("INSERT INTO accounts(Email, Password, Salt, Token) VALUES(\"salt@test.com\", \"test\", \"wGlQBekQRsnp3l0wE8vyrqmJnlWZX37gk1D4s63S9jZwmORvugVJ19oeUn3H00BL\", \"MLLd6rSBqH35BjlMnSjLIGIWHEbmrhwRqe2HmskBr4lhNbBKVH3Q9IrjttE4q6w0\")");
  */
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

  mysql_close(mysql.con);

  FD_ZERO(&(connections.fds));
}

void b_cleanup_mysql(void) {
  mysql_close(mysql.con);
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

int b_verify_connection(struct b_connection *connection) {
  char query[BUFSIZE+1], digest[BUFSIZE+1], password[BUFSIZE+1];
  unsigned int e_len, p_len;
  MYSQL_ROW row;
  MYSQL_RES *result;

  e_len = strlen(connection->buffer);
  p_len = strlen(connection->buffer+e_len);

  if (!b_verify_email(connection->buffer)) {
    return 0;
  }

  snprintf(query, BUFSIZE, "SELECT * FROM accounts WHERE Email=\"%s\";", connection->buffer); 

  b_mysql_query(query);

  result = mysql_store_result(mysql.con);

  if ((result) && (row = mysql_fetch_row(result))) {
    char *salt = row[3];

    PKCS5_PBKDF2_HMAC(connection->buffer+e_len+1, strlen(connection->buffer+e_len+1), (unsigned char*)salt, 64, 4096, EVP_sha512(), 64, (unsigned char*)digest);

    p_len = mysql_real_escape_string(mysql.con, password, digest, 64);

    password[p_len] = 0;

    p_len = mysql_real_escape_string(mysql.con, digest, row[2], 64);

    digest[p_len] = 0;

    if (!strcmp(password, digest)) {
      b_write_connection(connection, 3, row[4], CHAT_HOSTNAME, CHAT_PORT);
      return 1;
    }
  }

  return 0;
}

int b_verify_email(const char *email) {
  regex_t regex;
  const char *pattern = "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,4}$";

  if (regcomp(&regex, pattern, REG_EXTENDED)) {
    perror("regcomp\n");
    return 0;
  }

  return !regexec(&regex, email, 0, NULL, 0);
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

        if (!b_verify_connection(connection)) {
          b_write_connection(connection, 1, "err");
        }

        b_close_connection(&connection);
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

    if ((connection->s != listener.s) && (connection->tv.tv_sec+TIMEOUT < tv.tv_sec)) {
      printf("connection %i timed out.\n", connection->s);
      b_close_connection(&connection);
      b_prompt();
    } else {
      FD_SET(connection->s, &(set->fds));
    }
  }
}

void b_prompt(void) {
  printf("login$ ");
  fflush(stdout);
}

void b_mysql_query(const char *query) {
  if (mysql_query(mysql.con, query)) {
    fprintf(stderr, "mysql_query: %s\n", mysql_error(mysql.con));
    exit(EXIT_FAILURE);
  }
}

void b_signal_handler(int signal) {
  perror("b_signal_handler\n");
  b_exit();
}

void b_command_handler(char *command) {
  int i, len, argc = 0;
  char *begin = command;

  if (!strcmp(command, "exit")) {
    b_exit();
  } else if (!strcmp(command, "version")) {
    printf("Login 0.0.1 October 21 2017\n");
  } else {
    len = strlen(command); 

    while (len > 0) {
      i = strcspn(command, " ");

      *(command+i) = 0;

      command = command+i+1;

      len -= i+1;

      argc += 1;
    }

    if ((argc > 1) && (!strcmp(begin, "create"))) {
      len = strlen(begin);

      if ((argc == 5) && (!strcmp(begin+len+1, "account"))) {
        char *email = begin+len+1+strlen(begin+len+1)+1,
             *password = email+strlen(email)+1,
             *salt = password+strlen(password)+1;
        b_create_account(email, password, salt);
      }
    }
  }

  b_prompt();
}

void b_create_account(const char *email, const char *password, const char *salt) {
  int len;
  char query[BUFSIZE+1], digest[BUFSIZE+1], hashed_password[BUFSIZE+1];

  if (!b_verify_email(email)) {
    return;
  }

  PKCS5_PBKDF2_HMAC(password, strlen(password), (unsigned char*)salt, 64, 4096, EVP_sha512(), 64, (unsigned char*)digest);

  len = mysql_real_escape_string(mysql.con, hashed_password, digest, 64);

  hashed_password[len] = 0;

  snprintf(query, BUFSIZE, "INSERT INTO accounts(Email, Password, Salt, Token) VALUES(\"%s\", \"%s\", \"%s\", \"%s\");", email, hashed_password, salt, salt); 

  b_mysql_query(query);

  return;
}
