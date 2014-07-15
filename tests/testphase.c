/*
 * testphase.c - <description>
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
 * testphase.c - Benjamin Charron <bcharron@pobox.com>
 * Created  : Fri Dec 23 09:35:41 2011
 * Revision : $Id$
 */

#include <stdio.h>

int main (int argc, char *argv[]) {
	unsigned char y = 80;
	unsigned char a;

	a = 0;

	for (y = 80; y > 0; y--) {
		printf("LDA %04X\n", 0xC080 + a);
		a = y;
		a = a & 3;
		a = a << 1;
		printf("LDA %04X\n", 0xC081 + a);
	}

	return (0);
}
