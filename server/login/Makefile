CC=gcc
CFLAGS=-g -Wall -pedantic -std=c11 -O3

OBJECTS=$(patsubst %.c,%.o,$(wildcard *.c))
HEADERS=$(wildcard *.h)

TARGET=server

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -I /usr/local/opt/openssl/include/ -I /usr/local/include/mysql -c $< -o $@

all: macintosh

run: clean all
	./$(TARGET)

macintosh: $(OBJECTS)
	$(CC) $(OBJECTS) -L /usr/local/opt/openssl/lib/ -lssl -lcrypto -lmysqlclient -o $(TARGET)

clean:
	rm -f *.o
	rm -f $(TARGET)
