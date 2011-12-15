/*
 * Screen.h - <description>
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
 * Screen.h - Benjamin Charron <bcharron@pobox.com>
 * Created  : Thu Sep 22 23:38:54 2011
 * Revision : $Id$
 */

#ifndef _SCREEN_H
#define _SCREEN_H

#include <SDL/SDL.h>

#include <string>

#include "MemoryRegion.h"
#include "MemorySoftSwitch.h"

#define SCREEN_FONT_SIZE 4096
#define CHARACTER_WIDTH  7
#define CHARACTER_HEIGHT 8
#define CHARACTER_COLS   40
#define CHARACTER_ROWS   24
#define CHARACTER_LINE_SIZE 0x80

class Screen
{
public:
	Screen(unsigned int width, unsigned int height, MemoryRegion *mainRegion, MemoryRegion *auxRegion, MemorySoftSwitch *switches);
	bool init(void);
	void redraw(void);
	void drawCharacter(int x, int y, int charIndex);
	void drawBlock(int x, int y, int size_x, int size_y, unsigned char color);
	bool loadFont(std::string filename);
	bool setZoom(unsigned int zoomFactor);
	unsigned int getZoom(void);

private:
	void redrawText();
	void redrawGraphics();
	void redrawGraphicsHires(void);
	void redrawGraphicsLowres(void);
	void putZoomPixel(unsigned int x, unsigned int y, Uint32 color);
	void sdl_putpixel(unsigned int x, unsigned int y, Uint32 color);

	unsigned int width;
	unsigned int height;
	unsigned int zoomFactor;

	MemoryRegion *mainRegion;
	MemoryRegion *auxRegion;
	MemorySoftSwitch *switches;
	SDL_Surface *sdl_screen;
	uint8_t *fontBuffer;
	Uint32 colors[16];
};

#endif
