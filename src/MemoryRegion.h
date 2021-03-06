/*
 * MemoryRegion.h - <description>
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
 * MemoryRegion.h - Benjamin Charron <bcharron@pobox.com>
 * Created  : Mon Sep 12 15:01:21 2011
 * Revision : $Id$
 */

#ifndef _MEMORYREGION_H
#define _MEMORYREGION_H
#include <stdint.h>

#define REGION_RO true
#define REGION_RW false

class MemoryRegion
{
public:
	MemoryRegion(uint16_t regionStart, uint16_t regionEnd, bool readonly);
	~MemoryRegion(void);
	void setData(uint8_t *data);
	uint16_t getStart(void);
	uint16_t getEnd(void);
	virtual uint8_t read(uint16_t offset);
	virtual void write(uint16_t offset, uint8_t val);
	unsigned long getSize(void);
	bool isReadOnly(void);

protected:
	uint16_t translateOffset(uint16_t offset);

	uint16_t regionStart;
	uint16_t regionEnd;
	unsigned long size;
	uint8_t *data;
	bool readonly;
};

#endif
