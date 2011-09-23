#include "MemoryBus.h"

#include <string.h>
#include <assert.h>

#include <iostream>
#include <vector>

using namespace std;

#define SOFT_SWITCH_START 0xC000
#define SOFT_SWITCH_END   0xC01F

uint8_t get_page(uint16_t addr)
{
	uint8_t page = (addr >> 8) & 0x00FF;

	return(page);
}

MemoryBus::MemoryBus(unsigned int size)
	: memorySize(size)
{
}

void
MemoryBus::init(void)
{
/*
	this->ram = new MemoryRegion(0, getSize() - 1);
	this->auxiliary = new MemoryRegion(0, getSize() - 1);
	this->internalROM = new MemoryRegion(0xC100, 0xCFFF);
	this->mainROM = new MemoryRegion(0xD000, 0xFFFF);
	this->ioROM = new MemoryRegion(0xC100, 0xCFFF);
*/

	regions[REGION_MAIN_RAM] = new MemoryRegion(0x0000, memorySize - 1);
	regions[REGION_MAIN_ROM] = new MemoryRegion(0xD000, 0xFFFF);
	regions[REGION_INTERNAL_ROM] = new MemoryRegion(0xC100, 0xCFFF);
	regions[REGION_AUX_RAM] = new MemoryRegion(0x0000, memorySize - 1);
	regions[REGION_AUX_BANK2] = new MemoryRegion(0xD000, 0xDFFF);
	regions[REGION_MAIN_BANK1] = new MemoryRegion(0xD000, 0xDFFF);
	regions[REGION_SOFT_SWITCHES] = new MemoryRegion(0xC000, 0xC0FF);
	regions[REGION_SLOT_ROMS] = new MemoryRegion(0xC100, 0xCFFF);

	for(int x = 0; x < 256; x++) {
		pageTable[x] = regions[REGION_MAIN_RAM];
	}

	softSwitches.ramrd = false;
	softSwitches.ramwrt = false;
	softSwitches.altzp = false;
	softSwitches.bankRead = false;
}

unsigned int
MemoryBus::getSize(void)
{
	return(this->memorySize);
}

MemoryRegion*
MemoryBus::getRegion(enum memory_regions regionNumber)
{
	assert(regionNumber > 0 && regionNumber < NB_REGIONS);

	MemoryRegion *region = regions[regionNumber];

	return(region);
}

void
MemoryBus::addRegion(MemoryRegion *region)
{
	// regions.push_back(region);
}

/*
void
MemoryBus::setRegion(MemoryRegion *region, uint16_t start, uint16_t end)
{
	for (uint16_t addr = start; addr < end; addr += 0x100) {
		int8_t page = get_page(addr);
		pageTable[page] = region;
	}
}
*/

void
MemoryBus::setRegionData(enum memory_regions regionNumber, uint16_t size, uint8_t *data)
{
	assert(regionNumber >= 0 && regionNumber < NB_REGIONS);

	MemoryRegion *region = regions[regionNumber];

	assert(region != NULL && region->getSize() == size);

	region->setData(data);
}

MemoryRegion*
MemoryBus::findRegion(uint16_t offset)
{
/*
	MemoryRegion* region = NULL;

	std::vector<MemoryRegion*>::iterator it;

	for(it = regions.begin(); it < regions.end(); it++)
	{
		region = *it;

		if (region->getStart() <= offset && region->getEnd() >= offset) {
			return(region);
		}
	}
*/
	return(NULL);
}

#define USE_BIG_SWITCH

#ifdef USE_PAGE_TABLE

uint8_t
MemoryBus::read(uint16_t offset)
{
	uint8_t page = get_page(offset);

	MemoryRegion* region = pageTable[page];

	uint8_t result = region->read(offset);

	return(result);
}

#endif

#ifdef USE_BIG_SWITCH

