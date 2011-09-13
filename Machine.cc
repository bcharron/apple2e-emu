#include <iostream>
#include <fstream>
#include "Machine.h"
#include "instr_table.h"

using namespace std;

Machine::Machine(unsigned int memorySize)
{
	memory = new MemoryBus(memorySize);
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
}

void
Machine::setPC(uint16_t pc)
{
	this->pc = pc;
}

uint16_t
Machine::getPC(void)
{
	return(this->pc);
}

/* Returns number of bytes fetched for this instruction */
unsigned int
Machine::dumpInstruction(uint16_t offset)
{
	uint8_t opcode;
	instruction_t *instr;
	unsigned int instr_size;
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
	
	for (x = 0; x < 4; x++) {
		if (x < instr->len)
			printf("%02X ", memory->read(offset + x));
		else
			printf("   ");
	}
	
	printf("%s\n", strbuf);

	return(len);
}

void
Machine::executeNextInstruction(void)
{
	uint8_t opcode;

	opcode = memory->read(pc++);
}
