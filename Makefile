CC = gcc
CFLAGS = -O3 -Wall -Wextra -pedantic -ansi -std=c11
LIBS = -lX11 -lpthread
SERVER = soswm
CLIENT = sosc

soswm: wm.c server.c communication.h
	$(CC) $(CFLAGS) $(LIBS) -o $(SERVER) wm.c server.c

sosc: client.c communication.h
	$(CC) $(CFLAGS) $(LIBS) -o $(CLIENT) client.c

install: $(SERVER) $(CLIENT)
	mkdir -p /usr/local/bin
	cp -f $(SERVER) /usr/local/bin/$(SERVER)
	cp -f $(CLIENT) /usr/local/bin/$(CLIENT)
	mkdir -p /usr/share/xsessions
	cp -f $(SERVER).desktop /usr/share/xsessions/$(SERVER).desktop

format:
	clang-format -i *.c *.h
	clang-tidy *.c *.h -fix -checks="-*,readability-*" --