uint8_t
MemoryBus::read(uint16_t offset)
{
	MemoryRegion* region = NULL;
	uint8_t page = get_page(offset);
	uint8_t result;

	switch(page)
	{
		case 0x00:
		case 0x01:
		{
			if (softSwitches.altzp)
				region = regions[REGION_AUX_RAM];
			else
				region = regions[REGION_MAIN_RAM];

			break;
		}

		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		{
			if (softSwitches.ramrd || (softSwitches.text80Store && softSwitches.page2))
				region = regions[REGION_AUX_RAM];
			else
				region = regions[REGION_MAIN_RAM];
			break;
		}

		case 0xC0:
		{
			result = readSoftSwitch(offset);
			break;
		}

		// Slot ROMs
		case 0xC1:
		case 0xC2:
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xC6:
		case 0xC7:
		case 0xC8:
		case 0xC9:
		case 0xCA:
		case 0xCB:
		case 0xCC:
		case 0xCD:
		case 0xCE:
		case 0xCF:
		{
			region = regions[REGION_SLOT_ROMS];
			break;
		}

		// Bank switched memory
		case 0xD0:
		case 0xD1:
		case 0xD2:
		case 0xD3:
		case 0xD4:
		case 0xD5:
		case 0xD6:
		case 0xD7:
		case 0xD8:
		case 0xD9:
		case 0xDA:
		case 0xDB:
		case 0xDC:
		case 0xDD:
		case 0xDE:
		case 0xDF:
		{
			if (softSwitches.bankRead) {
				if (softSwitches.useBank2)
					region = regions[REGION_MAIN_BANK1];
				else
					region = regions[REGION_MAIN_BANK2];
			} else {
				region = regions[REGION_MAIN_ROM];
			}
		
			break;
		}
		
		default:
		{
			if (softSwitches.ramrd)
				region = regions[REGION_AUX_RAM];
			else
				region = regions[REGION_MAIN_RAM];
			break;
		}
	}

	if (region)
		result = region->read(offset);

	return(result);
}

void
MemoryBus::write(uint16_t offset, uint8_t byte)
{
	MemoryRegion* region = NULL;
	uint8_t page = get_page(offset);

	switch(page)
	{
		case 0x00:
		case 0x01:
		{
			if (softSwitches.altzp)
				region = regions[REGION_AUX_RAM];
			else
				region = regions[REGION_MAIN_RAM];

			break;
		}

		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		{
			if (softSwitches.ramrd || (softSwitches.text80Store && softSwitches.page2))
				region = regions[REGION_AUX_RAM];
			else
				region = regions[REGION_MAIN_RAM];
			break;
		}

		case 0xC0:
		{
			writeSoftSwitch(offset, byte);
			break;
		}

		// Slot ROMs
		case 0xC1:
		case 0xC2:
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xC6:
		case 0xC7:
		case 0xC8:
		case 0xC9:
		case 0xCA:
		case 0xCB:
		case 0xCC:
		case 0xCD:
		case 0xCE:
		case 0xCF:
		{
			region = regions[REGION_SLOT_ROMS];
			break;
		}

		// Bank switched memory
		case 0xD0:
		case 0xD1:
		case 0xD2:
		case 0xD3:
		case 0xD4:
		case 0xD5:
		case 0xD6:
		case 0xD7:
		case 0xD8:
		case 0xD9:
		case 0xDA:
		case 0xDB:
		case 0xDC:
		case 0xDD:
		case 0xDE:
		case 0xDF:
		{
			if (softSwitches.bankRead) {
				if (softSwitches.useBank2)
					region = regions[REGION_MAIN_BANK1];
				else
					region = regions[REGION_MAIN_BANK2];
			} else {
				region = regions[REGION_MAIN_ROM];
			}
		
			break;
		}
		
		default:
		{
			if (softSwitches.ramrd)
				region = regions[REGION_AUX_RAM];
			else
				region = regions[REGION_MAIN_RAM];
			break;
		}
	}

	if (region)
		region->write(offset, byte);
}

#endif

#ifdef USE_REGIONS

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
/*
	MemoryRegion* region = this->findRegion(offset);

	if (! region) {
		if (offset >= SOFT_SWITCH_START && offset <= SOFT_SWITCH_END)
			writeSoftSwitch(offset, byte);
		else
			region = ram;
	}

	if (region)
		region->write(offset, byte);
*/
}
#endif

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
