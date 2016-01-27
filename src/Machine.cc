/*
 * Machine.cc - The main machine emulation for the Apple ][e emulator
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

#include <assert.h>
#include <string.h>
#include <time.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "instr_table.h"
#include "MemoryDisk.h"

using namespace std;

#define SBC_TEST_TABLE_LEN sizeof(SBC_TEST_TABLE)
uint8_t SBC_TEST_TABLE[] = {
	0xD8, 0xA9, 0x01, 0x8D, 0x80, 0x03, 0xA9, 0x80,
	0x8D, 0x81, 0x03, 0x8D, 0x82, 0x03, 0xA9, 0x00,
	0x8D, 0x83, 0x03, 0x8D, 0x84, 0x03, 0xA0, 0x01,
	0x20, 0x40, 0x03, 0xE0, 0x01, 0xF0, 0x20, 0x20,
	0x90, 0x03, 0xE0, 0x01, 0xF0, 0x19, 0xEE, 0x81,
	0x03, 0xEE, 0x83, 0x03, 0xD0, 0xEA, 0xEE, 0x82,
	0x03, 0xEE, 0x84, 0x03, 0xD0, 0xE2, 0x88, 0x10,
	0xDF, 0xA9, 0x00, 0x8D, 0x80, 0x03, 0x60, 0x60,
	0xC0, 0x01, 0xAD, 0x81, 0x03, 0x6D, 0x82, 0x03,
	0xA2, 0x00, 0x50, 0x01, 0xE8, 0xC0, 0x01, 0xAD,
	0x83, 0x03, 0x6D, 0x84, 0x03, 0xB0, 0x04, 0x30,
	0x01, 0xE8, 0x60, 0x10, 0x01, 0xE8, 0x60, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x02, 0x03, 0x00, 0x04, 0x05, 0x06, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x07, 0x08, 0x00, 0x00,
	0xA2, 0x00, 0x50, 0x01, 0xE8, 0xC0, 0x01, 0xAD,
	0x83, 0x03, 0x6D, 0x84, 0x03, 0xB0, 0x04, 0x30,
	0x01, 0xE8, 0x60, 0x10, 0x01, 0xE8, 0x60,
	0xC0, 0x01, 0xAD, 0x81, 0x03, 0xED, 0x82, 0x03,
	0xA2, 0x00, 0x50, 0x01, 0xE8, 0xC0, 0x01, 0xAD,
	0x83, 0x03, 0xED, 0x84, 0x03, 0x48, 0xA9, 0xFF,
	0xE9, 0x00, 0xC9, 0xFE, 0xD0, 0x05, 0x68, 0x30,
	0x01, 0xE8, 0x60, 0x68, 0x90, 0xFB, 0x10, 0x01,
	0xE8, 0x60,
};

/*
 *   Utility functions
 */

/* Make a 16-bit value out of two 8-bit ones */
uint16_t make16(uint8_t high, uint8_t low)
{
	uint16_t offset = ((uint16_t) high << 8) | low;

	return(offset);
}

/* Get low byte of a 16-bit word */
uint8_t get_low(uint16_t word)
{
	uint8_t low = (word & 0x00FF);

	return(low);
}

/* Get high byte of a 16-bit word */
uint8_t get_high(uint16_t word)
{
	uint8_t high = (word & 0xFF00) >> 8;

	return(high);
}

/* Convert from Binary Coded Decimal to binary */
uint8_t from_bcd(uint8_t val)
{
	uint8_t result;

	uint8_t low  = val & 0x0F;
	uint8_t high = (val >> 4) & 0x0F;

	result = high * 10 + low;

	return(result);
}

/* Convert from binary to Binary Coded Decimal */
uint8_t to_bcd(uint8_t val)
{
	uint8_t result;

	uint8_t low = val % 10;
	uint8_t high = val / 10;

	result = (high << 4) | low;

	return(result);
}

Machine::Machine()
	: cycles(0),
	  pcBreakpointEnabled(false),
	  pcBreakpointOffset(0x0000),
	  traceInstructions(false)
{

}

bool
Machine::init()
{
	memory = new MemoryBus(64 * 1024, &registers);
	memory->init();

	registers.a = 0x00;
	registers.x = 0x00;
	registers.y = 0x00;
	registers.sp = 0xff;
	registers.pc = BOOTSTRAP_ADDRESS; // Monitor start
	registers.psw.val = 0;

	MemoryRegion *mainRAM = memory->getRegion(REGION_MAIN_RAM);
	MemoryRegion *auxRAM = memory->getRegion(REGION_AUX_RAM);
	
	diskController = (MemoryDisk *) memory->getRegion(REGION_SLOT_IO);

	disk[0] = new Disk();
	disk[0]->init();

	disk[1] = new Disk();
	disk[1]->init();

	diskController->setDisk(0, disk[0]);
	diskController->setDisk(1, disk[1]);

	MemorySoftSwitch *switches = (MemorySoftSwitch *) memory->getRegion(REGION_SOFT_SWITCHES);

	screen = new Screen(640, 480, mainRAM, auxRAM, switches);

	if (! screen->init()) {
		fprintf(stderr, "WARNING: Running in text mode\n");
	}

	screen->setZoom(2);

	return(true);
}

/*
 * Table 8-1. APPLE2E.ROM file map

 From   To     Description
 0x0000 0x01ff Empty
 0x0200 0x02ff Unknown, probably a slot 2 peripheral card rom
 0x0300 0x05ff Empty
 0x0600 0x06ff 16 Sector Disk II controller card ROM
 0x0700 0x0fff Empty
 0x1000 0x3fff Integer basic ROM ?
 0x4000 0x40ff Empty
 0x4100 0x4fff Internal $C100-$CFFF ROM
 0x5000 0x7fff Main $D000-$FFFF ROM
 */
bool
Machine::loadApple2eROM(string &filename)
{
	ifstream file(filename.c_str(), ios::in | ios::binary);
	uint8_t *data = new uint8_t[APPLE2E_ROM_SIZE];
        bool success = false;

	if (file.is_open()) {
		file.read((char *) data, APPLE2E_ROM_SIZE);
		file.close();
		memory->setRegionData(REGION_SLOT_ROMS, (0xC1FF - 0xC100) + 1, &data[0x0600]); // Disk controller
		memory->setRegionData(REGION_INTERNAL_ROM, (0xCFFF - 0xC100 + 1), &data[0x4100]);
		memory->setRegionData(REGION_MAIN_ROM, (0xFFFF - 0xD000 + 1), &data[0x5000]);

		success = true;
	} else {
		cerr << "Unable to open " << filename << endl;
	}

	delete[] data;

	return(success);
}

void
Machine::setPC(uint16_t pc)
{
	this->registers.pc = pc;
}

uint16_t
Machine::getPC(void)
{
	return(this->registers.pc);
}

void
Machine::dumpFlags(spc_flags_t *flags, char *buf)
{
	char strFlags[] = "czidbpvn";
	int x;

	for (x = 0; x < 8; x++) {
		uint8_t v = 0x01 << x;
		
		if (flags->val & v) {
			buf[x] = strFlags[x];
		} else {
			buf[x] = ' ';
		}
	}

	buf[x] = '\0';
}

void
Machine::dumpRegisters(void)
{
	char strFlags[10];

	dumpFlags(&registers.psw, strFlags);

	cout << "[ Registers ]" << endl;
	printf("A  : 0x%02X (S%d  U%u)\n", registers.a, (int8_t) registers.a, registers.a);
	printf("X  : 0x%02X (S%d  U%u)\n", registers.x, (int8_t) registers.x, registers.x);
	printf("Y  : 0x%02X (S%d  U%u)\n", registers.y, (int8_t) registers.y, registers.y);
	printf("SP : 0x%02X (S%d  U%u)\n", registers.sp, (int8_t) registers.sp, registers.sp);
	printf("PC : 0x%02X (S%d  U%u)\n", registers.pc, (int8_t) registers.pc, registers.pc);
	printf("PSW: 0x%02X  [%s]\n", registers.psw.val, strFlags);

/*
	cout << "A  : 0x" << hex << setw( 2 ) << setfill( '0' ) << registers.a << endl;
	cout << "X  : 0x" << hex << setw( 2 ) << setfill( '0' ) << registers.x << endl;
	cout << "Y  : 0x" << hex << setw( 2 ) << setfill( '0' ) << registers.y << endl;
	cout << "SP : 0x" << hex << setw( 2 ) << setfill( '0' ) << registers.sp << endl;
	cout << "PC : 0x" << hex << setw( 4 ) << setfill( '0' ) << registers.pc << endl;
	cout << "PSW: 0x" << hex << setw( 2 ) << setfill( '0' ) << registers.psw.val << endl;
*/
}

bool isRelativeBranchInstruction(uint8_t opcode)
{
	bool result = false;

	if ((opcode & 0x1F) == 0x10)
		result = true;

	return(result);
}

bool isBranchInstruction(uint8_t opcode)
{
	bool result = false;

	if ((opcode & 0x1F) == 0x10 || opcode == 0x4C || opcode == 0x6C)
		result = true;

	return(result);
}

struct monitor_subroutines_offsets_s
{
	const char *name;
	uint16_t offset;
};

