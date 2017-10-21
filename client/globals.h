#ifndef __GLOBALS__
#define __GLOBALS__

#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

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
#elif _WIN32
#endif

extern struct b_client client;

#endif /* __GLOBALS__ */
