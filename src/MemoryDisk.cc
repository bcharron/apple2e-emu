/*
 * MemoryDisk.cc - Memory part of the disk controller for the Apple ][e emulator
 * Copyright (C) 2012 Benjamin Charron <bcharron@pobox.com>
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
 * MemoryDisk.cc - Benjamin Charron <bcharron@pobox.com>
 * Created  : Tue Jan 10 19:44:05 2012
 * Revision : $Id$
 */

#include "MemoryDisk.h"

#include <cstdio>

#include <assert.h>

MemoryDisk::MemoryDisk(uint16_t regionStart, uint16_t regionEnd, bool readonly)
	: MemoryRegion(regionStart, regionEnd, readonly),
	  currentDisk(NULL)
	 
{
	disk[0] = NULL;
	disk[1] = NULL;
}

/*

Slot 1's I/O Space is C090-C09F
Slot 2.. C0A0-C0AF
Slot 3.. C0B0-C0BF
..
Slot 7.. C0F0-C0FF

*/
void 
MemoryDisk::write(uint16_t offset, uint8_t byte)
{
	uint16_t realOffset = translateOffset(offset);

	// Don't try anything if there's no disk loaded
	if (! currentDisk)
		return;

	printf("MemoryDisk::read($%04X)\n", offset);

	switch(realOffset) {
		case 0x0000:
			currentDisk->phaseOff(0);
			break;

		case 0x0001:
			currentDisk->phaseOn(0);
			break;

		case 0x0002:
			currentDisk->phaseOff(1);
			break;

		case 0x0003:
			currentDisk->phaseOn(1);
			break;

		case 0x0004:
			currentDisk->phaseOff(2);
			break;

		case 0x0005:
			currentDisk->phaseOn(2);
			break;

		case 0x0006:
			currentDisk->phaseOff(3);
			break;

		case 0x0007:
			currentDisk->phaseOn(3);
			break;

		case 0x0008:
			currentDisk->motorOn();
			break;

		case 0x0009:
			currentDisk->motorOff();
			break;

		case 0x000A:
			currentDisk = disk[0];
			break;

		case 0x000B:
			currentDisk = disk[1];
			break;

		case 0x000C: 
			q6 = 0;
			break;

		case 0x000D:
			q6 = 1;
			break;

		case 0x000E:
			q7 = 0;
			break;

		case 0x000F:
			q7 = 1;
			break;

		default:
			// Ignore?
			break;
	}	
}

uint8_t 
MemoryDisk::read(uint16_t offset)
{
	uint16_t realOffset = translateOffset(offset);
	uint8_t val = 0x00;

	// XXX: There's a specific value to return here but I don't
	// remember what. Possibly just a random value.
	if (! currentDisk)
		return(0);

	// printf("MemoryDisk::read($%04X)\n", offset);

	switch(realOffset) {
		case 0x0000:
			currentDisk->phaseOff(0);
			break;

		case 0x0001:
			currentDisk->phaseOn(0);
			break;

		case 0x0002:
			currentDisk->phaseOff(1);
			break;

		case 0x0003:
			currentDisk->phaseOn(1);
			break;

		case 0x0004:
			currentDisk->phaseOff(2);
			break;

		case 0x0005:
			currentDisk->phaseOn(2);
			break;

		case 0x0006:
			currentDisk->phaseOff(3);
			break;

		case 0x0007:
			currentDisk->phaseOn(3);
			break;

		case 0x0008:
			currentDisk->motorOn();
			break;

		case 0x0009:
			currentDisk->motorOff();
			break;

		case 0x000A:
			currentDisk = disk[0];
			break;

		case 0x000B:
			currentDisk = disk[1];
			break;

		case 0x000C: 
			q6 = 0;
			if (! q7)
				val = currentDisk->readNextByte();
			break;

		case 0x000D:
			q6 = 1;
			break;

		case 0x000E:
			q7 = 0;
			break;

		case 0x000F:
			q7 = 1;
			break;

		default:
			// Ignore?
			break;
	}	

	return(val);
}

void
MemoryDisk::setDisk(int driveNumber, Disk *disk)
{
	assert(driveNumber >= 0 && driveNumber <= 1);

	this->disk[driveNumber] = disk;

	if (currentDisk == NULL)
		currentDisk = this->disk[driveNumber];
}