struct monitor_subroutines_offsets_s monitor_subroutines_offsets[] =
{
	{ "BELL",   0xFF3A },
	{ "BELL1",  0xFBDD },
	{ "CLREOL", 0xFC9C },
	{ "CLEOLZ", 0xFC9E },
	{ "CLREOP", 0xFC42 },
	{ "CLRSCR", 0xF832 },
	{ "CLRTOP", 0xF836 },
	{ "COUT",   0xFDED },
	{ "COUT1",  0xFDF0 },
	{ "CROUT",  0xFD8E },
	{ "CROUT1", 0xFD8B },
	{ "GETLNZ", 0xFD67 },
	{ "GETLN",  0xFD6A },
	{ "GETLN1", 0xFD6F },
	{ "HLINE",  0xF819 },
	{ "HOME",   0xFC58 },
	{ "IOREST", 0xFF3F },
	{ "IOSAVE", 0xFF4A },
	{ "KEYIN",  0xFD1B },
	{ "MOVE",   0xFE2C },
	{ "NEXTCOL",0xF85F },
	{ "PLOT",   0xF800 },
	{ "PRBLNK", 0xF948 },
	{ "PRBL2",  0xF94A },
	{ "PRBYTE", 0xFDDA },
	{ "PREAD",  0xFB1E },
	{ "PRERR",  0xFF2D },
	{ "PRHEX",  0xFDE3 },
	{ "PRNTAX", 0xF941 },
	{ "RDCHAR", 0xFD35 },
	{ "RDKEY",  0xFD0C },
	{ "READ",   0xFEFD },
	{ "SCRN",   0xF871 },
	{ "SETCOL", 0xF864 },
	{ "SETINV", 0xFE80 },
	{ "SETNORM",0xFE84 },
	{ "VERIFY", 0xFE36 },
	{ "VLINE",  0xF828 },
	{ "WAIT",   0xFCA8 },
	{ "WRITE",  0xFECD },
};

#define MONITOR_SUBROUTINES_OFFSET_LEN (sizeof(monitor_subroutines_offsets) / sizeof(struct monitor_subroutines_offsets_s))

std::string*
Machine::getSubroutineHandle(uint16_t offset)
{
	std::string *name = NULL;
	unsigned int x;

	for (x = 0; x < MONITOR_SUBROUTINES_OFFSET_LEN; x++) {
		if (monitor_subroutines_offsets[x].offset == offset) {
			name = new std::string(monitor_subroutines_offsets[x].name);
			break;
		}
	}

	return(name);
}

unsigned int getInstructionLen(uint8_t opcode)
{
	instruction_t *instr;
	
	instr = &instr_table[opcode];

	unsigned int len = instr->len;

	return(len);
}

#define BRANCH_INSTRUCTIONS_TABLE_LEN sizeof(BRANCH_INSTRUCTIONS_TABLE)
uint8_t BRANCH_INSTRUCTIONS_TABLE[] =
{
	0x10, // BPL rel
	0x30, // BMI rel
	0x50, // BVC rel
	0x70, // BVS rel
	0x80, // BRA rel
	0x90, // BCC rel
	0xB0, // BCS rel
	0xD0, // BNE rel
	0xF0, // BEQ rel
};

bool
Machine::isConditionalBranchInstruction(uint8_t opcode)
{
	bool isBranch = false;

	for (unsigned int x = 0; x < BRANCH_INSTRUCTIONS_TABLE_LEN; x++) {
		if (BRANCH_INSTRUCTIONS_TABLE[x] == opcode) {
			isBranch = true;
			break;
		}
	}

	return(isBranch);
}

/* 
 * XXX: Every conditionnal branch instruction should probably use this
 * function to minimize code duplication
 */
bool
Machine::isBranchTaken(uint8_t opcode)
{
	bool taken;

	switch(opcode)
	{
		case 0x10: // BPL
		{
			taken = ! registers.psw.f.n;
			break;
		}

		case 0x30: // BMI
		{
			taken = registers.psw.f.n;
			break;
		}

		case 0x50: // BVC
		{
			taken = ! registers.psw.f.v;
			break;
		}

		
		case 0x70: // BVS rel
		{
			taken = registers.psw.f.v;
			break;
		}

		case 0x80: // BRA rel
		{
			taken = true;
			break;
		}

		case 0x90: // BCC rel
		{
			taken = ! registers.psw.f.c;
			break;
		}

		case 0xB0: // BCS rel
		{
			taken = registers.psw.f.c;
			break;
		}

		case 0xD0: // BNE rel
		{
			taken = ! registers.psw.f.z;
			break;
		}

		case 0xF0: // BEQ rel
		{
			taken = registers.psw.f.z;
			break;
		}

		default:
		{
			taken = true;
			break;
		}
	}

	return(taken);
}

/* Returns number of bytes fetched for this instruction */
unsigned int
Machine::dumpInstruction(uint16_t offset)
{
	uint8_t opcode;
	instruction_t *instr;
	char strbuf[1024];
	int bufsize = sizeof(strbuf) - 1;

	opcode = memory->read(offset);

	instr = &instr_table[opcode];

	unsigned int len = instr->len;

	switch(len) {
		case 1:
		{
			snprintf(strbuf, bufsize, "%s", instr->str);
			break;
		}

		case 2:
		{
			uint8_t operand1 = memory->read(offset + 1);
			snprintf(strbuf, bufsize, instr->str, operand1);
			break;
		}

		case 3:
		{
			uint8_t operand1 = memory->read(offset + 1);
			uint8_t operand2 = memory->read(offset + 2);
			snprintf(strbuf, bufsize, instr->str, operand2, operand1);
			break;
		}

		default:
		{
			fprintf(stderr, "ERROR: Opcode (%02X) has len %u (should be between 1 and 3)\n", opcode, len);
			snprintf(strbuf, bufsize, "OOPS[%02X]\n", opcode);
			break;
		}
	}

	printf("%04X  ", offset);
	
	for (unsigned int x = 0; x < 4; x++) {
		if (x < instr->len)
			printf("%02X ", memory->read(offset + x));
		else
			printf("   ");
	}
	
	printf("%s", strbuf);

	if (isBranchInstruction(opcode)) {
		if (isRelativeBranchInstruction(opcode)) {
			uint16_t dest = offset + len + (int8_t) memory->read(offset + 1);

			printf("  ($%04X)", dest);
		}

		// Show whether a branch will be taken or not
		if (isConditionalBranchInstruction(opcode)) {
			if (isBranchTaken(opcode)) {
				printf(" [taken]");
			} else
				printf(" [not taken]");
		}

		if (opcode == 0x20 || opcode == 0x4C) { // JSR || JMP abs
			uint8_t operand1 = memory->read(offset + 1);
			uint8_t operand2 = memory->read(offset + 2);

			uint16_t dest = make16(operand2, operand1);

			std::string *subroutine = getSubroutineHandle(dest);

			if (subroutine)
				printf(" ; %s", subroutine->c_str());
		}
	}


	printf("\n");

	return(len);
}

