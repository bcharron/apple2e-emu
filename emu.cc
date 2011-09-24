/*
 * emu.c - An Apple ][e Emulator
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

#include "Machine.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h> // strlen
#include <SDL/SDL.h>

#include <iostream>
#include <string>

using namespace std;

// The original Apple IIe (AKA Apple ][) is a 6502B ("a high-speed version of 6502")
// The enhanced Apple IIe (AKA Apple //e) is a 65C02
void usage(char *argv0)
{
	printf("Usage: %s <file.dsk>\n", argv0);
}

int main (int argc, char *argv[])
{
	Machine machine;
	string romFilename(ROM_FILENAME);

	machine.init();

	printf("0x61: %c\n", 0x61);
	
	if (! machine.loadApple2eROM(romFilename)) {
		cerr << "Could not load the apple ROM " << ROM_FILENAME << endl;
		exit(1);
	}

	// machine.testCPU();

	machine.setPC(BOOTSTRAP_ADDRESS);
	machine.interactive();

	return (0);
}
