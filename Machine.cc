#include <iostream>
#include <fstream>
#include <assert.h>
#include "Machine.h"
#include "instr_table.h"

using namespace std;

Machine::Machine(unsigned int memorySize)
{
	this->memory = new MemoryBus(memorySize);
	this->cycles = 0;
}

bool
Machine::loadApple2eROM(string &filename)
{
	ifstream file(filename.c_str(), ios::in | ios::binary);
	uint8_t *data = new uint8_t[APPLE2E_ROM_SIZE];
        bool success = false;

	if (file.is_open()) {
		file.read((char *) data, APPLE2E_ROM_SIZE);

		// MemoryRegion *unknown1 = new MemoryRegion(0x0200, 0x02FF, &data[0x4100]);
		MemoryRegion *diskController = new MemoryRegion(0x0600, 0x06FF, &data[0x0600]);
		MemoryRegion *internal = new MemoryRegion(0xC100, 0xCFFF, &data[0x4100]);
		MemoryRegion *main = new MemoryRegion(0xD000, 0xFFFF, &data[0x5000]);

		this->memory->addRegion(diskController);
		this->memory->addRegion(internal);
		this->memory->addRegion(main);

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

/* Returns number of bytes fetched for this instruction */
unsigned int
Machine::dumpInstruction(uint16_t offset)
{
	uint8_t opcode;
	instruction_t *instr;
	char strbuf[1024];
	int bufsize = sizeof(strbuf) - 1;

	opcode = memory->read(offset);

        if (opcode >= INSTR_TABLE_LEN) {
                cerr << "Error: Opcode 0x" << hex << opcode << " is outside the instruction table" << endl;
                return(0);
        }

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
	
	printf("%s\n", strbuf);

	return(len);
}

/* Make a 16-bit value out of two 8-bit ones */
uint16_t make16(uint8_t low, uint8_t high)
{
	uint16_t offset = ((uint16_t) high << 8) + low;

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

void
Machine::executeNextInstruction(void)
{
	uint8_t opcode;
	instruction_t *instr;
	uint8_t operands[2];

	opcode = memory->read(registers.pc++);

	assert(opcode < INSTR_TABLE_LEN);

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
			uint8_t val = memory->read(operands[0]);
			do_asl_m(val);
			break;
		}

		case 0x07:
		case 0x08:
		{
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
			uint16_t offset = make16(operands[1], operands[0]) + registers.y;
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
			uint16_t offset = make16(operands[1], operands[0]) + registers.x;
			uint8_t val = memory->read(offset);
			do_ora(val);
			break;
		}

		case 0x1E:
		{
			uint16_t offset = make16(operands[1], operands[0]) + registers.x;
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
			do_bit(offset);
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
			uint16_t offset = make16(operands[1], operands[0]) + registers.y;
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
			uint16_t offset = make16(operands[1], operands[0]) + registers.x;
			uint8_t val = memory->read(offset);
			do_bit(val);
			break;
		}

		case 0x3C:
		{
			uint16_t offset = make16(operands[1], operands[0]) + registers.x;
			uint8_t val = memory->read(offset);
			do_and(val);
			break;
		}

		case 0x3D:
		{
			uint16_t offset = make16(operands[1], operands[0]) + registers.x;
			do_rol_m(offset);
			break;
		}

		case 0x3E:
		{
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
			break;
		}

		case 0x46:
		{
			break;
		}

		case 0x47:
		{
			break;
		}

		case 0x48:
		{
			break;
		}

		case 0x49:
		{
			break;
		}

		case 0x4A:
		{
			break;
		}

		case 0x4B:
		{
			break;
		}

		case 0x4C:
		{
			break;
		}

		case 0x4D:
		{
			break;
		}

		case 0x4E:
		{
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
			break;
		}

		case 0x51:
		{
			break;
		}

		case 0x52:
		{
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
			break;
		}

		case 0x56:
		{
			break;
		}

		case 0x57:
		{
			break;
		}

		case 0x58:
		{
			break;
		}

		case 0x59:
		{
			break;
		}

		case 0x5A:
		{
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
			break;
		}

		case 0x5E:
		{
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
			break;
		}

		case 0x61:
		{
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
			break;
		}

		case 0x65:
		{
			break;
		}

		case 0x66:
		{
			break;
		}

		case 0x67:
		{
			break;
		}

		case 0x68:
		{
			break;
		}

		case 0x69:
		{
			break;
		}

		case 0x6A:
		{
			break;
		}

		case 0x6B:
		{
			break;
		}

		case 0x6C:
		{
			break;
		}

		case 0x6D:
		{
			break;
		}

		case 0x6E:
		{
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
			break;
		}

		case 0x71:
		{
			break;
		}

		case 0x72:
		{
			break;
		}

		case 0x73:
		{
			break;
		}

		case 0x74:
		{
			break;
		}

		case 0x75:
		{
			break;
		}

		case 0x76:
		{
			break;
		}

		case 0x77:
		{
			break;
		}

		case 0x78:
		{
			break;
		}

		case 0x79:
		{
			break;
		}

		case 0x7A:
		{
			break;
		}

		case 0x7B:
		{
			break;
		}

		case 0x7C:
		{
			break;
		}

		case 0x7D:
		{
			break;
		}

		case 0x7E:
		{
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
			break;
		}

		case 0x81:
		{
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
			break;
		}

		case 0x85:
		{
			break;
		}

		case 0x86:
		{
			break;
		}

		case 0x87:
		{
			break;
		}

		case 0x88:
		{
			break;
		}

		case 0x89:
		{
			break;
		}

		case 0x8A:
		{
			break;
		}

		case 0x8B:
		{
			break;
		}

		case 0x8C:
		{
			break;
		}

		case 0x8D:
		{
			break;
		}

		case 0x8E:
		{
			break;
		}

		case 0x8F:
		{
			break;
		}

		case 0x90:
		{
			break;
		}

		case 0x91:
		{
			break;
		}

		case 0x92:
		{
			break;
		}

		case 0x93:
		{
			break;
		}

		case 0x94:
		{
			break;
		}

		case 0x95:
		{
			break;
		}

		case 0x96:
		{
			break;
		}

		case 0x97:
		{
			break;
		}

		case 0x98:
		{
			break;
		}

		case 0x99:
		{
			break;
		}

		case 0x9A:
		{
			break;
		}

		case 0x9B:
		{
			break;
		}

		case 0x9C:
		{
			break;
		}

		case 0x9D:
		{
			break;
		}

		case 0x9E:
		{
			break;
		}

		case 0x9F:
		{
			break;
		}

		case 0xA0:
		{
			break;
		}

		case 0xA1:
		{
			break;
		}

		case 0xA2:
		{
			break;
		}

		case 0xA3:
		{
			break;
		}

		case 0xA4:
		{
			break;
		}

		case 0xA5:
		{
			break;
		}

		case 0xA6:
		{
			break;
		}

		case 0xA7:
		{
			break;
		}

		case 0xA8:
		{
			break;
		}

		case 0xA9:
		{
			break;
		}

		case 0xAA:
		{
			break;
		}

		case 0xAB:
		{
			break;
		}

		case 0xAC:
		{
			break;
		}

		case 0xAD:
		{
			break;
		}

		case 0xAE:
		{
			break;
		}

		case 0xAF:
		{
			break;
		}

		case 0xB0:
		{
			break;
		}

		case 0xB1:
		{
			break;
		}

		case 0xB2:
		{
			break;
		}

		case 0xB3:
		{
			break;
		}

		case 0xB4:
		{
			break;
		}

		case 0xB5:
		{
			break;
		}

		case 0xB6:
		{
			break;
		}

		case 0xB7:
		{
			break;
		}

		case 0xB8:
		{
			break;
		}

		case 0xB9:
		{
			break;
		}

		case 0xBA:
		{
			break;
		}

		case 0xBB:
		{
			break;
		}

		case 0xBC:
		{
			break;
		}

		case 0xBD:
		{
			break;
		}

		case 0xBE:
		{
			break;
		}

		case 0xBF:
		{
			break;
		}

		case 0xC0:
		{
			break;
		}

		case 0xC1:
		{
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
			break;
		}

		case 0xC5:
		{
			break;
		}

		case 0xC6:
		{
			break;
		}

		case 0xC7:
		{
			break;
		}

		case 0xC8:
		{
			break;
		}

		case 0xC9:
		{
			break;
		}

		case 0xCA:
		{
			break;
		}

		case 0xCB:
		{
			break;
		}

		case 0xCC:
		{
			break;
		}

		case 0xCD:
		{
			break;
		}

		case 0xCE:
		{
			break;
		}

		case 0xCF:
		{
			break;
		}

		case 0xD0:
		{
			break;
		}

		case 0xD1:
		{
			break;
		}

		case 0xD2:
		{
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
			break;
		}

		case 0xD6:
		{
			break;
		}

		case 0xD7:
		{
			break;
		}

		case 0xD8:
		{
			break;
		}

		case 0xD9:
		{
			break;
		}

		case 0xDA:
		{
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
			break;
		}

		case 0xDE:
		{
			break;
		}

		case 0xDF:
		{
			break;
		}

		case 0xE0:
		{
			break;
		}

		case 0xE1:
		{
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
			break;
		}

		case 0xE5:
		{
			break;
		}

		case 0xE6:
		{
			break;
		}

		case 0xE7:
		{
			break;
		}

		case 0xE8:
		{
			break;
		}

		case 0xE9:
		{
			break;
		}

		case 0xEA:
		{
			break;
		}

		case 0xEB:
		{
			break;
		}

		case 0xEC:
		{
			break;
		}

		case 0xED:
		{
			break;
		}

		case 0xEE:
		{
			break;
		}

		case 0xEF:
		{
			break;
		}

		case 0xF0:
		{
			break;
		}

		case 0xF1:
		{
			break;
		}

		case 0xF2:
		{
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
			break;
		}

		case 0xF6:
		{
			break;
		}

		case 0xF7:
		{
			break;
		}

		case 0xF8:
		{
			break;
		}

		case 0xF9:
		{
			break;
		}

		case 0xFA:
		{
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
			break;
		}

		case 0xFE:
		{
			break;
		}

		case 0xFF:
		{
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

/* Returns the effective memory address of (zp_offset,X) */
uint16_t
Machine::get_indexed_indirect(uint8_t zp_offset)
{
	uint16_t offset = (registers.x + zp_offset) % 0xFF;
	
	return(offset);
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
	uint16_t offset = ((uint16_t) (high << 8)) + low;
	
	return(offset);
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
	registers.psw.f.c = ((registers.a & 0x80) > 0);

	registers.a = registers.a << 1;

	registers.psw.f.z = (registers.a == 0);
	registers.psw.f.n = ((registers.a & 0x80) > 0);
}

void
Machine::do_asl_m(uint16_t offset)
{
	uint8_t val = memory->read(offset);

	registers.psw.f.c = ((val & 0x80) > 0);

	val = val << 1;

	registers.psw.f.z = (registers.a == 0); // XXX: Not sure?
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
Machine::do_bit(uint8_t val)
{	
	val = val & registers.a;

	registers.psw.f.v = ((val & 0x40) > 0);
	registers.psw.f.n = ((val & 0x80) > 0);
	registers.psw.f.z = (val == 0);
}

void
Machine::do_bmi(int8_t rel)
{
	if (registers.psw.f.n)
		registers.pc += rel;
}

void
Machine::do_bpl(int8_t rel)
{	
	if (registers.psw.f.n == 0)
		registers.pc += rel;
}

void
Machine::do_brk(void)
{
	cout << "do_brk() not implemented yet" << endl;
}

void
Machine::do_clc(void)
{
	registers.psw.f.c = 0;
}

void
Machine::do_dea(void)
{
	registers.a--;
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
}

void
Machine::do_jsr(uint16_t offset)
{
	uint8_t low = get_low(registers.pc);
	uint8_t high = get_high(registers.pc);
	
	push_stack(low);
	push_stack(high);
	
	registers.pc = offset;
}

void
Machine::do_ora(uint8_t val)
{
	this->registers.a |= val;

	// XXX: flags N and Z are supposed to be set.. but how?
}

uint8_t
Machine::rotate_left(uint8_t val)
{
	uint8_t temp_carry = (val & 0x80);
	val = (val << 1) + registers.psw.f.c;
	registers.psw.f.c = (temp_carry != 0);

	return(val);
}

void
Machine::do_rol_a(void)
{
	registers.a = rotate_left(registers.a);
	
	registers.psw.f.z = (registers.a == 0);
}

void
Machine::do_rol_m(uint16_t offset)
{
	uint8_t val = memory->read(offset);

	val = rotate_left(val);

	// XXX: Not sure?
	registers.psw.f.z = (registers.a == 0);

	memory->write(offset, val);
}

void
Machine::do_rti(void)
{
	registers.psw.val = pop_stack();

	uint8_t low = pop_stack();
	uint8_t high = pop_stack();
	
	registers.pc = make16(low, high);
}

void
Machine::do_sec(void)
{
	registers.psw.f.c = 1;
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

uint8_t
Machine::pop_stack(void)
{
	registers.sp--;

	uint16_t offset = registers.sp;

	uint8_t val = memory->read(offset);

	return(val);
}

void
Machine::push_stack(uint8_t val)
{
	uint16_t offset = registers.sp;

	memory->write(offset, val);

	registers.sp--;
}