void
Machine::executeNextInstruction(void)
{
	uint8_t opcode;
	instruction_t *instr;
	uint8_t operands[2];

	opcode = memory->read(registers.pc++);

	instr = &instr_table[opcode];

	switch(instr->len)
	{
		case 0: break;
			
		case 1: break;
			
		case 2: 
		{
			operands[0] = memory->read(registers.pc++);
			break;
		}
		
		case 3:
		{
			operands[0] = memory->read(registers.pc++);
			operands[1] = memory->read(registers.pc++);
			break;
		}
		
		default:
		{
			break;
		}
	}

	switch(opcode)
	{
		case 0x00: 
		{
			do_brk();
			break;
		}

		case 0x01:
		{
			uint16_t offset = get_indirect_indexed(operands[0]);
			uint8_t val = memory->read(offset);
			do_ora(val);
			break;
		}

		case 0x02:
		case 0x03:
		{
			break;
		}

		case 0x04:
		{
			do_tsb(operands[0]);
			break;
		}

		case 0x05:
		{
			uint8_t val = memory->read(operands[0]);
			do_ora(val);
			break;
		}

		case 0x06:
		{
			uint16_t offset = operands[0];
			do_asl_m(offset);
			break;
		}

		case 0x07:
		{
			break;
		}

		case 0x08:
		{
			do_php();
			break;
		}

		case 0x09:
		{
			do_ora(operands[0]);
			break;
		}

		case 0x0A:
		{
			do_asl_a();
			break;
		}

		case 0x0B:
		{
			break;
		}

		case 0x0C:
		{	
			// Shouldn't the operands be reversed?
			uint16_t offset = make16(operands[1], operands[0]);
			do_tsb(offset);
			break;
		}

		case 0x0D:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			uint8_t val = memory->read(offset);
			do_ora(val);
			break;
		}

		case 0x0E:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			do_asl_m(offset);
			break;
		}

		case 0x0F:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbr(0x01, val, operands[1]);
			break;
		}

		case 0x10:
		{
			do_bpl(operands[0]);
			break;
		}

		case 0x11:
		{
			uint16_t offset = get_indirect_indexed(operands[0]);
			uint8_t val = memory->read(offset);
			do_ora(val);
			break;
		}

		case 0x12:
		{
			uint16_t offset = get_indirect_zeropage(operands[0]);
			uint8_t val = memory->read(offset);
			do_ora(val);
			break;
		}

		case 0x13:
		{
			break;
		}

		case 0x14:
		{
			do_trb(operands[0]);
			break;
		}

		case 0x15:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			uint8_t val = memory->read(offset);
			do_ora(val);
			break;
		}

		case 0x16:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			do_asl_m(offset);
			break;
		}

		case 0x17:
		{
			break;		
		}

		case 0x18:
		{
			do_clc();
			break;		
		}

		case 0x19:
		{
			uint16_t offset = get_absolute_y(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_ora(val);
			break;
		}

		case 0x1A:
		{
			do_ina();
			break;
		}

		case 0x1B:
		{			
			break;
		}

		case 0x1C:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			do_trb(offset);
			break;
		}

		case 0x1D:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_ora(val);
			break;
		}

		case 0x1E:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			do_asl_m(offset);
			break;
		}

		case 0x1F:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbr(0x02, val, operands[1]);
			break;
		}

		case 0x20:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			do_jsr(offset);
			break;
		}

		case 0x21:
		{
			uint16_t offset = get_indexed_indirect(operands[0]);
			uint8_t val = memory->read(offset);
			do_and(val);
			break;
		}

		case 0x22:
		{
			break;
		}

		case 0x23:
		{
			break;
		}

		case 0x24:
		{
			uint8_t val = memory->read(operands[0]);
			do_bit(val);
			break;
		}

		case 0x25:
		{
			uint8_t val = memory->read(operands[0]);
			do_and(val);
			break;
		}

		case 0x26:
		{
			do_rol_m(operands[0]);
			break;
		}

		case 0x27:
		{
			break;
		}

		case 0x28:
		{
			do_plp();
			break;
		}

		case 0x29:
		{
			do_and(operands[0]);
			break;
		}

		case 0x2A:
		{
			do_rol_a();
			break;
		}

		case 0x2B:
		{
			break;
		}

		case 0x2C:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			uint8_t val = memory->read(offset);
			do_bit(val);
			break;
		}

		case 0x2D:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			uint8_t val = memory->read(offset);
			do_and(val);
			break;
		}

		case 0x2E:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			do_rol_m(offset);
			break;
		}

		case 0x2F:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbr(0x04, val, operands[1]);
			break;
		}

		case 0x30:
		{
			do_bmi(operands[0]);
			break;
		}

		case 0x31:
		{
			uint16_t offset = get_indirect_indexed(operands[0]);
			uint8_t val = memory->read(offset);
			do_and(val);
			break;
		}

		case 0x32:
		{
			uint16_t offset = get_indirect_zeropage(operands[0]);
			uint8_t val = memory->read(offset);
			do_and(val);
			break;
		}

		case 0x33:
		{
			break;
		}

		case 0x34:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			uint8_t val = memory->read(offset);
			do_bit(val);
			break;
		}

		case 0x35:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			uint8_t val = memory->read(offset);
			do_and(val);
			break;
		}

		case 0x36:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			do_rol_m(offset);
			break;
		}

		case 0x37:
		{
			break;
		}

		case 0x38:
		{
			do_sec();
			break;
		}

		case 0x39:
		{
			uint16_t offset = get_absolute_y(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_and(val);
			break;
		}

		case 0x3A:
		{
			do_dea();
			break;
		}

		case 0x3B:
		{
			break;
		}

		case 0x3C:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_bit(val);
			break;
		}

		case 0x3D:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_and(val);
			break;
		}

		case 0x3E:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			do_rol_m(offset);
			break;
		}

		case 0x3F:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbr(0x08, val, operands[1]);
			break;
		}

		case 0x40:
		{
			do_rti();
			break;
		}

		case 0x41:
		{
			uint16_t offset = get_indexed_indirect(operands[0]);
			uint8_t val = memory->read(offset);
			do_eor(val);
			break;
		}

		case 0x42:
		{
			break;
		}

		case 0x43:
		{
			break;
		}

		case 0x44:
		{
			break;
		}

		case 0x45:
		{
			uint8_t val = memory->read(operands[0]);
			do_eor(val);
			break;
		}

		case 0x46:
		{
			do_lsr_m(operands[0]);
			break;
		}

		case 0x47:
		{
			break;
		}

		case 0x48:
		{
			do_pha();
			break;
		}

		case 0x49:
		{			
			do_eor(operands[0]);
			break;
		}

		case 0x4A:
		{
			do_lsr_a();
			break;
		}

		case 0x4B:
		{
			break;
		}

		case 0x4C:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			do_jmp(offset);
			break;
		}

		case 0x4D:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			uint8_t val = memory->read(offset);
			do_eor(val);
			break;
		}

		case 0x4E:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			do_lsr_m(offset);
			break;
		}

		case 0x4F:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbr(0x10, val, operands[1]);
			break;
		}

		case 0x50:
		{
			do_bvc(operands[0]);
			break;
		}

		case 0x51:
		{
			uint16_t offset = get_indirect_indexed(operands[0]);
			uint8_t val = memory->read(offset);
			do_eor(val);
			break;
		}

		case 0x52:
		{
			uint16_t offset = get_indirect_zeropage(operands[0]);
			uint8_t val = memory->read(offset);
			do_eor(val);
			break;
		}

		case 0x53:
		{			
			break;
		}

		case 0x54:
		{
			break;
		}

		case 0x55:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			uint8_t val = memory->read(offset);
			do_eor(val);
			break;
		}

		case 0x56:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			do_lsr_m(offset);
			break;
		}

		case 0x57:
		{
			break;
		}

		case 0x58:
		{
			do_cli();
			break;
		}

		case 0x59:
		{
			uint16_t offset = get_absolute_y(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_eor(val);
			break;
		}

		case 0x5A:
		{
			do_phy();
			break;
		}

		case 0x5B:
		{
			break;
		}

		case 0x5C:
		{
			break;
		}

		case 0x5D:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_eor(val);
			break;
		}

		case 0x5E:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			do_lsr_m(offset);
			break;
		}

		case 0x5F:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbr(0x20, val, operands[1]);
			break;
		}

		case 0x60:
		{
			do_rts();
			break;
		}

		case 0x61:
		{
			uint16_t offset = get_indexed_indirect(operands[0]);
			uint8_t val = memory->read(offset);
			do_adc(val);
			break;
		}

		case 0x62:
		{
			break;
		}

		case 0x63:
		{
			break;
		}

		case 0x64:
		{
			uint16_t offset = operands[0];
			do_stz(offset);
			break;
		}

		case 0x65:
		{
			uint16_t offset = operands[0];
			uint8_t val = memory->read(offset);
			do_adc(val);
			break;
		}

		case 0x66:
		{
			uint16_t offset = operands[0];
			do_ror_m(offset);
			break;
		}

		case 0x67:
		{
			break;
		}

		case 0x68:
		{
			do_pla();
			break;
		}

		case 0x69:
		{
			do_adc(operands[0]);
			break;
		}

		case 0x6A:
		{
			do_ror_a();
			break;
		}

		case 0x6B:
		{
			break;
		}

		case 0x6C:
		{
			// XXX: NMOS versions have a bug where, if
			// offset = xxFF, than xxFF and xx00 are
			// fetched instead of xxFF and x100
			uint16_t offset = make16(operands[1], operands[0]);
			uint8_t low = memory->read(offset);
			uint8_t high = memory->read(offset + 1);

			offset = make16(high, low);
			do_jmp(offset);
			break;
		}

		case 0x6D:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			uint8_t val = memory->read(offset);
			do_adc(val);
			break;
		}

		case 0x6E:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			do_ror_m(offset);
			break;
		}

		case 0x6F:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbr(0x40, val, operands[1]);
			break;
		}

		case 0x70:
		{
			do_bvs(operands[0]);
			break;
		}

		case 0x71:
		{
			uint16_t offset = get_indirect_indexed(operands[0]);
			uint8_t val = memory->read(offset);
			do_adc(val);
			break;
		}

		case 0x72:
		{
			uint16_t offset = get_indirect_zeropage(operands[0]);
			uint8_t val = memory->read(offset);
			do_adc(val);
			break;
		}

		case 0x73:
		{
			break;
		}

		case 0x74:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			uint8_t val = memory->read(offset);
			do_stz(val);
			break;
		}

		case 0x75:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			uint8_t val = memory->read(offset);
			do_adc(val);
			break;
		}

		case 0x76:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			do_ror_m(offset);
			break;
		}

		case 0x77:
		{
			break;
		}

		case 0x78:
		{
			do_sei();
			break;
		}

		case 0x79:
		{
			uint16_t offset = get_absolute_y(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_adc(val);
			break;
		}

		case 0x7A:
		{
			do_ply();
			break;
		}

		case 0x7B:
		{
			break;
		}

		case 0x7C:
		{
			// XXX: NMOS versions have a bug where, if
			// offset = xxFF, then xxFF and xx00 are
			// fetched instead of xxFF and x100
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			uint8_t low = memory->read(offset);
			uint8_t high = memory->read(offset + 1);
			offset = make16(high, low);
			do_jmp(offset);
			break;
		}

		case 0x7D:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_adc(val);
			break;
		}

		case 0x7E:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			do_ror_m(offset);
			break;
		}

		case 0x7F:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbr(0x80, val, operands[1]);
			break;
		}

		case 0x80:
		{
			do_bra(operands[0]);
			break;
		}

		case 0x81:
		{
			uint16_t offset = get_indexed_indirect(operands[0]);
			do_sta(offset);
			break;
		}

		case 0x82:
		{
			break;
		}

		case 0x83:
		{
			break;
		}

		case 0x84:
		{
			uint16_t offset = operands[0];
			do_sty(offset);
			break;
		}

		case 0x85:
		{
			uint16_t offset = operands[0];
			do_sta(offset);
			break;
		}

		case 0x86:
		{
			uint16_t offset = operands[0];
			do_stx(offset);
			break;
		}

		case 0x87:
		{
			break;
		}

		case 0x88:
		{
			do_dey();
			break;
		}

		case 0x89:
		{
			do_bit(operands[0]);
			break;
		}

		case 0x8A:
		{
			do_txa();
			break;
		}

		case 0x8B:
		{
			break;
		}

		case 0x8C:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			do_sty(offset);
			break;
		}

		case 0x8D:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			do_sta(offset);
			break;
		}

		case 0x8E:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			do_stx(offset);
			break;
		}

		case 0x8F:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbs(0x01, val, operands[1]);
			break;
		}

		case 0x90:
		{
			do_bcc(operands[0]);
			break;
		}

		case 0x91:
		{
			uint16_t offset = get_indirect_indexed(operands[0]);
			do_sta(offset);
			break;
		}

		case 0x92:
		{
			uint16_t offset = get_indirect_zeropage(operands[0]);
			do_sta(offset);
			break;
		}

		case 0x93:
		{
			break;
		}

		case 0x94:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			do_sty(offset);
			break;
		}

		case 0x95:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			do_sta(offset);
			break;
		}

		case 0x96:
		{
			uint16_t offset = (operands[0] + registers.y) % 0xFF;
			do_stx(offset);
			break;
		}

		case 0x97:
		{
			break;
		}

		case 0x98:
		{
			do_tya();
			break;
		}

		case 0x99:
		{
			uint16_t offset = get_absolute_y(operands[0], operands[1]);
			do_sta(offset);
			break;
		}

		case 0x9A:
		{
			do_txs();
			break;
		}

		case 0x9B:
		{
			break;
		}

		case 0x9C:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			do_stz(offset);
			break;
		}

		case 0x9D:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			do_sta(offset);
			break;
		}

		case 0x9E:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			do_stz(offset);
			break;
		}

		case 0x9F:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbs(0x02, val, operands[1]);
			break;
		}

		case 0xA0:
		{
			do_ldy(operands[0]);
			break;
		}

		case 0xA1:
		{
			uint16_t offset = get_indexed_indirect(operands[0]);
			uint8_t val = memory->read(offset);
			do_lda(val);
			break;
		}

		case 0xA2:
		{
			do_ldx(operands[0]);
			break;
		}

		case 0xA3:
		{
			break;
		}

		case 0xA4:
		{
			uint16_t offset = operands[0];
			uint8_t val = memory->read(offset);
			do_ldy(val);
			break;
		}

		case 0xA5:
		{
			uint16_t offset = operands[0];
			uint8_t val = memory->read(offset);
			do_lda(val);
			break;
		}

		case 0xA6:
		{
			uint16_t offset = operands[0];
			uint8_t val = memory->read(offset);
			do_ldx(val);
			break;
		}

		case 0xA7:
		{
			break;
		}

		case 0xA8:
		{
			do_tay();
			break;
		}

		case 0xA9:
		{
			do_lda(operands[0]);
			break;
		}

		case 0xAA:
		{
			do_tax();
			break;
		}

		case 0xAB:
		{
			break;
		}

		case 0xAC:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			uint8_t val = memory->read(offset);
			do_ldy(val);
			break;
		}

		case 0xAD:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			uint8_t val = memory->read(offset);
			do_lda(val);
			break;
		}

		case 0xAE:
		{			
			uint16_t offset = make16(operands[1], operands[0]);
			uint8_t val = memory->read(offset);
			do_ldx(val);
			break;
		}

		case 0xAF:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbs(0x04, val, operands[1]);
			break;
		}

		case 0xB0:
		{
			do_bcs(operands[0]);
			break;
		}

		case 0xB1:
		{
			uint16_t offset = get_indirect_indexed(operands[0]);
			uint8_t val = memory->read(offset);
			do_lda(val);
			break;
		}

		case 0xB2:
		{
			uint16_t offset = get_indirect_zeropage(operands[0]);
			uint8_t val = memory->read(offset);
			do_lda(val);			
			break;
		}

		case 0xB3:
		{
			break;
		}

		case 0xB4:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			uint8_t val = memory->read(offset);
			do_ldy(val);
			break;
		}

		case 0xB5:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xFF;
			uint8_t val = memory->read(offset);
			do_lda(val);
			break;
		}

		case 0xB6:
		{
			uint16_t offset = (operands[0] + registers.y) % 0xFF;
			uint8_t val = memory->read(offset);
			do_ldx(val);
			break;
		}

		case 0xB7:
		{
			break;
		}

		case 0xB8:
		{
			do_clv();
			break;
		}

		case 0xB9:
		{
			uint16_t offset = get_absolute_y(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_lda(val);
			break;
		}

		case 0xBA:
		{
			do_tsx();
			break;
		}

		case 0xBB:
		{
			break;
		}

		case 0xBC:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_ldy(val);
			break;
		}

		case 0xBD:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_lda(val);
			break;
		}

		case 0xBE:
		{
			uint16_t offset = get_absolute_y(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_ldx(val);
			break;
		}

		case 0xBF:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbs(0x08, val, operands[1]);
			break;
		}

		case 0xC0:
		{
			do_cpy(operands[0]);
			break;
		}

		case 0xC1:
		{
			uint16_t offset = get_indexed_indirect(operands[0]);
			uint8_t val = memory->read(offset);
			do_cmp(val);
			break;
		}

		case 0xC2:
		{
			break;
		}

		case 0xC3:
		{
			break;
		}

		case 0xC4:
		{
			uint16_t offset = operands[0];
			uint8_t val = memory->read(offset);
			do_cpy(val);
			break;
		}

		case 0xC5:
		{
			uint16_t offset = operands[0];
			uint8_t val = memory->read(offset);
			do_cmp(val);
			break;
		}

		case 0xC6:
		{
			uint16_t offset = operands[0];
			do_dec(offset);
			break;
		}

		case 0xC7:
		{
			break;
		}

		case 0xC8:
		{
			do_iny();
			break;
		}

		case 0xC9:
		{
			do_cmp(operands[0]);
			break;
		}

		case 0xCA:
		{
			do_dex();
			break;
		}

		case 0xCB:
		{
			break;
		}

		case 0xCC:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			uint8_t val = memory->read(offset);
			do_cpy(val);
			break;
		}

		case 0xCD:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			uint8_t val = memory->read(offset);
			do_cmp(val);
			break;
		}

		case 0xCE:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			do_dec(offset);
			break;
		}

		case 0xCF:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbs(0x10, val, operands[1]);
			break;
		}

		case 0xD0:
		{
			do_bne(operands[0]);
			break;
		}

		case 0xD1:
		{
			uint16_t offset = get_indirect_indexed(operands[0]);
			uint8_t val = memory->read(offset);
			do_cmp(val);
			break;
		}

		case 0xD2:
		{
			uint16_t offset = get_indirect_zeropage(operands[0]);
			uint8_t val = memory->read(offset);
			do_cmp(val);
			break;
		}

		case 0xD3:
		{
			break;
		}

		case 0xD4:
		{
			break;
		}

		case 0xD5:
		{
			uint16_t offset = operands[0];
			uint8_t val = memory->read(offset);
			do_cmp(val);			
			break;
		}

		case 0xD6:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xff;
			do_dec(offset);
			break;
		}

		case 0xD7:
		{
			break;
		}

		case 0xD8:
		{
			do_cld();
			break;
		}

		case 0xD9:
		{
			uint16_t offset = get_absolute_y(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_cmp(val);
			break;
		}

		case 0xDA:
		{
			do_phx();
			break;
		}

		case 0xDB:
		{
			break;
		}

		case 0xDC:
		{
			break;
		}

		case 0xDD:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_cmp(val);
			break;
		}

		case 0xDE:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			do_dec(offset);
			break;
		}

		case 0xDF:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbs(0x20, val, operands[1]);
			break;
		}

		case 0xE0:
		{
			do_cpx(operands[0]);
			break;
		}

		case 0xE1:
		{
			uint16_t offset = get_indexed_indirect(operands[0]);
			uint8_t val = memory->read(offset);
			do_sbc(val);
			break;
		}

		case 0xE2:
		{
			break;
		}

		case 0xE3:
		{
			break;
		}

		case 0xE4:
		{
			uint16_t offset = operands[0];
			uint8_t val = memory->read(offset);
			do_cpx(val);
			break;
		}

		case 0xE5:
		{
			uint16_t offset = operands[0];
			uint8_t val = memory->read(offset);
			do_sbc(val);
			break;
		}

		case 0xE6:
		{
			uint16_t offset = operands[0];
			do_inc(offset);
			break;
		}

		case 0xE7:
		{
			break;
		}

		case 0xE8:
		{
			do_inx();
			break;
		}

		case 0xE9:
		{
			do_sbc(operands[0]);
			break;
		}

		case 0xEA:
		{
			do_nop();
			break;
		}

		case 0xEB:
		{
			break;
		}

		case 0xEC:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			uint8_t val = memory->read(offset);
			do_cpx(val);
			break;
		}

		case 0xED:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			uint8_t val = memory->read(offset);
			do_sbc(val);
			break;
		}

		case 0xEE:
		{
			uint16_t offset = make16(operands[1], operands[0]);
			do_inc(offset);
			break;
		}

		case 0xEF:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbs(0x40, val, operands[1]);
			break;
		}

		case 0xF0:
		{
			do_beq(operands[0]);
			break;
		}

		case 0xF1:
		{
			uint16_t offset = get_indirect_indexed(operands[0]);
			uint8_t val = memory->read(offset);
			do_sbc(val);
			break;
		}

		case 0xF2:
		{
			uint16_t offset = get_indirect_zeropage(operands[0]);
			uint8_t val = memory->read(offset);
			do_sbc(val);
			break;
		}

		case 0xF3:
		{
			break;
		}

		case 0xF4:
		{
			break;
		}

		case 0xF5:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xff;
			uint8_t val = memory->read(offset);
			do_sbc(val);
			break;
		}

		case 0xF6:
		{
			uint16_t offset = (operands[0] + registers.x) % 0xff;
			do_inc(offset);
			break;
		}

		case 0xF7:
		{
			break;
		}

		case 0xF8:
		{
			do_sed();
			break;
		}

		case 0xF9:
		{
			uint16_t offset = get_absolute_y(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_sbc(val);
			break;
		}

		case 0xFA:
		{
			do_plx();
			break;
		}

		case 0xFB:
		{
			break;
		}

		case 0xFC:
		{
			break;
		}

		case 0xFD:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			uint8_t val = memory->read(offset);
			do_sbc(val);			
			break;
		}

		case 0xFE:
		{
			uint16_t offset = get_absolute_x(operands[0], operands[1]);
			do_inc(offset);
			break;
		}

		case 0xFF:
		{
			uint8_t val = memory->read(operands[0]);
			do_bbs(0x80, val, operands[1]);
			break;
		}

		default:
		{
			cout << "Instruction 0x" << hex << opcode << " not implemented yet." << endl;
			break;
		}
	}

	this->cycles += instr->cycles;
}

