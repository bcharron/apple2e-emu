#include <vector>
#include "MemoryBus.h"

MemoryBus::MemoryBus(uint16_t size)
{
	uint8_t mem = new uint8_t[size];
	ram = new MemoryRegion(0, size - 1, mem);
	delete[] mem;
}

MemoryRegion* MemoryBus::findRegion(uint16_t offset)
{
	MemoryRegion* region = NULL;
	std::vector<MemoryRegion*>::iterator it;

	for(it = regions.begin(); it < regions.end(); it++)
	{
		if (region->getStart() <= offset && region->getEnd() >= offset) {
			region = *it;
			break;
		}
	}

	return(region);
}

uint8_t MemoryBus::read(uint16_t offset)
{
	MemoryRegion *region;

	region = this->findRegion(offset);

	uint8_t val;

	if (! region)
		region = ram;

	val = region->read(offset);

	return(val);
}


void MemoryBus::write(uint16_t offset, uint8_t byte)
{
	MemoryRegion *region;

	region = this->findRegion(offset);

	if (! region)
		region = ram;

	region->write(offset, byte);
}
