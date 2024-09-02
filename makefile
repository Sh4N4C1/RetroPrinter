CC = x86_64-w64-mingw32-gcc
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
EXECUTABLE = RetroPrinter.exe
OPTIONS := -Iinclude -s -w -static -Os -fpermissive

all: 
	$(CC) $(SRCS) $(OPTIONS) -o $(EXECUTABLE)
	x86_64-w64-mingw32-strip --strip-unneeded $(EXECUTABLE)
debug:
	$(CC) $(SRCS) $(OPTIONS) -o $(EXECUTABLE) -DPRINTER -DDEBUG
	x86_64-w64-mingw32-strip --strip-unneeded $(EXECUTABLE)
