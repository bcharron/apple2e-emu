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
			uint16_t offset = memory->read(operands[0]) + registers.x;
			uint8_t val = memory->read(offset);
			do_eor(val);
			break;
		}

		case 0x56:
		{
			uint16_t offset = memory->read(operands[0]) + registers.x;
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
			uint16_t offset = get_absolute_y(operands[0], operands[1]);
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
			// offset = xxFF, than xxFF and xx00 are
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
			do_ldy(operands[0]);
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
			do_cmp(val);
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
Machine::get_absolute_x(uint8_t operand0, uint8_t operand1)
{
	// How is wrapping handled?
	uint16_t offset = make16(operand1, operand0) + registers.x;

	return(offset);
}

uint16_t
Machine::get_absolute_y(uint8_t operand0, uint8_t operand1)
{
	// How is wrapping handled?
	uint16_t offset = make16(operand1, operand0) + registers.y;

	return(offset);
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
Machine::do_adc(uint8_t val)
{
	uint16_t result;
	int overflow = 0;

	result = registers.a + val + registers.psw.f.c;

	if (result > 0xff) {
		overflow = 1;
		result = result % 0x00ff;
	}

	registers.a = result;

	registers.psw.f.z = (registers.a == 0);
	registers.psw.f.n = ((registers.a & 0x80) != 0);
	registers.psw.f.c = overflow;
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

	registers.psw.f.z = (registers.a == 0); // XXX: Not sure? Should probably be (val == 0)
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
	
	push_stack(low);
	push_stack(high);

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
	registers.psw.f.z = (reg == val);
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
}

void
Machine::do_dey(void)
{
	registers.y--;
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
	uint8_t low = get_low(registers.pc);
	uint8_t high = get_high(registers.pc);
	
	push_stack(low);
	push_stack(high);
	
	registers.pc = offset;
}

void
Machine::do_lda(uint8_t val)
{
	registers.a = val;
}

void
Machine::do_ldx(uint8_t val)
{
	registers.x = val;
}

void
Machine::do_ldy(uint8_t val)
{
	registers.y = val;
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
	registers.psw.f.n = ((val & 0x80) != 0);

	return(val);
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

	return(val);
}

void
Machine::do_ror_a(void)
{
	registers.a = rotate_right(registers.a);
	
	registers.psw.f.z = (registers.a == 0);
}

void
Machine::do_ror_m(uint16_t offset)
{
	uint8_t val = memory->read(offset);

	val = rotate_right(val);

	// XXX: Not sure? Should probably be (val == 0)
	registers.psw.f.z = (registers.a == 0);

	memory->write(offset, val);
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

	// XXX: Not sure? Should probably be 'val == 0'
	registers.psw.f.z = (registers.a == 0);

	memory->write(offset, val);
}

uint8_t
Machine::shift_right(uint8_t val)
{
	registers.psw.f.c = val & 0x01;

	val = (val >> 1);

	return(val);
}

void
Machine::do_lsr_a(void)
{
	registers.a = shift_right(registers.a);
	
	registers.psw.f.z = (registers.a == 0);
	
	// XXX: This make no sense.
	// registers.psw.f.n
}

void
Machine::do_lsr_m(uint16_t offset)
{
	uint8_t val = memory->read(offset);

	val = shift_right(val);

	registers.psw.f.z = (val == 0);

	memory->write(offset, val);
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
}

void
Machine::do_plp(void)
{
	registers.psw.val = pop_stack();
}

void
Machine::do_plx(void)
{
	registers.x = pop_stack();
}

void
Machine::do_ply(void)
{
	registers.y = pop_stack();
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
	
	registers.pc = make16(high, low);
}

void
Machine::do_sbc(uint8_t val)
{
	int temp = registers.a - val - (1 - registers.psw.f.c);
	registers.a = registers.a - val - (1 - registers.psw.f.c);

	registers.psw.f.c = ! ((registers.a & 0x80) != 0);
	registers.psw.f.z = (registers.a == 0);
	registers.psw.f.v = (temp > 127 || temp < -128);
}

void
Machine::do_sec(void)
{
	registers.psw.f.c = 1;
}

void
Machine::do_sed(void)
{
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
Machine::do_tay(void)
{
	registers.y = registers.a;
}

void
Machine::do_tax(void)
{
	registers.x = registers.a;
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
}

void
Machine::do_txa(void)
{
	registers.a = registers.x;
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
	uint16_t offset = OFFSET_PAGE_1 + registers.sp;

	memory->write(offset, val);

	registers.sp--;
}
