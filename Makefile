CFLAGS=-Wall -ggdb
CC=g++

all: emu

emu: Machine.o MemoryRegion.o MemoryBus.o MemoryScreen.o emu.o

emu.o: emu.cc

Machine.o: Machine.cc Machine.h

MemoryBus.o: MemoryBus.cc MemoryBus.h

MemoryRegion.o: MemoryRegion.cc MemoryBus.h

MemoryScreen.o: MemoryScreen.cc MemoryScreen.h

clean:
	rm -f *.o emu
