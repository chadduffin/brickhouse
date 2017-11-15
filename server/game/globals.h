#ifndef __GLOBALS__
#define __GLOBALS__

#include <mysql.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>

#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define TITLE " ______     ______     __    __     ______    \n/\\  ___\\   /\\  __ \\   /\\ \"-./  \\   /\\  ___\\   \n\\ \\ \\__ \\  \\ \\  __ \\  \\ \\ \\-./\\ \\  \\ \\  __\\   \n \\ \\_____\\  \\ \\_\\ \\_\\  \\ \\_\\ \\ \\_\\  \\ \\_____\\ \n  \\/_____/   \\/_/\\/_/   \\/_/  \\/_/   \\/_____/"

extern struct sigaction action;

extern struct b_listener listener;
extern struct b_connection_set connections;

#endif /* __GLOBALS__ */
