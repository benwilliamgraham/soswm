CC = gcc
CFLAGS = -O3 -Wall
LIBS = -lX11
TARGET = soswm

soswm: soswm.c config.c
	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $< 

install: $(TARGET)
	mkdir -p /usr/local/bin
	cp -f $(TARGET) /usr/local/bin/$(TARGET)

uninstall:
	rm /usr/local/bin/$(TARGET)

clean:
	rm $(TARGET) 