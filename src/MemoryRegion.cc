/*
 * MemoryRegion.cc - Emulates a memory region for the Apple ][e emulator
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
 * MemoryRegion.cc - Benjamin Charron <bcharron@pobox.com>
 * Created  : Tue Jan 10 19:44:05 2012
 * Revision : $Id$
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "MemoryRegion.h"

MemoryRegion::MemoryRegion(uint16_t regionStart, uint16_t regionEnd, bool readonly)
	: regionStart(regionStart),
	  regionEnd(regionEnd),
	  size((unsigned long) regionEnd - regionStart + 1),
	  readonly(readonly)
{
	this->data = new uint8_t[this->size];
	memset(this->data, 0, this->size);
}

MemoryRegion::~MemoryRegion(void)
{
	delete[] this->data;
}

void
MemoryRegion::setData(uint8_t data[])
{
	memcpy(this->data, data, this->size);
}

uint16_t MemoryRegion::getStart(void)
{
	return(this->regionStart);
}

uint16_t MemoryRegion::getEnd(void)
{
	return(this->regionEnd);
}

unsigned long MemoryRegion::getSize(void)
{
	return(this->size);
}

bool MemoryRegion::isReadOnly(void)
{
	return(readonly);
}

/* 
 * Translate from absolute offset (ie, offset in the entire memory) to
 * the offset for this memory region. ie, if the entire memory is 65k,
 * this region is mapped at 10k and this region's size is 2k, then the
 * real offset should be between [0 , 2k-1].
 */
uint16_t MemoryRegion::translateOffset(uint16_t offset)
{
	uint16_t realOffset;

	realOffset = offset - regionStart;

	return(realOffset);
}

/* Read a byte from this memory region */
uint8_t MemoryRegion::read(uint16_t offset)
{
	assert(offset >= regionStart && offset <= regionEnd);

	uint16_t regionOffset = translateOffset(offset);

	return(data[regionOffset]);
}

/* Write a byte to this memory region */
void MemoryRegion::write(uint16_t offset, uint8_t val)
{
	assert(offset >= regionStart && offset <= regionEnd);

	uint16_t regionOffset = translateOffset(offset);

	if (! readonly)
		data[regionOffset] = val;
}
