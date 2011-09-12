CFLAGS=-Wall -ggdb
CC=g++

all: emu

emu: MemoryRegion.o MemoryBus.o emu.o

emu.o: emu.c

MemoryBus.o: MemoryBus.cc

MemoryRegion.o: MemoryRegion.cc

clean:
	rm -f *.o emu