uint16_t
Machine::get_absolute_x(uint8_t low, uint8_t high)
{
	// How is wrapping handled?
	uint16_t offset = make16(high, low) + registers.x;

	return(offset);
}

uint16_t
Machine::get_absolute_y(uint8_t low, uint8_t high)
{
	// How is wrapping handled?
	uint16_t offset = make16(high, low) + registers.y;

	return(offset);
}

/* Returns the effective memory address of (zp_offset,X) */
uint16_t
Machine::get_indexed_indirect(uint8_t zp_offset)
{
	uint16_t offset = (registers.x + zp_offset) % 0xFF;

	uint8_t low = memory->read(offset);
	uint8_t high = memory->read(offset + 1);

	uint16_t effective_address = make16(high, low);
	
	return(effective_address);
}

/* Returns the effective memory address of (zp_offset),Y */
uint16_t
Machine::get_indirect_indexed(uint8_t zp_offset)
{
	uint16_t offset = get_indirect_zeropage(zp_offset) + registers.y;
	
	return(offset);
}

/* Returns the effective memory address of (zp_offset) */
uint16_t
Machine::get_indirect_zeropage(uint8_t zp_offset)
{
	uint8_t low = memory->read(zp_offset);
	uint8_t high = memory->read(zp_offset + 1);
	uint16_t offset = make16(high, low);
	
	return(offset);
}

