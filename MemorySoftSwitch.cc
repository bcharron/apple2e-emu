/*
 * MemorySoftSwitch.cc - <description>
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
 * MemorySoftSwitch.cc - Benjamin Charron <bcharron@pobox.com>
 * Created  : Thu Sep 22 23:08:35 2011
 * Revision : $Id$
 */

#include "MemorySoftSwitch.h"

MemorySoftSwitch::MemorySoftSwitch(uint16_t regionStart, uint16_t regionEnd, bool readonly)
	: MemoryRegion(regionStart, regionEnd, readonly),
	  altCharset(false),
	  text(true),
	  mixed(false),
	  page2(false),
	  hires(false),
	  text80Col(false),
	  text80Store(false),
	  vbl(false),
	  altzp(false),
	  ramrd(false),
	  ramwrt(false),
	  bankRead(false),
	  bankWrite(false),
	  bBank2(false),
	  slotCXROM(false),
	  slotC3ROM(false)
{
}

void
MemorySoftSwitch::write(uint16_t offset, uint8_t byte)
{
	switch(offset)
	{
		case 0xC000:
		{
			text80Store = false;
			break;
		}

		case 0xC001:
		{
			text80Store = true;
			break;
		}

		case 0xC050:
		{
			text = false;
			break;
		}

		case 0xC051:
		{
			text = true;
			break;
		}

		case 0xC052:
		{
			mixed = false;
			break;
		}

		case 0xC053:
		{
			mixed = true;
			break;
		}

		case 0xC054:
		{
			page2 = false;
			break;
		}

		case 0xC055:
		{
			page2 = true;
			break;
		}

		case 0xC056:
		{
			hires = false;
			break;
		}

		case 0xC057:
		{
			hires = true;
			break;
		}

		case 0xC00C:
		{
			text80Col = false;
			break;
		}

		case 0xC00D:
		{
			text80Col = true;
			break;
		}

		case 0xC00E:
		{
			altCharset = false;
			break;
		}

		case 0xC00F:
		{
			altCharset = true;
			break;
		}

		default:
		{
			uint16_t realOffset = translateOffset(offset);
			data[realOffset] = byte;

			break;
		}
	}
}

uint8_t
MemorySoftSwitch::read(uint16_t offset)
{
	uint8_t val = 0x00;

	switch(offset) {
		case 0xC01F:
		{
			val = (text80Col ? 0x80 : 0x00);
			break;
		}

		case 0xC01E:
		{
			val = (altCharset ? 0x80 : 0x00);
			break;
		}
			
		case 0xC01A:
		{
			val = (text ? 0x80 : 0x00);
			break;
		}
			
		case 0xC01B:
		{
			val = (mixed ? 0x80 : 0x00);
			break;
		}

		case 0xC019:
		{
			// XXX: This is probably a factor of cycles.
			val = vbl;
			break;
		}

		case 0xC030:
		{
			// This is the speaker
			break;
		}
			
		default:
		{
			uint16_t realOffset = translateOffset(offset);
			val = data[realOffset];
			break;
		}
	}			

	return(val);
}
