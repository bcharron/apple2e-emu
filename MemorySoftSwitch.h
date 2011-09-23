/*
 * SoftSwitchRegion.h - <description>
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
 * SoftSwitchRegion.h - Benjamin Charron <bcharron@pobox.com>
 * Created  : Thu Sep 22 23:05:40 2011
 * Revision : $Id$
 */

#include "MemoryRegion.h"

class MemorySoftSwitch : public MemoryRegion
{
public:
	MemorySoftSwitch(uint16_t regionStart, uint16_t regionEnd, bool readonly);
	bool isTextMode(void) { return(text); }
	bool isRAMRD(void) { return(ramrd); }
	bool isRAMWRT(void) { return(ramwrt); }
	bool useBank2(void) { return(bBank2); }
	bool isALTZP(void) { return(altzp); }
	bool is80Store(void) { return(text80Store); }
	bool is80Col(void) { return(text80Col); }
	bool isPage2(void) { return(page2); }
	bool isBankRead(void) { return(bankRead); }
	bool isBankWrite(void) { return(bankWrite); }
	bool isSlotCXROM(void) { return(slotCXROM); }
	bool isSlotC3ROM(void) { return(slotC3ROM); }

private:
	void write(uint16_t offset, uint8_t byte);
	uint8_t read(uint16_t offset);

	bool altCharset;
	bool text;
	bool mixed;
	bool page2;
	bool hires;
	bool text80Col;
	bool text80Store;
	bool vbl;
	bool altzp;        // Pages 0 and 1 go go AUX mem
	bool ramrd;        // Reads go to AUX mem
	bool ramwrt;       // Writes go to AUX mem
	bool bankRead;   // True if reading from 0xD000 BANK rather than ROM
	bool bankWrite;  // True if writing to 0xD000 BANK, otherwise discard the write
	bool bBank2;     // 0: Read from bank 1.  1: Read from bank 2.
	bool slotCXROM;  // 0: Read from internal ROM  1: Read from expansion ROM
	bool slotC3ROM;  // 0: Read from 80-col firmware  1: Read from expansion ROM
};
