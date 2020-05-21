CC = gcc
CFLAGS = -O3 -Wall
LIBS = -lX11 -lXrandr
TARGET = soswm

soswm: soswm.c config.c
	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $< 

install: $(TARGET)
	mkdir -p /usr/local/bin
	cp -f $(TARGET) /usr/local/bin/$(TARGET)
	mkdir -p /usr/share/xsessions
	cp -f $(TARGET).desktop /usr/share/xsessions/$(TARGET).desktop

uninstall:
	-rm /usr/local/bin/$(TARGET)

clean:
	-rm $(TARGET) 
