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

#include <stdio.h>

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
	  slotC3ROM(false),
	  keyboardData(0x00),
	  keyboardStrobe(false)
{
}

void
MemorySoftSwitch::doKeyboardStrobe(void)
{
	keyboardStrobe = true;
}

void
MemorySoftSwitch::setKeyboardData(uint8_t val)
{
	keyboardData = val;
}

void
MemorySoftSwitch::write(uint16_t offset, uint8_t byte)
{
//	printf("Switch: Writing to 0x%X\n", offset);

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

		case 0xC006:
		{
			slotCXROM = true;
			break;
		}

		case 0xC007:
		{
			slotCXROM = false;
			break;
		}

		case 0xC00A:
		{
			slotC3ROM = false;
			break;
		}

		case 0xC00B:
		{
			slotC3ROM = true;
			break;
		}

		case 0xC00C:
		{
			changeText80Col(false);
			break;
		}

		case 0xC00D:
		{
			changeText80Col(true);
			break;
		}

		case 0xC00E:
		{
			changeAltCharset(false);
			break;
		}

		case 0xC00F:
		{
			changeAltCharset(true);
			break;
		}

		case 0xC010:
		{
			// printf("0xC010: Strobe cleared.\n");
			keyboardStrobe = false;
			break;
		}

		case 0xC050:
		{
			changeText(false);
			break;
		}

		case 0xC051:
		{
			changeText(true);
			break;
		}

		case 0xC052:
		{
			changeMixed(false);
			break;
		}

		case 0xC053:
		{
			// XXX: Does setting mixed mode ON change text mode?
			changeMixed(true);
			break;
		}

		case 0xC054:
		{
			changePage2(false);
			break;
		}

		case 0xC055:
		{
			changePage2(true);
			break;
		}

		case 0xC056:
		{
			changeHires(false);
			break;
		}

		case 0xC057:
		{
			changeHires(true);
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

/*
	if (offset != 0xC000)
		printf("Switch: Reading from 0x%X\n", offset);
*/

	switch(offset) {
		case 0xC000:
		{
			// printf("Reading from 0xC000\n");
			// XXX: Read from a file when "include <file>" is used
			val = keyboardData;

			if (keyboardStrobe)
				val |= 0x80;

			break;
		}

		case 0xC010:
		{
			// printf("0xC010: Strobe cleared.\n");
			val = (keyboardStrobe ? 0x80 : 0x00);
			keyboardStrobe = false;
			break;
		}

		case 0xC015:
		{
			val = (slotCXROM ? 0x80 : 0x00);
			break;
		}

		case 0xC017:
		{
			val = (slotC3ROM ? 0x80 : 0x00);
			break;
		}

		case 0xC019:
		{
			// XXX: This is probably a factor of cycles.
			val = vbl;
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

		case 0xC01C:
		{
			val = (page2 ? 0x80 : 0x00);
			break;
		}

		case 0xC01D:
		{
			val = (hires ? 0x80 : 0x00);
			break;
		}

		case 0xC01E:
		{
			val = (altCharset ? 0x80 : 0x00);
			break;
		}			

		case 0xC01F:
		{
			val = (text80Col ? 0x80 : 0x00);
			break;
		}

		case 0xC030:
		{
			// This is the speaker
			break;
		}

		case 0xC050:
		{
			changeText(false);
			val = text;
			break;
		}

		case 0xC051:
		{
			changeText(true);
			val = text;
			break;
		}

		case 0xC052:
		{
			changeMixed(false);
			val = mixed;
			break;
		}

		case 0xC053:
		{
			changeMixed(true);
			val = mixed;
			break;
		}

		case 0xC054:
		{
			changePage2(false);
			val = page2;
			break;
		}

		case 0xC055:
		{
			changePage2(true);
			val = page2;
			break;
		}

		case 0xC056:
		{
			changeHires(false);
			val = hires;
			break;
		}

		case 0xC057:
		{
			changeHires(true);
			val = hires;
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