void
Machine::do_adc(uint8_t val)
{
	uint16_t result;
	int16_t sResult;

	// printf("ADC(%02X,%02X,%02X)\n", val, registers.a, registers.psw.f.c);

	if (registers.psw.f.d) {
		// BCD mode
		result = from_bcd(registers.a) + from_bcd(val) + registers.psw.f.c;
		sResult = (int8_t) from_bcd(registers.a) + (int8_t) from_bcd(val) + registers.psw.f.c;
		registers.a = to_bcd(result % 100);
		registers.psw.f.c = (result > 99);
	} else {
		// Binary mode
		sResult = (int8_t) registers.a + (int8_t) val + registers.psw.f.c;
		result = registers.a + val + registers.psw.f.c;
		registers.a = (result & 0x00FF);
		registers.psw.f.c = (result > 0xFF);
	}

	// One reference says "result == 0", but I think it would
	// makes more sense if "A == 0", since for other operations is
	// essentially checks if <reg> is zero.
	registers.psw.f.v = (sResult < -128 || sResult > 127);
	registers.psw.f.z = (registers.a == 0);  // 65C02 mode. In 6502, 'result' is tested.
	registers.psw.f.n = ((registers.a & 0x80) != 0);

	// printf("Result: $%04X (%d)  A: $%02X  V:%d  C:%d  N:%d\n", result, result, registers.a, registers.psw.f.v, registers.psw.f.c, registers.psw.f.n);
}

/*
 * SBC (SuBstract with Carry)
 * When the carry is clear, SBC NUM performs the calculation A = A - NUM - 1
 * When the carry is set, SBC NUM performs the calculation A = A - NUM
 * 
 * Result:
 * V flag is set if the [signed] result is outside the -128..127 range
 * C flag is clear if the [unsigned] operation had to borrow
*/
void
Machine::do_sbc(uint8_t val)
{
	uint16_t result;
	int16_t sResult;

	// printf("SBC(%02X,%02X,%02X)\n", registers.a, val, registers.psw.f.c);

	if (registers.psw.f.d) {
		// BCD mode
		result = from_bcd(registers.a) - from_bcd(val) - (! registers.psw.f.c);
		sResult = (int8_t) from_bcd(registers.a) - (int8_t) from_bcd(val) - (! registers.psw.f.c);
		registers.a = to_bcd(result & 0x00FF);
	} else {
		// Binary mode.
		result = registers.a - val - (! registers.psw.f.c);
		sResult = (int8_t) registers.a - (int8_t) val - (! registers.psw.f.c);
		registers.a = result & 0x00FF;
	}

	// int16_t sResult = (int16_t) result;

	// XXX: There is something wrong: SBC(A:0x01,VAL:#$D0,C:1)
	// returns C:1 when it should be C:0
	//registers.psw.f.c = (sResult >= 0);
	registers.psw.f.c = !(result > 0xFF);
	registers.psw.f.n = ((registers.a & 0x80) != 0);
	registers.psw.f.v = (sResult < -128 || sResult > 127);
	registers.psw.f.z = (registers.a == 0); // 65C02 mode. In 6502, 'result' is tested.

	//printf("Result: %d  uresult: $%04X   c:%d  n:%d  v:%d  z:%dn", sResult, result, registers.psw.f.c, registers.psw.f.n, registers.psw.f.v, registers.psw.f.z);
}

void
Machine::do_and(uint8_t val)
{
	registers.a = registers.a & val;

	registers.psw.f.z = (registers.a == 0);
	registers.psw.f.n = ((registers.a & 0x80) != 0);
}

void
Machine::do_asl_a(void)
{
	registers.psw.f.c = ((registers.a & 0x80) != 0);

	registers.a = registers.a << 1;

	registers.psw.f.z = (registers.a == 0);
	registers.psw.f.n = ((registers.a & 0x80) != 0);
}

void
Machine::do_asl_m(uint16_t offset)
{
	uint8_t val = memory->read(offset);

	registers.psw.f.c = ((val & 0x80) > 0);

	val = val << 1;

	memory->write(offset, val);

	registers.psw.f.z = (val == 0);
	registers.psw.f.n = ((val & 0x80) > 0);
}

void
Machine::do_bbr(uint8_t bit, uint8_t val, int8_t rel)
{	
	if ((val & bit) == 0)
		registers.pc += rel;
}

void
Machine::do_bbs(uint8_t bit, uint8_t val, int8_t rel)
{	
	if ((val & bit) > 0)
		registers.pc += rel;
}

void
Machine::do_bcc(int8_t rel)
{	
	if (! registers.psw.f.c)
		registers.pc += rel;
}

void
Machine::do_bcs(int8_t rel)
{	
	if (registers.psw.f.c)
		registers.pc += rel;
}

void
Machine::do_beq(int8_t rel)
{	
	if (registers.psw.f.z)
		registers.pc += rel;
}

void
Machine::do_bit(uint8_t val)
{	
	// Depending on the reference, V and N are either applied on
	// the memory value or the AND'd value. The BASIC "BIT $11"
	// test at $DAEE leads me to think they come directly from the
	// memory value.
	registers.psw.f.v = ((val & 0x40) != 0);
	registers.psw.f.n = ((val & 0x80) != 0);

	val = val & registers.a;

	registers.psw.f.z = (val == 0);
}

void
Machine::do_bmi(int8_t rel)
{
	if (registers.psw.f.n)
		registers.pc += rel;
}

void
Machine::do_bne(int8_t rel)
{
	if (! registers.psw.f.z)
		registers.pc += rel;
}

void
Machine::do_bpl(int8_t rel)
{	
	if (! registers.psw.f.n)
		registers.pc += rel;
}

void
Machine::do_bra(int8_t rel)
{
	registers.pc += rel;
}

void
Machine::do_brk(void)
{
	// Yes, the byte following a BRK is skipped. Apparently this
	// is normal.
	registers.pc++;

	uint8_t low = get_low(registers.pc);
	uint8_t high = get_high(registers.pc);
	
	push_stack(high);
	push_stack(low);

	// BRK flag is only set on the stack.
	spc_flags_t flags;
	flags.val = registers.psw.val;
	flags.f.b = 1;

	push_stack(flags.val);

	low = memory->read(0xFFFE);
	high = memory->read(0xFFFF);

	registers.pc = make16(high, low);
}

void
Machine::do_bvc(int8_t rel)
{
	if (! registers.psw.f.v)
		registers.pc += rel;
}

void
Machine::do_bvs(int8_t rel)
{
	if (registers.psw.f.v)
		registers.pc += rel;
}

void
Machine::do_clc(void)
{
	registers.psw.f.c = 0;
}

void
Machine::do_cld(void)
{
	registers.psw.f.d = 0;
}

void
Machine::do_cli(void)
{
	registers.psw.f.i = 0;
}

void
Machine::do_clv(void)
{
	registers.psw.f.v = 0;
}

void
Machine::compare(uint8_t reg, uint8_t val)
{
	uint8_t result = reg - val;

	registers.psw.f.c = (reg >= val);
	registers.psw.f.z = (result == 0);
	registers.psw.f.n = ((result & 0x80) != 0);
}

void
Machine::do_cmp(uint8_t val)
{
	compare(registers.a, val);
}

void
Machine::do_cpx(uint8_t val)
{
	compare(registers.x, val);
}

void
Machine::do_cpy(uint8_t val)
{
	compare(registers.y, val);
}

void
Machine::do_dea(void)
{
	registers.a--;

	registers.psw.f.z = (registers.a == 0);
	registers.psw.f.n = ((registers.a & 0x80) != 0);
}

void
Machine::do_dec(uint16_t offset)
{
	uint8_t val = memory->read(offset);

	val--;

	memory->write(offset, val);

	registers.psw.f.z = (val == 0);
	registers.psw.f.n = ((val & 0x80) != 0);
}

void
Machine::do_dex(void)
{
	registers.x--;

	registers.psw.f.z = (registers.x == 0);
	registers.psw.f.n = ((registers.x & 0x80) != 0);
}

void
Machine::do_dey(void)
{
	registers.y--;

	registers.psw.f.z = (registers.y == 0);
	registers.psw.f.n = ((registers.y & 0x80) != 0);
}

