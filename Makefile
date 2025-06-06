CC = gcc
CFLAGS = -Wall -g
LIBS = -lm

# Objetos da biblioteca
NETWORK_OBJS = network_lib.o libServer.o libClient.o

# Executáveis
TARGETS = server client

all: $(TARGETS)

# Biblioteca de rede
network_lib.o: network_lib.c network_lib.h
	$(CC) $(CFLAGS) -c network_lib.c

libServer.o: libServer.c libServer.h network_lib.h
	$(CC) $(CFLAGS) -c libServer.c

libClient.o: libClient.c libClient.h network_lib.h
	$(CC) $(CFLAGS) -c libClient.c

# Servidor
server: server.c $(NETWORK_OBJS)
	$(CC) $(CFLAGS) -o server server.c $(NETWORK_OBJS) $(LIBS)

# Cliente
client: client.c $(NETWORK_OBJS)
	$(CC) $(CFLAGS) -o client client.c $(NETWORK_OBJS) $(LIBS)

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
