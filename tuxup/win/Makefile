#####################################################
## Makefile for the tux bootloader example program
#####################################################

## General Flags
	CC = "gcc.exe"
	DEFS = -DWIN32
	C_INCLUDE_DIRS = -I"C:\MinGW\include\ddk"
	CFLAGS = -pipe -Wall -funsigned-char -g2 -O0
	LIBS = -lwinmm -lhid -lsetupapi -lhidparse 

TARGET = tuxup

## Libraries

## Compile and link
SRC_OBJS = main.o bootloader.o tux_hid_win32.o tux_misc.o log.o

define build_target
@echo Linking...
@$(CC) -o "$(TARGET)" $(SRC_OBJS) $(LIB_DIRS) $(LIBS) $(LDFLAGS)
endef

define compile_source
@echo Compiling $<
@$(CC) $(CFLAGS) $(C_PREPROC) $(C_INCLUDE_DIRS) -c "$<" -o "$@"
endef

.PHONY: print_header
$(TARGET): print_header $(SRC_OBJS)
	$(build_target)
	
print_header:
	@echo Start compiling

main.o: main.c \
tux-api.h \
bootloader.h \
version.h \
tux_misc.h \
error.h \
log.h \
tux_hid_win32.h
	$(compile_source)

bootloader.o: bootloader.c \
tux_misc.h \
tux_hid_win32.h \
error.h \
log.h \
tux-api.h
	$(compile_source)

tux_hid_win32.o: tux_hid_win32.c \
tux_hid_win32.h	\
tux_misc.h
	$(compile_source)

tux_misc.o: tux_misc.c \
tux_misc.h
	$(compile_source)

log.o: log.c \
log.h
	$(compile_source)
 
all: $(TARGET)

clean :
	-rm -f $(TARGET) *.o
