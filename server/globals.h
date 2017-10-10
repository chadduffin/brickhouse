#ifndef __GLOBALS__
#define __GLOBALS__

#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef __linux__
  #include <arpa/inet.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <openssl/ssl.h>
  #include <openssl/err.h>
#elif __APPLE__
  #include <arpa/inet.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <openssl/ssl.h>
  #include <openssl/err.h>
#elif __unix__
  #include <arpa/inet.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <openssl/ssl.h>
  #include <openssl/err.h>
#elif _WIN32
#endif

extern struct b_listener listener;

#endif /* __GLOBALS__ */
