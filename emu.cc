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
#include <string>
#include "Machine.h"

using namespace std;

struct cpu_state_s {
	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t sp;   // Stack pointer. Works in 0x0100 to 0x01FF (page 1)
	uint8_t psw;
	uint16_t pc;
};

typedef struct cpu_state_s cpu_state_t;

const char *instr_c01[] =
{
	"ORA", // 0x00
	"AND", // 0x01
	"EOR", // 0x02
	"ADC", // 0x03
	"STA", // 0x04
	"LDA", // 0x05
	"CMP", // 0x06
	"SBC", // 0x07
};

const char *addr_mode_c01[] =
{
	"($%02X,X)",  // 0x00
	"$%02X",      // 0x01
	"#$%02X",     // 0x02
	"#$%02X%02X", // 0x03
	"($%02X,Y)",  // 0x04
	"$%02X,X",    // 0x05
	"$%02X%02X,Y", // 0x06
	"$%02X%02X,X", // 0x06
};

void usage(char *argv0)
{
	printf("Usage: %s <file.dsk>\n", argv0);
}

int main (int argc, char *argv[])
{
	Machine machine(64 * 1024);
	string romFilename(ROM_FILENAME);
	
	if (! machine.loadApple2eROM(romFilename)) {
		cerr << "Could not load the apple ROM " << ROM_FILENAME << endl;
		exit(1);
	}

	unsigned int size = machine.memory->getSize();
	unsigned int x = 0;

	do {
		unsigned int len = machine.dumpInstruction(x);
		x += len;
	} while(x < machine.memory->getSize());

	return (0);
}
