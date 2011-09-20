/*
 * ScreenMemory.h - <description>
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
 * ScreenMemory.h - Benjamin Charron <bcharron@pobox.com>
 * Created  : Mon Sep 12 18:50:26 2011
 * Revision : $Id$
 */

#include "MemoryRegion.h"

class MemoryScreen : public MemoryRegion
{
public:
	MemoryScreen(void);
	void write(uint16_t offset, uint8_t val);
private:
	uint8_t *auxData;
};
