CC = gcc
CFLAGS = -O3 -Wall -Wextra -pedantic -ansi -std=c11
LIBS = -lX11 -lXrandr
SERVER = soswm
CLIENT = sosc

soswm: soswm.c server.c
	$(CC) $(CFLAGS) $(LIBS) -o $(SERVER) $^

sosc: sosc.c
	$(CC) $(CFLAGS) $(LIBS) -o $(CLIENT) $^

format:
	clang-format -i *.c *.h
	clang-tidy *.c *.h -fix -checks="-*,readability-*" --
