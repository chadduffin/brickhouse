#include "login.h"
#include "globals.h"

// INITIALIZATION //

void b_initialize(void) {
  b_initialize_socket();
  b_initialize_openssl();
}

void b_initialize_socket(void) {
  struct addrinfo
    hints,
    *result;

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
  b_cleanup_socket();
  b_cleanup_openssl();
}

void b_cleanup_socket(void) {
  close(listener.s);
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
    exit(1);
  }

  SSL_set_fd(connection->ssl, connection->s);
  
  SSL_accept(connection->ssl);
  
  return connection;
}

void b_close_connection(struct b_connection **connection) {
  SSL_free((*connection)->ssl);
  close((*connection)->s);
  free((*connection));
}
