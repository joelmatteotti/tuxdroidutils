#####################################################
## Makefile for the tux bootloader example program
#####################################################

## General Flags
CC = gcc
DEFS = 
CFLAGS = -g -Wall $(DEFS)
LIBS = -lusb
TARGET = tuxup
FILES=main.c \
      bootloader.c \
      bootloader.h \
      usb-connection.c \
      usb-connection.h \
      tux_hid_unix.c \
      tux_hid_unix.h \
      tux-api.h \
      version.h \
      common/commands.h \
      log.c \
      log.h \
      http_request.c \
      http_request.h
OBJECTS=main.c \
	bootloader.c \
	usb-connection.c \
	tux_hid_unix.c \
	log.c \
	http_request.c



all: $(TARGET)
tuxup: $(FILES) 
	${CC} ${LIBS} ${CFLAGS} ${C_INCLUDE_DIRS} ${DEFS} -o tuxup ${OBJECTS} 
	    
clean :
	-rm -f $(TARGET) *.o
