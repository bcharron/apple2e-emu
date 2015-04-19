LDFLAGS=`sdl2-config --libs`
CPPFLAGS=-Wall -ggdb `sdl2-config --cflags`
CC=g++

all: emu

emu: Disk.o Machine.o MemoryRegion.o MemoryBus.o MemoryDisk.o MemorySoftSwitch.o Screen.o emu.o

emu.o: emu.cc

Disk.o: Disk.cc Disk.h

Machine.o: Machine.cc Machine.h

MemoryBus.o: MemoryBus.cc MemoryBus.h

MemoryDisk.o: MemoryDisk.cc MemoryDisk.h

MemoryRegion.o: MemoryRegion.cc MemoryBus.h

MemorySoftSwitch.o: MemorySoftSwitch.cc MemorySoftSwitch.h

Screen.o: Screen.cc Screen.h

clean:
	rm -f *.o emu
