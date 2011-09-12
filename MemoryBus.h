#include <vector>
#include <stdint.h>
#include "MemoryRegion.h"

class MemoryBus
{
public:
	MemoryBus(uint16_t size);
	void addRegion(MemoryRegion *region);
	uint8_t read(uint16_t offset);
	void write(uint16_t offset, uint8_t byte);
protected:
	MemoryRegion* findRegion(uint16_t offset);
	std::vector<MemoryRegion*> regions;
	MemoryRegion *ram;
};
