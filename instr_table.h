/*
 * instr_table.h - <description>
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
 * instr_table.h - Benjamin Charron <bcharron@pobox.com>
 * Created  : Mon Sep 12 23:51:43 2011
 * Revision : $Id$
 */

struct instruction_s {
	const char *str;
	unsigned int len;      // Number of bytes in instruction
	unsigned int cycles;   // Number of cycles for this operation
};

typedef struct instruction_s instruction_t;

#define INSTR_TABLE_LEN sizeof(instr_table) / sizeof(instruction_t)

struct instruction_s instr_table[] =
{
	{ "BRK",               1, 7 }, // 0x00
	{ "ORA ($%02X,X)",     2, 6 }, // 0x01
	{ "???",               1, 1 }, // 0x02
	{ "???",               1, 1 }, // 0x03
	{ "TSB $%02X",         2, 5 }, // 0x04
	{ "ORA $%02X",         2, 3 }, // 0x05
	{ "ASL $%02X",         2, 5 }, // 0x06
	{ "???",               1, 1 }, // 0x07
	{ "???",               1, 1 }, // 0x08
	{ "ORA #$%02X",        2, 2 }, // 0x09
	{ "ASL A",             1, 2 }, // 0x0A
	{ "???",               1, 1 }, // 0x0B
	{ "TSB $%02X%02X",     3, 6 }, // 0x0C
	{ "ORA $%02X%02X",     3, 4 }, // 0x0D
	{ "ASL $%02X%02X",     3, 6 }, // 0x0E
	{ "BBR0 $%02X",        2, 2 }, // 0x0F
	{ "BPL $%02X",         2, 2 }, // 0x10
	{ "ORA ($%02X),Y",     2, 5 }, // 0x11
	{ "ORA ($%02X)",       2, 5 }, // 0x12
	{ "???",               1, 1 }, // 0x13
	{ "TRB $%02X",         2, 5 }, // 0x14
	{ "ORA $%02X,X",       2, 4 }, // 0x15
	{ "ASL $%02X,X",       2, 6 }, // 0x16
	{ "???",               1, 1 }, // 0x17
	{ "CLC",               1, 2 }, // 0x18
	{ "ORA $%02X%02X,Y",   3, 4 }, // 0x19
	{ "INA",               1, 2 }, // 0x1A
	{ "???",               1, 1 }, // 0x1B
	{ "TRB $%02X%02X",     3, 6 }, // 0x1C
	{ "ORA $%02X%02X,X",   3, 4 }, // 0x1D
	{ "ASL $%02X%02X,X",   3, 7 }, // 0x1E
	{ "BBR1 $%02X",        2, 2 }, // 0x1F
	{ "JSR $%02X%02X",     3, 6 }, // 0x20
	{ "AND ($%02X,X)",     2, 6 }, // 0x21
	{ "???",               1, 1 }, // 0x22
	{ "???",               1, 1 }, // 0x23
	{ "BIT $%02X",         2, 3 }, // 0x24
	{ "AND $%02X",         2, 3 }, // 0x25
	{ "ROL $%02X",         2, 5 }, // 0x26
	{ "???",               1, 1 }, // 0x27
	{ "???",               1, 1 }, // 0x28
	{ "AND #$%02X",        2, 2 }, // 0x29
	{ "ROL A",             1, 2 }, // 0x2A
	{ "???",               1, 1 }, // 0x2B
	{ "BIT $%02X%02X",     3, 4 }, // 0x2C
	{ "AND $%02X%02X",     3, 4 }, // 0x2D
	{ "ROL $%02X%02X",     3, 6 }, // 0x2E
	{ "BBR2 $%02X",        2, 2 }, // 0x2F
	{ "BMI $%02X",         2, 2 }, // 0x30
	{ "AND ($%02X),Y",     2, 5 }, // 0x31
	{ "AND ($%02X)",       2, 5 }, // 0x32
	{ "???",               1, 1 }, // 0x33
	{ "BIT $%02X,X",       2, 4 }, // 0x34
	{ "AND $%02X,X",       2, 4 }, // 0x35
	{ "ROL $%02X,X",       2, 6 }, // 0x36
	{ "???",               1, 1 }, // 0x37
	{ "SEC",               1, 2 }, // 0x38
	{ "AND $%02X%02X,Y",   3, 4 }, // 0x39
	{ "DEA",               1, 2 }, // 0x3A
	{ "???",               1, 1 }, // 0x3B
	{ "BIT $%02X%02X,X",   3, 4 }, // 0x3C
	{ "AND $%02X%02X,X",   3, 4 }, // 0x3D
	{ "ROL $%02X%02X,X",   3, 7 }, // 0x3E
	{ "BBR3 $%02X",        2, 2 }, // 0x3F
	{ "RTI",               1, 6 }, // 0x40
	{ "EOR ($%02X,X)",     2, 6 }, // 0x41
	{ "???",               1, 1 }, // 0x42
	{ "???",               1, 1 }, // 0x43
	{ "???",               1, 1 }, // 0x44
	{ "EOR $%02X",         2, 3 }, // 0x45
	{ "LSR $%02X",         2, 5 }, // 0x46
	{ "???",               1, 1 }, // 0x47
	{ "PHA",               1, 3 }, // 0x48
	{ "EOR #$%02X",        2, 2 }, // 0x49
	{ "LSR A",             1, 2 }, // 0x4A
	{ "???",               1, 1 }, // 0x4B
	{ "JMP $%02X%02X",     3, 3 }, // 0x4C
	{ "EOR $%02X%02X",     3, 4 }, // 0x4D
	{ "LSR $%02X%02X",     3, 6 }, // 0x4E
	{ "BBR4 $%02X",        2, 2 }, // 0x4F
	{ "BVC $%02X",         2, 2 }, // 0x50
	{ "EOR ($%02X),Y",     2, 5 }, // 0x51
	{ "EOR ($%02X)",       2, 5 }, // 0x52
	{ "???",               1, 1 }, // 0x53
	{ "???",               1, 1 }, // 0x54
	{ "EOR $%02X,X",       2, 4 }, // 0x55
	{ "LSR $%02X,X",       2, 6 }, // 0x56
	{ "???",               1, 1 }, // 0x57
	{ "CLI",               1, 2 }, // 0x58
	{ "EOR $%02X%02X,Y",   3, 4 }, // 0x59
	{ "PHY",               1, 3 }, // 0x5A
	{ "???",               1, 1 }, // 0x5B
	{ "???",               1, 1 }, // 0x5C
	{ "EOR $%02X%02X,X",   3, 4 }, // 0x5D
	{ "LSR $%02X%02X,X",   3, 7 }, // 0x5E
	{ "BBR5 $%02X",        2, 2 }, // 0x5F
	{ "RTS",               1, 6 }, // 0x60
	{ "ADC ($%02X,X)",     2, 6 }, // 0x61
	{ "???",               1, 1 }, // 0x62
	{ "???",               1, 1 }, // 0x63
	{ "STZ $%02X",         2, 3 }, // 0x64
	{ "ADC $%02X",         2, 3 }, // 0x65
	{ "ROR $%02X",         2, 5 }, // 0x66
	{ "???",               1, 1 }, // 0x67
	{ "PLA",               1, 4 }, // 0x68
	{ "ADC #$%02X",        2, 2 }, // 0x69
	{ "ROR A",             1, 2 }, // 0x6A
	{ "???",               1, 1 }, // 0x6B
	{ "JMP ($%02X%02X)",   3, 5 }, // 0x6C
	{ "ADC $%02X%02X",     3, 4 }, // 0x6D
	{ "ROR $%02X%02X",     3, 6 }, // 0x6E
	{ "BBR6 $%02X",        2, 2 }, // 0x6F
	{ "BVS $%02X",         2, 2 }, // 0x70
	{ "ADC ($%02X),Y",     2, 5 }, // 0x71
	{ "ADC ($%02X)",       2, 5 }, // 0x72
	{ "???",               1, 1 }, // 0x73
	{ "STZ $%02X,X",       2, 4 }, // 0x74
	{ "ADC $%02X,X",       2, 4 }, // 0x75
	{ "ROR $%02X,X",       2, 6 }, // 0x76
	{ "???",               1, 1 }, // 0x77
	{ "SEI",               1, 2 }, // 0x78
	{ "ADC $%02X%02X,Y",   3, 4 }, // 0x79
	{ "PLY",               1, 4 }, // 0x7A
	{ "???",               1, 1 }, // 0x7B
	{ "JMP ($%02X%02X,X)", 3, 6 }, // 0x7C
	{ "ADC $%02X%02X,X",   3, 4 }, // 0x7D
	{ "ROR $%02X%02X,X",   3, 7 }, // 0x7E
	{ "BBR7 $%02X",        2, 2 }, // 0x7F
	{ "BRA $%02X",         2, 3 }, // 0x80
	{ "STA ($%02X,X)",     2, 6 }, // 0x81
	{ "???",               1, 1 }, // 0x82
	{ "???",               1, 1 }, // 0x83
	{ "STY $%02X",         2, 3 }, // 0x84
	{ "STA $%02X",         2, 3 }, // 0x85
	{ "STX $%02X",         2, 3 }, // 0x86
	{ "???",               1, 1 }, // 0x87
	{ "DEY",               1, 2 }, // 0x88
	{ "BIT #$%02X",        2, 2 }, // 0x89
	{ "TXA",               1, 2 }, // 0x8A
	{ "???",               1, 1 }, // 0x8B
	{ "STY $%02X%02X",     3, 4 }, // 0x8C
	{ "STA $%02X%02X",     3, 4 }, // 0x8D
	{ "STX $%02X%02X",     3, 4 }, // 0x8E
	{ "BBS0 $%02X",        2, 2 }, // 0x8F
	{ "BCC $%02X",         2, 2 }, // 0x90
	{ "STA ($%02X),Y",     2, 6 }, // 0x91
	{ "STA ($%02X)",       2, 5 }, // 0x92
	{ "???",               1, 1 }, // 0x93
	{ "STY $%02X,X",       2, 4 }, // 0x94
	{ "STA $%02X,X",       2, 4 }, // 0x95
	{ "STX $%02X,Y",       2, 4 }, // 0x96
	{ "???",               1, 1 }, // 0x97
	{ "TYA",               1, 2 }, // 0x98
	{ "STA $%02X%02X,Y",   3, 5 }, // 0x99
	{ "TXS",               1, 2 }, // 0x9A
	{ "???",               1, 1 }, // 0x9B
	{ "STZ $%02X%02X",     3, 4 }, // 0x9C
	{ "STA $%02X%02X,X",   3, 5 }, // 0x9D
	{ "STZ $%02X%02X,X",   3, 5 }, // 0x9E
	{ "BBS1 $%02X",        2, 2 }, // 0x9F
	{ "LDY #$%02X",        2, 2 }, // 0xA0
	{ "LDA ($%02X,X)",     2, 6 }, // 0xA1
	{ "LDX #$%02X",        2, 2 }, // 0xA2
	{ "???",               1, 1 }, // 0xA3
	{ "LDY $%02X",         2, 3 }, // 0xA4
	{ "LDA $%02X",         2, 3 }, // 0xA5
	{ "LDX $%02X",         2, 3 }, // 0xA6
	{ "???",               1, 1 }, // 0xA7
	{ "TAY",               1, 2 }, // 0xA8
	{ "LDA #$%02X",        2, 2 }, // 0xA9
	{ "TAX",               1, 2 }, // 0xAA
	{ "???",               1, 1 }, // 0xAB
	{ "LDY $%02X%02X",     3, 4 }, // 0xAC
	{ "LDA $%02X%02X",     3, 4 }, // 0xAD
	{ "LDX $%02X%02X",     3, 4 }, // 0xAE
	{ "BBS2 $%02X",        2, 2 }, // 0xAF
	{ "BCS $%02X",         2, 2 }, // 0xB0
	{ "LDA ($%02X),Y",     2, 5 }, // 0xB1
	{ "LDA ($%02X)",       2, 5 }, // 0xB2
	{ "???",               1, 1 }, // 0xB3
	{ "LDY $%02X,X",       2, 4 }, // 0xB4
	{ "LDA $%02X,X",       2, 4 }, // 0xB5
	{ "LDX $%02X,Y",       2, 4 }, // 0xB6
	{ "???",               1, 1 }, // 0xB7
	{ "CLV",               1, 2 }, // 0xB8
	{ "LDA $%02X%02X,Y",   3, 4 }, // 0xB9
	{ "TSX",               1, 2 }, // 0xBA
	{ "???",               1, 1 }, // 0xBB
	{ "LDY $%02X%02X,X",   3, 4 }, // 0xBC
	{ "LDA $%02X%02X,X",   3, 4 }, // 0xBD
	{ "LDX $%02X%02X,Y",   3, 4 }, // 0xBE
	{ "BBS3 $%02X",        2, 2 }, // 0xBF
	{ "CPY #$%02X",        2, 2 }, // 0xC0
	{ "CMP ($%02X,X)",     2, 6 }, // 0xC1
	{ "???",               1, 1 }, // 0xC2
	{ "???",               1, 1 }, // 0xC3
	{ "CPY $%02X",         2, 3 }, // 0xC4
	{ "CMP $%02X",         2, 3 }, // 0xC5
	{ "DEC $%02X",         2, 5 }, // 0xC6
	{ "???",               1, 1 }, // 0xC7
	{ "INY",               1, 2 }, // 0xC8
	{ "CMP #$%02X",        2, 2 }, // 0xC9
	{ "DEX",               1, 2 }, // 0xCA
	{ "???",               1, 1 }, // 0xCB
	{ "CPY $%02X%02X",     3, 4 }, // 0xCC
	{ "CMP $%02X%02X",     3, 4 }, // 0xCD
	{ "DEC $%02X%02X",     3, 6 }, // 0xCE
	{ "BBS4 $%02X",        2, 2 }, // 0xCF
	{ "BNE $%02X",         2, 2 }, // 0xD0
	{ "CMP ($%02X),Y",     2, 5 }, // 0xD1
	{ "CMP ($%02X)",       2, 5 }, // 0xD2
	{ "???",               1, 1 }, // 0xD3
	{ "???",               1, 1 }, // 0xD4
	{ "CMP $%02X",         2, 4 }, // 0xD5
	{ "DEC $%02X,X",       2, 6 }, // 0xD6
	{ "???",               1, 1 }, // 0xD7
	{ "CLD",               1, 2 }, // 0xD8
	{ "CMP $%02X%02X,Y",   3, 4 }, // 0xD9
	{ "PHX",               1, 3 }, // 0xDA
	{ "???",               1, 1 }, // 0xDB
	{ "???",               1, 1 }, // 0xDC
	{ "CMP $%02X%02X,X",   3, 4 }, // 0xDD
	{ "DEC $%02X%02X,X",   3, 7 }, // 0xDE
	{ "BBS5 $%02X",        2, 2 }, // 0xDF
	{ "CPX #$%02X",        2, 2 }, // 0xE0
	{ "SBC ($%02X,X)",     2, 6 }, // 0xE1
	{ "???",               1, 1 }, // 0xE2
	{ "???",               1, 1 }, // 0xE3
	{ "CPX $%02X",         2, 3 }, // 0xE4
	{ "SBC $%02X",         2, 3 }, // 0xE5
	{ "INC $%02X",         2, 5 }, // 0xE6
	{ "???",               1, 1 }, // 0xE7
	{ "INX",               1, 2 }, // 0xE8
	{ "SBC #$%02X",        2, 2 }, // 0xE9
	{ "NOP",               1, 2 }, // 0xEA
	{ "???",               1, 1 }, // 0xEB
	{ "CPX $%02X%02X",     3, 4 }, // 0xEC
	{ "SBC $%02X%02X",     3, 4 }, // 0xED
	{ "INC $%02X%02X",     3, 6 }, // 0xEE
	{ "BBS6 $%02X",        2, 2 }, // 0xEF
	{ "BEQ $%02X",         2, 2 }, // 0xF0
	{ "SBC ($%02X),Y",     2, 5 }, // 0xF1
	{ "SBC ($%02X)",       2, 5 }, // 0xF2
	{ "???",               1, 1 }, // 0xF3
	{ "???",               1, 1 }, // 0xF4
	{ "SBC $%02X,X",       2, 4 }, // 0xF5
	{ "INC $%02X,X",       2, 6 }, // 0xF6
	{ "???",               1, 1 }, // 0xF7
	{ "SED",               1, 2 }, // 0xF8
	{ "SBC $%02X%02X,Y",   3, 4 }, // 0xF9
	{ "PLX",               1, 4 }, // 0xFA
	{ "???",               1, 1 }, // 0xFB
	{ "???",               1, 1 }, // 0xFC
	{ "SBC $%02X%02X,X",   3, 4 }, // 0xFD
	{ "INC $%02X%02X,X",   3, 7 }, // 0xFE
	{ "BBS7 $%02X",        2, 2 }, // 0xFF
};
