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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <string.h> // strlen
#include <string>
#include "Machine.h"

using namespace std;

// The Apple IIe CPU is a 6502B ("a high-speed version of 6502")
void usage(char *argv0)
{
	printf("Usage: %s <file.dsk>\n", argv0);
}

int main (int argc, char *argv[])
{
	Machine machine;
	string romFilename(ROM_FILENAME);

	machine.init();
	
	if (! machine.loadApple2eROM(romFilename)) {
		cerr << "Could not load the apple ROM " << ROM_FILENAME << endl;
		exit(1);
	}

/*
	unsigned int x = 0xFF69;

	do {
		unsigned int len = machine.dumpInstruction(x);
		x += len;
	} while(x < machine.memory->getSize());
*/
//C23F  B9 44 C2    LDA $C244,Y

	machine.interactive();

	//machine.testCPU();

	return (0);
}
