/*
 * MemoryDisk.h - Interface between memory and disk controller (handles $Cx00-$CxFF)
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
 * MemoryDisk.h - Benjamin Charron <bcharron@pobox.com>
 * Created  : Tue Jan 10 19:43:30 2012
 * Revision : $Id$
 */

#ifndef _MEMORYDISK_H
#define _MEMORYDISK_H

#include "Disk.h"
#include "MemoryRegion.h"

class MemoryDisk : public MemoryRegion
{
public:
	MemoryDisk(uint16_t regionStart, uint16_t regionEnd, bool readonly);
	~MemoryDisk(void);
	void write(uint16_t offset, uint8_t byte);
	uint8_t read(uint16_t offset);
	void setDisk(int driveNumber, Disk *disk);

private:
	Disk *disk[2];
	Disk *currentDisk;
	uint8_t q6;
	uint8_t q7;
};

#endif
