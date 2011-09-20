#include <iostream>
#include <vector>
#include <string.h>
#include "MemoryBus.h"

using namespace std;

#define SOFT_SWITCH_START 0xC000
#define SOFT_SWITCH_END   0xC01F

MemoryBus::MemoryBus(unsigned int size)
{
	this->memorySize = size;
	this->ram = new MemoryRegion(0, size - 1);
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
	uint8_t val;

	if (! region) {
		if (offset >= SOFT_SWITCH_START && offset <= SOFT_SWITCH_END)
			val = readSoftSwitch(offset);
		else
			region = ram;
	}

	if (region)
		val = region->read(offset);

	return(val);
}

void MemoryBus::write(uint16_t offset, uint8_t byte)
{
	MemoryRegion* region = this->findRegion(offset);

	if (! region) {
		if (offset >= SOFT_SWITCH_START && offset <= SOFT_SWITCH_END)
			writeSoftSwitch(offset, byte);
		else
			region = ram;
	}

	if (region)
		region->write(offset, byte);
}

void MemoryBus::writeSoftSwitch(uint16_t offset, uint8_t byte)
{
	switch(offset)
	{
	case 0xC000:
	{
		softSwitches.text80Store = false;
		break;
	}

	case 0xC001:
	{
		softSwitches.text80Store = true;
		break;
	}

	case 0xC050:
	{
		softSwitches.text = false;
		break;
	}

	case 0xC051:
	{
		softSwitches.text = true;
		break;
	}

	case 0xC052:
	{
		softSwitches.mixed = false;
		break;
	}

	case 0xC053:
	{
		softSwitches.mixed = true;
		break;
	}

	case 0xC054:
	{
		softSwitches.page2 = false;
		break;
	}

	case 0xC055:
	{
		softSwitches.page2 = true;
		break;
	}

	case 0xC056:
	{
		softSwitches.hires = false;
		break;
	}

	case 0xC057:
	{
		softSwitches.hires = true;
		break;
	}

	case 0xC00C:
	{
		softSwitches.text80Col = false;
		break;
	}

	case 0xC00D:
	{
		softSwitches.text80Col = true;
		break;
	}

	case 0xC00E:
	{
		softSwitches.altCharset = false;
		break;
	}

	case 0xC00F:
	{
		softSwitches.altCharset = true;
		break;
	}

	default:
	{
		break;
	}
	}
}

uint8_t MemoryBus::readSoftSwitch(uint16_t offset)
{
	uint8_t val = 0x00;

	switch(offset) {
	case 0xC01E:
	{
		val = (softSwitches.altCharset ? 0x80 : 0x00);
		break;
	}
			
	case 0xC01A:
	{
		val = (softSwitches.text ? 0x80 : 0x00);
		break;
	}
			
	case 0xC01B:
	{
		val = (softSwitches.mixed ? 0x80 : 0x00);
		break;
	}

	case 0xC019:
	{
		// XXX: This is probably a factor of cycles.
		val = softSwitches.vbl;
		break;
	}

	case 0xC030:
	{
		// This is the speaker
		break;
	}
			
	default:
	{
		val = 0x00;
		break;
	}
	}			

	return(val);
}
