#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

int create_socket(int port) {
  int s;
	struct addrinfo
    hints,
    *result,
    *rp;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	if ((s = getaddrinfo("localhost", "30000", &hints, &result)) != 0 ) {
		printf("getaddrinfo\n");
		exit(1);
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
		perror("stream-talk-client: connect");
		exit(1);
	}

	freeaddrinfo(result);

  return s;
}

void init_openssl() { 
  SSL_load_error_strings();	
  OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
  EVP_cleanup();
}

SSL_CTX *create_context() {
  const SSL_METHOD *method;
  SSL_CTX *ctx;

  method = TLSv1_2_client_method();

  ctx = SSL_CTX_new(method);

  if (!ctx) {
    perror("Unable to create SSL context");
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  return ctx;
}

void configure_context(SSL_CTX *ctx) {
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
  SSL_CTX_set_verify_depth(ctx, 1);
  SSL_CTX_load_verify_locations(ctx, "/usr/local/etc/openssl/cert.pem", "/usr/local/etc/openssl/");
  SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION);
}

int main(int argc, char **argv) {
  int sock;
  SSL *ssl;
  SSL_CTX *ctx;
  X509_VERIFY_PARAM *param = NULL;

  init_openssl();
  ctx = create_context();

  configure_context(ctx);

  sock = create_socket(30000);

  ssl = SSL_new(ctx);

  param = SSL_get0_param(ssl);

  X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
  X509_VERIFY_PARAM_set1_host(param, "", 0);

  SSL_set_fd(ssl, sock);
  if (SSL_connect(ssl) <= 0) {
    ERR_print_errors_fp(stderr);
  } else {
    char buff[256];
    memset(buff, 0, sizeof(buff));
    SSL_read(ssl, buff, 255);
    buff[255] = '\0';
    printf("%s\n", buff);
  }

  SSL_write(ssl, "Hello!\0", 7);

  sleep(atoi(argv[1]));

  close(sock);
  SSL_free(ssl);
  SSL_CTX_free(ctx);
  cleanup_openssl();
}
