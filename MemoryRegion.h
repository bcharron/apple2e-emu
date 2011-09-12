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

class MemoryRegion
{
public:
	MemoryRegion(uint16_t regionStart, uint16_t regionEnd, uint8_t *data);
	~MemoryRegion(void);
	uint16_t getStart(void);
	uint16_t getEnd(void);
	uint8_t read(uint16_t offset);
	void write(uint16_t offset, uint8_t val);
protected:
	uint16_t translateOffset(uint16_t offset);

	uint16_t regionStart;
	uint16_t regionEnd;
	uint16_t size;
	uint8_t *data;
};

#endif
