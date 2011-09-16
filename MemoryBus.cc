#include <iostream>
#include <vector>
#include <string.h>
#include "MemoryBus.h"

#define BOGUS_DEFAULT_VALUE 0x00

using namespace std;

MemoryBus::MemoryBus(unsigned int size)
{
	this->memorySize = size;

	uint8_t *mem = new uint8_t[size];

	memset(mem, BOGUS_DEFAULT_VALUE, size);

	this->ram = new MemoryRegion(0, size - 1, mem);

	delete[] mem;
}

unsigned int
MemoryBus::getSize(void)
{
	return(this->memorySize);
}

void MemoryBus::addRegion(MemoryRegion *region)
{
	regions.push_back(region);
}

MemoryRegion* MemoryBus::findRegion(uint16_t offset)
{
	MemoryRegion* region = NULL;
	std::vector<MemoryRegion*>::iterator it;

	for(it = regions.begin(); it < regions.end(); it++)
	{
		region = *it;

		if (region->getStart() <= offset && region->getEnd() >= offset) {
			return(region);
		}
	}

	return(NULL);
}

uint8_t MemoryBus::read(uint16_t offset)
{
	MemoryRegion* region = this->findRegion(offset);

	if (! region)
		region = ram;

	uint8_t val = region->read(offset);

	return(val);
}


void MemoryBus::write(uint16_t offset, uint8_t byte)
{
	MemoryRegion* region = this->findRegion(offset);

	if (! region)
		region = ram;

	region->write(offset, byte);
}
