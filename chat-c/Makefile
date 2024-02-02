# CC = gcc
# CFLAGS = -g -Wall -Wextra -Werror

# all: server_s client_s

# tcpc: server_s.c client_s.c
# 	$(CC) $(CFLAGS) -o server_s server_s.c 
# 	$(CC) $(CFLAGS) -o client_s client_s.c 

#  gcc -Wall -Wextra -Werror -o srvr.exe server_s.c server_struct.c -liphlpapi -lws2_32
# clean:
# 	rm -rf *.dSYM tcpc client_s server_s

CC		= gcc

CFLAGS	= -Wall -Wextra -Werror

sSRC		= server_s.c server_struct.c socket_changes.c election_lcr.c chat_client.c disconnection_handler.c \
			socket_setup.c udp.c multicast.c service_discovery.c
cSRC		= client/client_s.c

sOBJ		= $(sSRC:.c=.o)
cOBJ		= $(cSRC:.c=.o)
sNAME	= srvr
cNAME	= client_s
NAME	= tcpc

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

${NAME} :	${sOBJ} ${cOBJ}
	@echo "compiling and linking files"
	${CC} ${CFLAGS} -o ${sNAME} ${sOBJ}
	${CC} ${CFLAGS} -o ${cNAME} ${cOBJ}

clean:
	rm -f ${sOBJ}
	rm -f ${cOBJ}

fclean:	clean
	rm -f ${sNAME}
	rm -f ${cNAME}

.PHONY: clean fclean