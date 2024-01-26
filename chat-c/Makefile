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

CFLAGS	= -Wextra -Werror

sSRC		= server_s.c server_struct.c socket_changes.c
cSRC		= client_s.c

sOBJ		= $(sSRC:.c=.o)
cOBJ		= $(cSRC:.c=.o)
LIBS		= -liphlpapi -lws2_32
sNAME	= srvr
cNAME	= client_s
NAME	= tcpc

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

${NAME} :	${sOBJ} ${cOBJ}
	@echo "compiling and linking files"
	${CC} ${CFLAGS} -o ${sNAME} ${sOBJ} ${LIBS}
	${CC} ${CFLAGS} -o ${cNAME} ${cOBJ} ${LIBS}

clean:
	del /F ${sOBJ}
	del /F ${cOBJ}

fclean:	clean
	del /F ${sNAME}.exe
	del /F ${cNAME}.exe

.PHONY: clean fclean