void
Machine::do_eor(uint8_t val)
{
	registers.a = registers.a ^ val;

	registers.psw.f.z = (registers.a == 0);
	registers.psw.f.n = ((registers.a & 0x80) != 0);
}

void
Machine::do_ina(void)
{
	registers.a++;
	registers.psw.f.z = (registers.a == 0);
	registers.psw.f.n = ((registers.a & 0x80) != 0);
}

void
Machine::do_inx(void)
{
	registers.x++;
	registers.psw.f.z = (registers.x == 0);
	registers.psw.f.n = ((registers.x & 0x80) != 0);
}

void
Machine::do_iny(void)
{
	registers.y++;
	registers.psw.f.z = (registers.y == 0);
	registers.psw.f.n = ((registers.y & 0x80) != 0);
}

void
Machine::do_inc(uint16_t offset)
{
	uint8_t val = memory->read(offset);
	val++;
	memory->write(offset, val);

	registers.psw.f.z = (val == 0);
	registers.psw.f.n = ((val & 0x80) != 0);	
}

void
Machine::do_jmp(uint16_t offset)
{
	registers.pc = offset;
}

void
Machine::do_jsr(uint16_t offset)
{
	// For some reason, (pc - 1) is pushed on the stack rather
	// than pc.
	uint8_t low = get_low(registers.pc - 1);
	uint8_t high = get_high(registers.pc - 1);
	
	push_stack(high);
	push_stack(low);
	
	registers.pc = offset;
}

void
Machine::do_lda(uint8_t val)
{
	registers.a = val;
	registers.psw.f.z = (registers.a == 0);
	registers.psw.f.n = ((registers.a & 0x80) != 0);
}

void
Machine::do_ldx(uint8_t val)
{
	registers.x = val;
	registers.psw.f.z = (registers.x == 0);
	registers.psw.f.n = ((registers.x & 0x80) != 0);
}

void
Machine::do_ldy(uint8_t val)
{
	registers.y = val;
	registers.psw.f.z = (registers.y == 0);
	registers.psw.f.n = ((registers.y & 0x80) != 0);
}

uint8_t
Machine::shift_right(uint8_t val)
{
	registers.psw.f.c = val & 0x01;
	registers.psw.f.n = 0;

	val = (val >> 1);

	registers.psw.f.z = (val == 0);

	return(val);
}

void
Machine::do_lsr_a(void)
{
	registers.a = shift_right(registers.a);
}

void
Machine::do_lsr_m(uint16_t offset)
{
	uint8_t val = memory->read(offset);

	val = shift_right(val);

	memory->write(offset, val);
}

void
Machine::do_ora(uint8_t val)
{
	registers.a |= val;
	registers.psw.f.z = (registers.a == 0);
	registers.psw.f.n = ((registers.a & 0x80) != 0);
}

void
Machine::do_nop(void)
{
}

void
Machine::do_pha(void)
{
	push_stack(registers.a);
}

void
Machine::do_php(void)
{
	push_stack(registers.psw.val);
}

void
Machine::do_phx(void)
{
	push_stack(registers.x);
}

void
Machine::do_phy(void)
{
	push_stack(registers.y);
}

void
Machine::do_pla(void)
{
	registers.a = pop_stack();
	registers.psw.f.n = ((registers.a & 0x80) != 0);
	registers.psw.f.z = (registers.a == 0);
}

void
Machine::do_plp(void)
{
	registers.psw.val = pop_stack();
}

void
Machine::do_plx(void)
{
	// Not a 6502 instruction
	registers.x = pop_stack();

	// XXX: possibly supposed to check reg A
	registers.psw.f.n = ((registers.x & 0x80) != 0);
	registers.psw.f.z = (registers.x == 0);
}

void
Machine::do_ply(void)
{
	// Not a 6502 instruction
	registers.y = pop_stack();

	// XXX: possibly supposed to check reg A
	registers.psw.f.n = ((registers.y & 0x80) != 0);
	registers.psw.f.z = (registers.y == 0);
}

uint8_t
Machine::rotate_left(uint8_t val)
{
	uint8_t new_carry = ((val & 0x80) != 0);

	val = (val << 1) | registers.psw.f.c;

	registers.psw.f.c = new_carry;
	registers.psw.f.n = ((val & 0x80) != 0);
	registers.psw.f.z = (val == 0);

	return(val);
}

void
Machine::do_rol_a(void)
{
	registers.a = rotate_left(registers.a);
}

void
Machine::do_rol_m(uint16_t offset)
{
	uint8_t val = memory->read(offset);

	val = rotate_left(val);

	memory->write(offset, val);
}

uint8_t
Machine::rotate_right(uint8_t val)
{
	uint8_t temp_carry = (val & 0x01);

	val = (val >> 1);

	registers.psw.f.n = registers.psw.f.c;

	if (registers.psw.f.c)
		val |= 0x80;

	registers.psw.f.c = temp_carry;
	registers.psw.f.z = (val == 0);

	return(val);
}

void
Machine::do_ror_a(void)
{
	registers.a = rotate_right(registers.a);
}

void
Machine::do_ror_m(uint16_t offset)
{
	uint8_t val = memory->read(offset);

	val = rotate_right(val);

	memory->write(offset, val);
}

void
Machine::do_rti(void)
{
	registers.psw.val = pop_stack();

	uint8_t low = pop_stack();
	uint8_t high = pop_stack();
	
	registers.pc = make16(high, low);
}

void
Machine::do_rts(void)
{
	uint8_t low = pop_stack();
	uint8_t high = pop_stack();
	
	registers.pc = make16(high, low) + 1;
}

void
Machine::do_sec(void)
{
	registers.psw.f.c = 1;
}

void
Machine::do_sed(void)
{
	printf("Warning: BCD-mode enabled. This may or not work.\n");
	registers.psw.f.d = 1;
}

void
Machine::do_sei(void)
{
	registers.psw.f.i = 1;
}

void
Machine::do_sta(uint16_t offset)
{
	memory->write(offset, registers.a);
}

void
Machine::do_stx(uint16_t offset)
{
	memory->write(offset, registers.x);
}

void
Machine::do_sty(uint16_t offset)
{
	memory->write(offset, registers.y);
}

void
Machine::do_stz(uint16_t offset)
{
	memory->write(offset, 0x00);
}

void
Machine::do_tax(void)
{
	registers.x = registers.a;
	registers.psw.f.n = ((registers.x & 0x80) != 0);
	registers.psw.f.z = (registers.x == 0);
}

void
Machine::do_tay(void)
{
	registers.y = registers.a;
	registers.psw.f.n = ((registers.y & 0x80) != 0);
	registers.psw.f.z = (registers.y == 0);
}

void
Machine::do_tsb(uint16_t offset)
{
	uint8_t val = memory->read(offset);

	registers.psw.f.z = ((registers.a & val) == 0);

	memory->write(offset, val | registers.a);
}

void
Machine::do_trb(uint16_t offset)
{
	uint8_t val = memory->read(offset);

	registers.psw.f.z = ((registers.a & val) != 0);

	memory->write(offset, val | ~registers.a);
}

void
Machine::do_tsx(void)
{
	registers.x = registers.sp;
	registers.psw.f.n = ((registers.x & 0x80) != 0);
	registers.psw.f.z = (registers.x == 0);
}

void
Machine::do_txa(void)
{
	registers.a = registers.x;
	registers.psw.f.n = ((registers.a & 0x80) != 0);
	registers.psw.f.z = (registers.a == 0);
}

void
Machine::do_txs(void)
{
	registers.sp = registers.x;
}

void
Machine::do_tya(void)
{
	registers.a = registers.y;
	registers.psw.f.n = ((registers.a & 0x80) != 0);
	registers.psw.f.z = (registers.a == 0);
}

uint8_t
Machine::pop_stack(void)
{
	registers.sp++;

	uint16_t offset = OFFSET_PAGE_1 | registers.sp;

	uint8_t val = memory->read(offset);

	return(val);
}

void
Machine::push_stack(uint8_t val)
{
	uint16_t offset = OFFSET_PAGE_1 | registers.sp;

	memory->write(offset, val);

	registers.sp--;
}

