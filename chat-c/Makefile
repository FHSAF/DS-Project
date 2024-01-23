# CC = gcc
# CFLAGS = -g -Wall -Wextra -Werror

# all: server_s client_s

# tcpc: server_s.c client_s.c
# 	$(CC) $(CFLAGS) -o server_s server_s.c 
# 	$(CC) $(CFLAGS) -o client_s client_s.c 


# clean:
# 	rm -rf *.dSYM tcpc client_s server_s

CC		= gcc

CFLAGS	= -Wall -Wextra -Werror

sSRC		= server_s.c server_struct.c
cSRC		= client_s.c

sOBJ		= $(sSRC:.c=.o)
cOBJ		= $(cSRC:.c=.o)

sNAME	= server_s
cNAME	= client_s
NAME	= tcpc

${NAME} :	${sOBJ} ${cOBJ}
	@echo "compiling and linking files"
	${CC} ${CFLAGS} -o ${sNAME} ${sOBJ}
	${CC} ${CFLAGS} -o ${cNAME} ${cOBJ}

clean:
	rm -f ${sOBJ}
	rm -f ${cOBJ}

fclean:	clean
	rm -rf ${sNAME} *.dSYM
	rm -rf ${cNAME} *.dSYM

.PHONY: clean fclean