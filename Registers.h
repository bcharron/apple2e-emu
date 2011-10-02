/*
 * Registers.h - <description>
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
 * Registers.h - Benjamin Charron <bcharron@pobox.com>
 * Created  : Thu Sep 29 13:04:31 2011
 * Revision : $Id$
 */

#ifndef _REGISTERS_H
#define _REGISTERS_H

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

typedef struct registers_s {
	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t sp;   // Stack pointer. Works in 0x0100 to 0x01FF (page 1)
	spc_flags_t psw;
	uint16_t pc;	
} registers_t;

#endif