bool
Machine::testCPU(void)
{
	printf("Starting CPU test..\n");

	uint16_t offset = 0x300;
/*
	for(unsigned int x = 0; x < SBC_TEST_TABLE_LEN; x++) {
		memory->write(offset + x, SBC_TEST_TABLE[x]);
	}

	setPC(0x300);
	interactive();

	uint8_t val = memory->read(0x380);
	printf("SBC Test result: %02X\n", val);

	assert(val == 0x00);
*/

	/* Test ADC */
	registers.a = 0x00;
	registers.psw.f.c = 0;
	do_adc(0xFF);
	assert(registers.psw.f.v == 0 && registers.psw.f.c == 0);

	registers.a = -10;
	registers.psw.f.c = 0;
	do_adc(-117);
	assert(registers.psw.f.v == 0 && registers.psw.f.c == 1);

	registers.a = -20;
	registers.psw.f.c = 0;
	do_adc(-117);
	assert(registers.psw.f.v == 1 && registers.psw.f.c == 1);

	registers.a = 100;
	registers.psw.f.c = 0;
	do_adc(27);
	assert(registers.psw.f.v == 0 && registers.psw.f.c == 0);

	registers.a = 100;
	registers.psw.f.c = 0;
	do_adc(100);
	assert(registers.psw.f.v == 1 && registers.psw.f.c == 0);

	registers.a = 255;
	registers.psw.f.c = 0;
	do_adc(255);
	dumpRegisters();
	assert(registers.psw.f.v == 0 && registers.psw.f.c == 1);

	registers.psw.f.d = 0;

	for(int carry = 0; carry <= 1; carry++) {
		for(short int x = -128; x < 127; x++) {
			for (short int y = 127; y >= -128; y--) {
				registers.a = x;
				registers.psw.f.c = carry;
				do_adc(y);

				short int result = x + y + carry;
				short unsigned int uresult = (uint8_t) x + (uint8_t) y + carry;

				int vflag;
				int cflag;
				if (result > 127 || result < -128)
					vflag = 1;
				else
					vflag = 0;

				if (uresult > 255)
					cflag = 1;
				else
					cflag = 0;

				assert(vflag == registers.psw.f.v);

				if (cflag != registers.psw.f.c)
					printf("%d + %d = %d (%04X). c should be %d but is %d\n", x, y, result, uresult, cflag, registers.psw.f.c);

				assert(cflag == registers.psw.f.c);

				registers.a = x;
				registers.psw.f.c = carry;
				do_sbc(y);

				result = x - y - !carry;
				uresult = (uint8_t) x - (uint8_t) y - !carry;
				if (result > 127 || result < -128)
					vflag = 1;
				else
					vflag = 0;

				//if (x > (y + !carry))
				if (! (uresult > 255) )
					cflag = 1;
				else
					cflag = 0;

				assert(vflag == registers.psw.f.v);
				
				if (cflag != registers.psw.f.c)
					printf("%d - %d - %d = %d (%u). c should be %d but is %d\n", x, y, ! carry, result, uresult, cflag, registers.psw.f.c);

				assert(cflag == registers.psw.f.c);
			}
		}
	}

	/* Test BCD routines */
	uint8_t bcd_result = from_bcd(0x45) + from_bcd(0x05);
	assert(bcd_result == 50);

	bcd_result = from_bcd(0x10) + from_bcd(0x10);
	assert(bcd_result == 20);

	bcd_result = to_bcd(74);
	assert(bcd_result == 0x74);

	bcd_result = to_bcd(98);
	assert(bcd_result == 0x98);

	bcd_result = to_bcd(0);
	assert(bcd_result == 0x00);

	bcd_result = to_bcd(1);
	assert(bcd_result == 0x01);

	offset = 0x0000;
	setPC(offset);

	dumpRegisters();

	memory->write(offset++, 0xA9); // LDA #$00
	memory->write(offset++, 0x00);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.a == 0x00 && registers.psw.f.z == 1);

	memory->write(offset++, 0xA9); // LDA #$A5
	memory->write(offset++, 0xA5);
	dumpInstruction(registers.pc);
	executeNextInstruction();	
	dumpRegisters();
	assert(registers.a == 0xA5 && registers.psw.f.z == 0);

	memory->write(offset++, 0xA2); // LDX #$FF
	memory->write(offset++, 0xFF);
	dumpInstruction(registers.pc);
	executeNextInstruction();	
	dumpRegisters();
	assert(registers.x == 0xFF && registers.psw.f.z == 0);

	memory->write(offset++, 0x9A); // TXS
	dumpInstruction(registers.pc);
	executeNextInstruction();	
	dumpRegisters();
	assert(registers.sp == 0xFF);

	/* Test stack operations */
	memory->write(offset++, 0x48); // PHA
	dumpInstruction(registers.pc);
	executeNextInstruction();	
	dumpRegisters();
	uint8_t mem = memory->read(0x01FF);
	assert(registers.sp == 0xFE && mem == 0xA5);

	memory->write(offset++, 0x68); // PLA
	dumpInstruction(registers.pc);
	executeNextInstruction();	
	dumpRegisters();
	assert(registers.sp == 0xFF && registers.a == 0xA5);

	uint8_t low = get_low(offset + 3);
	uint8_t high = get_high(offset + 3);

	memory->write(offset++, 0x20); // JSR
	memory->write(offset++, low);
	memory->write(offset++, high);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.sp == 0xFD && registers.pc == offset);

	/* Test carry flag */
	memory->write(offset++, 0x38); // SEC
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.psw.f.c == 1);

	memory->write(offset++, 0x18); // CLC
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.psw.f.c == 0);

	memory->write(offset++, 0xD8); // CLD
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.psw.f.d == 0);

	memory->write(offset++, 0xA9); // LDA #$01
	memory->write(offset++, 0x01);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.a == 0x01);

	/* ADC without Carry flag set */
	memory->write(offset++, 0x69); // ADC #$01
	memory->write(offset++, 0x01);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.a == 0x02);

	/* ADC with Carry flag set */
	memory->write(offset++, 0x38); // SEC
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0xA9); // LDA #$01
	memory->write(offset++, 0x01);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0x69); // ADC #$01
	memory->write(offset++, 0x01);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.a == 0x03);

	/* ADC overflow */
	memory->write(offset++, 0x18); // CLC
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0xA9); // LDA #$FF
	memory->write(offset++, 0xFF);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0x69); // ADC #$01
	memory->write(offset++, 0x01);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	// On a 6502:
	// assert(registers.a == 0x00 && registers.psw.f.c == 1 && registers.psw.f.z == 0 && registers.psw.f.v == 0 && registers.psw.f.n == 0);
	// On a 65C02:
	assert(registers.a == 0x00 && registers.psw.f.c == 1 && registers.psw.f.z == 1 && registers.psw.f.v == 0 && registers.psw.f.n == 0);

	/* ADC overflow */
	memory->write(offset++, 0x18); // CLC
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0xA9); // LDA #$80
	memory->write(offset++, 0x80);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0x69); // ADC #$0F
	memory->write(offset++, 0x0F);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.a == 0x8F && registers.psw.f.c == 0 && registers.psw.f.z == 0 && registers.psw.f.v == 0 && registers.psw.f.n == 1);

	/* ADC overflow */
	memory->write(offset++, 0x18); // CLC
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0xA9); // LDA #$F0
	memory->write(offset++, 0xF0);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0x69); // ADC #$F0
	memory->write(offset++, 0xF0);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.a == 0xE0 && registers.psw.f.c == 1 && registers.psw.f.z == 0 && registers.psw.f.v == 0 && registers.psw.f.n == 1);

	/* SBC with carry */
	memory->write(offset++, 0x38); // SEC
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0xA9); // LDA #$04
	memory->write(offset++, 0x04);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0xE9); // SBC #$02
	memory->write(offset++, 0x02);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.a == 0x02 && registers.psw.f.c == 1 && registers.psw.f.z == 0 && registers.psw.f.v == 0 && registers.psw.f.n == 0);

	/* Test SBC zero flag */
	memory->write(offset++, 0x38); // SEC
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0xA9); // LDA #$04
	memory->write(offset++, 0x04);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0xE9); // SBC #$04
	memory->write(offset++, 0x04);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.a == 0x00 && registers.psw.f.c == 1 && registers.psw.f.z == 1 && registers.psw.f.v == 0 && registers.psw.f.n == 0);

	/* SBC without carry flag */
	memory->write(offset++, 0x18); // CLC
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0xA9); // LDA #$04
	memory->write(offset++, 0x04);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0xE9); // SBC #$02
	memory->write(offset++, 0x02);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.a == 0x01 && registers.psw.f.c == 1 && registers.psw.f.z == 0 && registers.psw.f.v == 0 && registers.psw.f.n == 0);

	/* SBC negative values */
	memory->write(offset++, 0x38); // SEC
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0xA9); // LDA #$04
	memory->write(offset++, 0x04);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	memory->write(offset++, 0xE9); // SBC #$05
	memory->write(offset++, 0x05);
	dumpInstruction(registers.pc);
	executeNextInstruction();
	dumpRegisters();
	assert(registers.a == 0xFF && registers.psw.f.c == 0 && registers.psw.f.z == 0 && registers.psw.f.v == 0 && registers.psw.f.n == 1);

	printf("All tests OK!\n");

	return(true);
}

void
Machine::dumpStack(uint16_t len)
{
	uint16_t x;
	uint8_t sp = registers.sp;

	for (x = 0; x < len; x++) {
		uint16_t offset = (uint16_t) sp | OFFSET_PAGE_1;
		printf("%04X (sp+%d): %02X\n", offset, x, memory->read(offset));
		sp++;
	}
}

void
Machine::dumpMemory(uint16_t offset, uint16_t len)
{
	int x = 0;

	while(len) {
		printf("%04X  ", offset);
		
		for (x = 0; x < 16 && len > 0; x++) {
			printf("%02X ", memory->read(offset + x));
			len--;
		}
		
		printf("   ");

		for (x = 0; x < 16; x++) {
			uint8_t c = memory->read(offset + x);
			if (! isprint(c))
				c = '.';

			printf("%c", c);

		}
		
		printf("\n");

		offset += 16;
	}
}

void
Machine::setPCBreakpoint(uint16_t offset)
{
	pcBreakpointOffset = offset;
	pcBreakpointEnabled = true;
}

/*
 * Run code until a user quits or a breakpoint is hit
 */
void
Machine::run(void)
{
	struct timespec ts = { 0, 977 * 100 };
	SDL_Event event;
	bool quit = false;

	while(! quit) {
		if (pcBreakpointEnabled && getPC() == pcBreakpointOffset) {
			printf("Breakpoint on PC($%04X)\n", pcBreakpointOffset);
			pcBreakpointEnabled = false;
			// Eww. This return is not clean..
			return;
		}

		if (traceInstructions) {
			dumpInstruction(getPC());
		}

		executeNextInstruction();

		// 1.023MHz / 60 Hz == 17050 cycles between refreshes
		if (cycles > 17050 * 10) {
			screen->redraw();
			cycles = 0;
		}
					
		// Sleeping about ~100us every 100 cycles is easier on
		// the CPU than sleeping 977ns every cycle. Same goes
		// for the event polling
		if (cycles % 100 == 0) {
			int nbEvents = SDL_PollEvent(&event);

			if (nbEvents > 0) {
				// printf("Found %d events waiting.\n", nbEvents);
				
				switch( event.type ){
					/* Keyboard event */
					case SDL_KEYDOWN:
					{
						if (event.key.keysym.sym > 0) {
							printf("Key event! %c (0x%02X)\n", event.key.keysym.sym, event.key.keysym.sym);
							uint8_t k = toupper(event.key.keysym.sym & 0xFF);
							MemorySoftSwitch *switches = (MemorySoftSwitch *) memory->getRegion(REGION_SOFT_SWITCHES);
							switches->setKeyboardData(k);
							switches->doKeyboardStrobe();
						}
						break;
					}
					
					/* SDL_QUIT event (window close) */
					case SDL_QUIT:
						quit = true;
						break;
						
					default:
						break;
				}
			} else if (nbEvents < 0) {
				fprintf(stderr, "SDL_PollEvents(): %s\n", SDL_GetError());
				exit(1);
			}
					       
			nanosleep(&ts, NULL);
		}
	}
}

