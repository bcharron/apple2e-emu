/*
 * Machine.h - <description>
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
 * Machine.h - Benjamin Charron <bcharron@pobox.com>
 * Created  : Mon Sep 12 23:02:01 2011
 * Revision : $Id$
 */

#include "MemoryBus.h"

#include <string>

#include "Screen.h"

#define APPLE2E_ROM_SIZE 32768
#define ROM_FILENAME "APPLE2E.ROM"
#define OFFSET_PAGE_1 0x0100
#define BOOTSTRAP_ADDRESS 0xFA62
#define MONITOR_START 0xFF69
#define CYCLE_TIME .00000097751710654936f     // Seconds per cycle

typedef union spc_flags_u {
        struct {
                unsigned int c : 1; // Carry
                unsigned int z : 1; // Zero
                unsigned int i : 1; // Interrupt Enable
                unsigned int d : 1; // Binary Coded Decimal
                unsigned int b : 1; // Break
                unsigned int p : 1; // Reserved
                unsigned int v : 1; // Overflow
                unsigned int n : 1; // Negative
        } f;
        uint8_t val;
} spc_flags_t;

struct registers_s {
	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t sp;   // Stack pointer. Works in 0x0100 to 0x01FF (page 1)
	spc_flags_t psw;
	uint16_t pc;	
};

class Machine
{
public:
	Machine();
	bool init(void);
	bool loadApple2eROM(std::string &filename);
	unsigned int dumpInstruction(uint16_t offset);
	void dumpFlags(spc_flags_t *flags, char *buf);
	void dumpMemory(uint16_t offset, uint16_t len);
	void dumpRegisters(void);
	void executeNextInstruction(void);
	void setPC(uint16_t pc);
	uint16_t getPC(void);
	bool testCPU(void);
	std::string* getSubroutineHandle(uint16_t offset);
	void dumpStack(uint16_t len);
	void interactive(void);
	void run(void);
	void setPCBreakpoint(uint16_t offset);

	MemoryBus *memory;

protected:
	void do_adc(uint8_t val);
	void do_and(uint8_t val);
	void do_asl_a(void);
	void do_asl_m(uint16_t offset);
	void do_bbr(uint8_t bit, uint8_t val, int8_t rel);
	void do_bbs(uint8_t bit, uint8_t val, int8_t rel);
	void do_bcc(int8_t rel);
	void do_bcs(int8_t rel);
	void do_beq(int8_t rel);
	void do_bit(uint8_t val);
	void do_bpl(int8_t rel);
	void do_bmi(int8_t rel);
	void do_bne(int8_t rel);
	void do_bra(int8_t rel);
	void do_brk(void);
	void do_bvc(int8_t rel);
	void do_bvs(int8_t rel);
	void do_clc(void);
	void do_cld(void);
	void do_cli(void);
	void do_clv(void);
	void do_cmp(uint8_t val);
	void do_cpx(uint8_t val);
	void do_cpy(uint8_t val);
	void do_dea(void);
	void do_dec(uint16_t offset);
	void do_dex(void);
	void do_dey(void);
	void do_eor(uint8_t val);
	void do_ina(void);
	void do_inc(uint16_t offset);
	void do_inx(void);
	void do_iny(void);
	void do_jmp(uint16_t offset);
	void do_jsr(uint16_t offset);
	void do_lda(uint8_t val);
	void do_ldx(uint8_t val);
	void do_ldy(uint8_t val);
	void do_lsr_a(void);
	void do_lsr_m(uint16_t offset);
	void do_nop(void);
	void do_ora(uint8_t val);
	void do_pha(void);
	void do_php(void);
	void do_phx(void);
	void do_phy(void);
	void do_pla(void);
	void do_plp(void);
	void do_plx(void);
	void do_ply(void);
	void do_rol_a(void);
	void do_rol_m(uint16_t offset);
	void do_ror_a(void);
	void do_ror_m(uint16_t offset);
	void do_rti(void);
	void do_rts(void);
	void do_sbc(uint8_t val);
	void do_sec(void);
	void do_sed(void);
	void do_sei(void);
	void do_sta(uint16_t offset);
	void do_stx(uint16_t offset);
	void do_sty(uint16_t offset);
	void do_stz(uint16_t offset);
	void do_tay(void);
	void do_tax(void);
	void do_trb(uint16_t offset);
	void do_tsb(uint16_t offset);
	void do_tsx(void);
	void do_txa(void);
	void do_txs(void);
	void do_tya(void);

	uint8_t rotate_left(uint8_t val);
	uint8_t rotate_right(uint8_t val);
	uint8_t shift_right(uint8_t val);
	uint8_t pop_stack(void);
	void push_stack(uint8_t val);
	void compare(uint8_t reg, uint8_t val);

	uint16_t get_indexed_indirect(uint8_t zp_offset);
	uint16_t get_indirect_indexed(uint8_t zp_offset);
	uint16_t get_indirect_zeropage(uint8_t zp_offset);
	uint16_t get_absolute_x(uint8_t operand0, uint8_t operand1);
	uint16_t get_absolute_y(uint8_t operand0, uint8_t operand1);

	struct registers_s registers;
	unsigned long int cycles;
	Screen *screen;
	bool pcBreakpointEnabled;
	uint16_t pcBreakpointOffset;

	bool traceInstructions;
};

