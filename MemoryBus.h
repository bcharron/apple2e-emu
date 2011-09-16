#include <vector>
#include <stdint.h>
#include "MemoryRegion.h"

/*
 * The memory bus is a linked list of all regions in the system: RAM, ROMs, screen memory, etc.
 */
class MemoryBus
{
public:
	MemoryBus(unsigned int size);
	void addRegion(MemoryRegion *region);
	uint8_t read(uint16_t offset);
	void write(uint16_t offset, uint8_t byte);
	unsigned int getSize(void);
protected:
	MemoryRegion* findRegion(uint16_t offset);
	std::vector<MemoryRegion*> regions;
	MemoryRegion *ram;
	unsigned int memorySize;
};
