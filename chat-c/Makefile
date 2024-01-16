CC = gcc
CFLAGS = -g -Wall -Wextra -Werror

all: server_s client_s

tcpc: server_s.c client_s.c
	$(CC) $(CFLAGS) -o server_s server_s.c 
	$(CC) $(CFLAGS) -o client_s client_s.c 


clean:
	rm -rf *.dSYM tcpc client_s server_s