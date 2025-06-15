all: client server

CC = gcc
CFLAGS = -g

# SDL2 flags para o cliente
SDL2_CFLAGS = $(shell pkg-config --cflags sdl2 SDL2_image)
SDL2_LIBS = $(shell pkg-config --libs sdl2 SDL2_image)

# Objetos comuns
OBJS_UTILS = utils.o

# Cliente (com SDL2)
CLIENT_OBJS = client.o $(OBJS_UTILS)
client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(SDL2_LIBS)

client.o: client.c utils.h
	$(CC) $(CFLAGS) $(SDL2_CFLAGS) -c $<

# Servidor (sem SDL2)
SERVER_OBJS = server.o $(OBJS_UTILS)
server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

server.o: server.c utils.h
	$(CC) $(CFLAGS) -c $<

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c $<

# Limpeza
clean:
	rm -f *.o client server

.PHONY: clean
