CC = gcc
CFLAGS = -Wall -std=c11 -ggdb3

client: client.c requests.c helpers.c buffer.c parson.c
	$(CC) $(CFLAGS) -o client client.c requests.c helpers.c buffer.c  parson.c

run: client
	./client

clean:
	rm -f *.o client
