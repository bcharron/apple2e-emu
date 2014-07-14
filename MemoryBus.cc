/*
 * MemoryBus.cc - Memory bus for the Apple ][e emulator
 * Copyright (C) 2011 Benjamin Charron <bcharron@pobox.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * emu.c - Benjamin Charron <bcharron@pobox.com>
 * Created  : Fri Sep  9 16:47:51 2011
 * Revision : $Id$
 */

#include "MemoryBus.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <vector>

#include "Registers.h"
#include "MemorySoftSwitch.h"
#include "MemoryDisk.h"

using namespace std;

#define SOFT_SWITCH_START 0xC000
#define SOFT_SWITCH_END   0xC01F

uint8_t get_page(uint16_t addr)
{
	uint8_t page = (addr >> 8) & 0x00FF;

	return(page);
}

MemoryBus::MemoryBus(unsigned int size, registers_t *registers)
	: memorySize(size),
	  registers(registers)
{
}

void
MemoryBus::init(void)
{
	regions[REGION_AUX_BANK2] = new MemoryRegion(0xD000, 0xDFFF, REGION_RW);
	regions[REGION_AUX_RAM] = new MemoryRegion(0x0000, memorySize - 1, REGION_RW);
	regions[REGION_INTERNAL_ROM] = new MemoryRegion(0xC100, 0xCFFF, REGION_RO);
	regions[REGION_MAIN_BANK2] = new MemoryRegion(0xD000, 0xDFFF, REGION_RW);
	regions[REGION_MAIN_RAM] = new MemoryRegion(0x0000, memorySize - 1, REGION_RW);
	regions[REGION_MAIN_ROM] = new MemoryRegion(0xD000, 0xFFFF, REGION_RO);
	regions[REGION_SOFT_SWITCHES] = new MemorySoftSwitch(0xC000, 0xC07F, REGION_RW);
	regions[REGION_SLOT_IO] = new MemoryDisk(0xC090, 0xC09F, REGION_RW);
	regions[REGION_SLOT_ROMS] = new MemoryRegion(0xC100, 0xCFFF, REGION_RO);
}

unsigned int
MemoryBus::getSize(void)
{
	return(this->memorySize);
}

MemoryRegion*
MemoryBus::getRegion(enum memory_regions regionNumber)
{
	assert(regionNumber >= 0 && regionNumber < NB_REGIONS);

	MemoryRegion *region = regions[regionNumber];

	return(region);
}

void
MemoryBus::setRegionData(enum memory_regions regionNumber, uint16_t size, uint8_t *data)
{
	assert(regionNumber >= 0 && regionNumber < NB_REGIONS);

	MemoryRegion *region = regions[regionNumber];

	assert(region != NULL && region->getSize() >= size);

	region->setData(data);
}

#define ACCESS_WRITE true
#define ACCESS_READ false

// XXX: Add memory breakpoints
uint8_t
MemoryBus::read(uint16_t offset)
{
	uint8_t result = this->access(offset, ACCESS_READ, 0x00);

	return(result);
}

void
MemoryBus::write(uint16_t offset, uint8_t byte)
{
	this->access(offset, ACCESS_WRITE, byte);
}

/*
 * write == false : perform a read (return a value, ignore 'byte')
 * write == true : perform a write (return 0, write byte at offset)
 */
uint8_t
MemoryBus::access(uint16_t offset, bool write, uint8_t byte)
{
	MemoryRegion* region = NULL;
	uint8_t page = get_page(offset);
	uint8_t result;
	MemorySoftSwitch* switches = (MemorySoftSwitch*) regions[REGION_SOFT_SWITCHES];

	switch(page)
	{
		case 0x00:
		case 0x01:
		{
			if (switches->isALTZP())
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
			if ((write && switches->isRAMWRT()) || (!write && switches->isRAMRD()) || (switches->is80Store() && switches->isPage2()))
				region = regions[REGION_AUX_RAM];
			else
				region = regions[REGION_MAIN_RAM];
			break;
		}

		case 0xC0:
		{
			if (offset >= 0xC090 && offset <= 0xC09F)
				region = regions[REGION_SLOT_IO]; // Disk controller
			else
				region = regions[REGION_SOFT_SWITCHES];
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
			if (switches->isSlotCXROM())
				region = regions[REGION_SLOT_ROMS];
			else
				region = regions[REGION_INTERNAL_ROM];
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
			// 4 possibilities: ROM, Main bank 1, Main bank 2, Aux bank 1, Aux bank 2
			if ( (write && switches->isRAMWRT()) || (!write && switches->isRAMRD()) )
				region = switches->useBank2() ? regions[REGION_AUX_BANK2] : regions[REGION_AUX_RAM];
			else if ( (write && switches->isBankWrite()) || (!write && switches->isBankRead()) )
				region = switches->useBank2() ? regions[REGION_MAIN_BANK2] : regions[REGION_MAIN_RAM];
			else
				region = regions[REGION_MAIN_ROM];

			break;
		}

		case 0xE0:
		case 0xE1:
		case 0xE2:
		case 0xE3:
		case 0xE4:
		case 0xE5:
		case 0xE6:
		case 0xE7:
		case 0xE8:
		case 0xE9:
		case 0xEA:
		case 0xEB:
		case 0xEC:
		case 0xED:
		case 0xEE:
		case 0xEF:
		case 0xF0:
		case 0xF1:
		case 0xF2:
		case 0xF3:
		case 0xF4:
		case 0xF5:
		case 0xF6:
		case 0xF7:
		case 0xF8:
		case 0xF9:
		case 0xFA:
		case 0xFB:
		case 0xFC:
		case 0xFD:
		case 0xFE:
		case 0xFF:
		{
			// XXX: This works for read, but what about write?
			if ( switches->isBankRead() )
				region = regions[REGION_MAIN_RAM];
			else
				region = regions[REGION_MAIN_ROM];
				     
			break;
		}
		
		default:
		{
			if (switches->isRAMRD())
				region = regions[REGION_AUX_RAM];
			else
				region = regions[REGION_MAIN_RAM];
			break;
		}
	}

	if (region) {
		if (write) {
			result = 0;

			// XXX: It would be very useful here to have access to the registers.
			if (region->isReadOnly())
				printf("Warning: Code at $%04X is trying to write to readonly region $%04X\n", registers->pc, offset);

			region->write(offset, byte);
		} else
			result = region->read(offset);
	}

	return(result);	
}

