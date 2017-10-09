#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int create_socket(int port) {
  int s;
  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  s = socket(AF_INET, SOCK_STREAM, 0);

  if (s < 0) {
    perror("Unable to create socket");
    exit(EXIT_FAILURE);
  }

  if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("Unable to bind");
    exit(EXIT_FAILURE);
  }

  if (listen(s, 1) < 0) {
    perror("Unable to listen");
    exit(EXIT_FAILURE);
  }

  return s;
}

int main(int argc, char **argv) {
  int sock;
  SSL_CTX *ctx;
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();
  const SSL_METHOD *method = TLSv1_2_server_method();

  if (!(ctx = SSL_CTX_new(method))) {
    exit(1);
  }

  SSL_CTX_set_ecdh_auto(ctx, 1);

  if (SSL_CTX_use_certificate_file(ctx, "../secrets/cert.pem", SSL_FILETYPE_PEM) <= 0) {
    exit(1);
  }

  if (SSL_CTX_use_PrivateKey_file(ctx, "../secrets/key.pem", SSL_FILETYPE_PEM) <= 0) {
    exit(1);
  }

  sock = create_socket(5555);

  while(1) {
    int nsock;
    SSL *ssl;

    nsock = accept(sock, NULL, NULL);

    ssl = SSL_new(ctx);

    SSL_set_fd(ssl, nsock);

    if (SSL_accept(ssl) <= 0) {
      ERR_print_errors_fp(stderr);
    } else {
      SSL_write(ssl, "hello", 6);
    }

    SSL_free(ssl);

    close(nsock);
  }

  close(sock);
  SSL_CTX_free(ctx);
  EVP_cleanup();
}