// This struct only exists to permit using a switch() statement
struct command_struct {
	const char *name;
	int value;
};

enum command_values {
	CMD_HELP,
	CMD_BREAKPOINT,
	CMD_DISASM,
	CMD_DUMP,
	CMD_INCLUDE,
	CMD_JUMP,
	CMD_KEY,
	CMD_LOAD,
	CMD_QUIT,
	CMD_REDRAW,
	CMD_RET,
	CMD_RUN,
	CMD_SHOW_REGS,
	CMD_SHOW_STACK,
	CMD_STEP,
	CMD_TRACE,
	CMD_WRITE,
	CMD_UNKNOWN
};

struct command_struct COMMAND_TABLE[] =
{
	{ "?",      CMD_HELP },
	{ "b",      CMD_BREAKPOINT },
	{ "break",  CMD_BREAKPOINT },
	{ "d",      CMD_DISASM },
	{ "disasm", CMD_DISASM },
	{ "dump",   CMD_DUMP },
	{ "h",      CMD_HELP },
	{ "help",   CMD_HELP },
	{ "include", CMD_INCLUDE },
	{ "j",      CMD_JUMP },
	{ "jump",   CMD_JUMP },
	{ "key",    CMD_KEY  },
	{ "load",   CMD_LOAD },
	{ "p",      CMD_DUMP },
	{ "q",      CMD_QUIT },
	{ "quit",   CMD_QUIT },
	{ "redraw", CMD_REDRAW },
	{ "r",      CMD_RUN },
	{ "ret",    CMD_RET },
	{ "run",    CMD_RUN },
	{ "sr",     CMD_SHOW_REGS },
	{ "ss",     CMD_SHOW_STACK },
	{ "trace",  CMD_TRACE },
	{ "x",      CMD_STEP },
	{ "w",      CMD_WRITE },
};

#define COMMAND_TABLE_LEN (sizeof(COMMAND_TABLE) / sizeof(struct command_struct))

void
Machine::interactive(void)
{
	std::string buf;

	// run();

	while(! cin.eof())
	{		
		cout << endl;

		dumpInstruction(getPC());

		cout << "> ";
		getline(cin, buf);
		if (buf.size() == 0) {
			executeNextInstruction();
			dumpRegisters();
			continue;
		}

		std::string cmd;
		std::string arg("");

		size_t pos = buf.find(' ');
		if (pos != buf.npos) {
			cmd = buf.substr(0, pos);
			arg = buf.substr(pos);
		} else {
			cmd = buf;
		}

		// enum command_values command = CMD_UNKNOWN;
		int command = CMD_UNKNOWN;

		for (unsigned int x = 0; x < COMMAND_TABLE_LEN; x++) {
			if (cmd.compare(COMMAND_TABLE[x].name) == 0)
				command = COMMAND_TABLE[x].value;
		}

		switch(command) {
			case CMD_HELP:
			{
				printf("Help:\n");
				printf("b $addr        Breakpoint on $addr\n");
				printf("d [$addr]      Disassemble at PC, or $addr if it's given\n");
				printf("disasm [$addr] Disassemble at PC, or $addr if it's given\n");
				printf("dump $addr     Print hex data at $addr\n");
				printf("h              This help\n");
				printf("include $file  Read $file as if it had been typed on screen\n");
				printf("load $file     Put $file in disk 0\n");
				printf("j $addr        Jump to $addr\n");
				printf("jump $addr     Jump to $addr\n");
				printf("key $xx        Emulate key $xx being typed-in\n");
				printf("p $addr        Print data at $addr\n");
				printf("q              Quit\n");
				printf("r              Run\n");
				printf("redraw         Redraw the screen\n");
				printf("ret            Run until return from JSR\n");
				printf("run            Run\n");
				printf("sr             Show Registers\n");
				printf("ss             Show Stack\n");
				printf("trace          Trace instructions when running\n");
				printf("x              Step over\n");
				printf("w $addr $byte  Write $byte at $addr (ie, POKE)\n");
				printf("<enter>        Execute next instruction\n");
				break;
			}

			case CMD_BREAKPOINT:
			{
				std::istringstream istr(arg);
				uint16_t offset;

				if (istr >> hex >> offset) {
					setPCBreakpoint(offset);
					run();
				} else {
					cout << "Error: Invalid argument '" << arg << "'" << endl;
					cout << "Usage: b $addr" << endl;
					cout << "Example: b 0xff00" << endl;
				}

				break;
			}

			case CMD_DISASM:
			{
				uint16_t offset = getPC();

				if (arg.size() > 0) {
					std::istringstream istr(arg);
				
					if (! (istr >> hex >> offset)) {
						cout << "Error: Invalid argument '" << arg << "'" << endl;
						cout << "Usage: d $addr" << endl;
						cout << "Example: d 0xff00" << endl;
					}
				}
			
				uint8_t len;
				for (int x = 0; x < 16; x++) {
					len = dumpInstruction(offset);
					offset += len;
				}

				break;
			}

			case CMD_INCLUDE:
			{
				std::istringstream istr(arg);
				std::string filename;

				if (arg.size() > 0) {
					istr >> filename;
					ifstream file(filename.c_str(), ios::in);
					file.close();
					/*
					if (file.is_open()) {
						file.read((char *) data, APPLE2E_ROM_SIZE);
					*/
					
				} else {
					cout << "Error: Invalid argument '" << arg << "'" << endl;
					cout << "Usage: include <filename>" << endl;
				}

				cout << "Including file " << filename << endl;
				break;
			}

			case CMD_JUMP:
			{
				std::istringstream istr(arg);
				uint16_t offset;

				if (istr >> hex >> offset) {
					setPC(offset);
				} else {
					cout << "Error: Invalid argument '" << arg << "'" << endl;
					cout << "Usage: j <addr>" << endl;
				}

				cout << "Jumping to " << hex << offset << endl;
				break;
			}

			case CMD_KEY:
			{
				std::istringstream istr(arg);
				uint16_t key;

				if (istr >> hex >> key) {
					MemorySoftSwitch *switches = (MemorySoftSwitch *) memory->getRegion(REGION_SOFT_SWITCHES);
					switches->setKeyboardData(key & 0x00FF);
					switches->doKeyboardStrobe();

					cout << "Added key $" << hex << key << " to keyboard buffer." << endl;
				} else {
					cout << "Error: Invalid argument '" << arg << "'" << endl;
					cout << "Example usage: key 0x61" << endl;
				}
				break;
			}

			case CMD_LOAD:
			{
	      			std::istringstream istr(arg);
				std::string filename;

				if (arg.size() > 0) {
					istr >> filename;
					disk[0]->openFile(filename);
				} else {
					cout << "Error: Invalid argument '" << arg << "'" << endl;
					cout << "Example usage: load file.dsk" << endl;
				}
				break;
			}

			case CMD_DUMP:
			{
				std::istringstream istr(arg);
				uint16_t offset;

				if (istr >> hex >> offset) {
					dumpMemory(offset, 64);
				} else {
					cout << "Error: Invalid argument '" << arg << "'" << endl;
					cout << "Usage: p $addr" << endl;
					cout << "Example: p 0x1234" << endl;
				}

				break;
			}

			case CMD_SHOW_STACK:
			{
				dumpStack(8);
				break;
			}

			case CMD_SHOW_REGS:
			{
				dumpRegisters();
				break;
			}

			case CMD_QUIT:
			{
				return;
				break;
			}

			case CMD_REDRAW:
			{
				screen->redraw();
				break;
			}

			case CMD_RET:
			{
				// Very flawed: assumes SP == PC, but
				// it could be anything pushed on the
				// stack
				uint16_t offset = OFFSET_PAGE_1 | (registers.sp + 1);
				uint8_t low = memory->read(offset);
				uint8_t high = memory->read(offset + 1);
				
				offset = make16(high, low) + 1;
				
				printf("Executing until PC == %04X\n", offset);
				setPCBreakpoint(offset);
				run();
				
				break;
			}

			case CMD_RUN:
			{
				run();
				break;
			}

			case CMD_STEP:
			{
				unsigned int len = getInstructionLen(memory->read(getPC()));
				uint16_t next = getPC() + len;

				printf("Executing until PC == %04X\n", next);
				setPCBreakpoint(next);
				run();

				break;
			}

			case CMD_TRACE:
			{
				traceInstructions = ! traceInstructions;

				printf("Instruction tracing is now %s\n", traceInstructions ? "ON" : "OFF");
				break;
			}

			case CMD_WRITE:
			{
				std::istringstream istr(arg);
				uint16_t offset;
				uint8_t byte;
				uint16_t val;

				istr >> hex >> offset;
				istr >> hex >> val;
				byte = val;

				memory->write(offset, byte);

				printf("Writing $%02X to $%04X\n", byte, offset);
				break;
			}
			
			default:
			{
				cout << "Unknown command" << endl;
				break;
			}
		}
	}

}
