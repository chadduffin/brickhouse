CC=gcc
CFLAGS=-g -Wall -pedantic -std=c11 -O3

OBJECTS=$(patsubst %.c,%.o,$(wildcard *.c))
HEADERS=$(wildcard *.h)

TARGET=client

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -I /usr/local/opt/openssl/include/ -c $< -o $@

all: macintosh

run: clean all
	./$(TARGET)

macintosh: $(OBJECTS)
	$(CC) $(OBJECTS) -L /usr/local/opt/openssl/lib/ -I /Library/Frameworks/ -framework SDL2 -framework SDL2_net -framework SDL2_image -lssl -lcrypto -o $(TARGET)

clean:
	rm -f *.o
	rm -f $(TARGET)
