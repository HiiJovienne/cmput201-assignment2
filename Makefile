CC = gcc
CFLAGS = -Wall -Werror -std=c99

assg:
	$(CC) $(CFLAGS) src/assg.c -o assg

clean:
	rm -f assg