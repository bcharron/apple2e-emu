LDFLAGS=`sdl-config --libs` -lSDL
CPPFLAGS=-Wall -ggdb
CC=g++

all: emu

emu: Disk.o Machine.o MemoryRegion.o MemoryBus.o MemorySoftSwitch.o Screen.o emu.o

emu.o: emu.cc

Disk.o: Disk.cc Disk.h

Machine.o: Machine.cc Machine.h

MemoryBus.o: MemoryBus.cc MemoryBus.h

MemoryRegion.o: MemoryRegion.cc MemoryBus.h

MemorySoftSwitch.o: MemorySoftSwitch.cc MemorySoftSwitch.h

Screen.o: Screen.cc Screen.h

clean:
	rm -f *.o emu
