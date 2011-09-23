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

#include "MemoryRegion.h"
#include "MemorySoftSwitch.h"

class Screen
{
public:
	Screen(MemoryRegion *mainRegion, MemoryRegion *auxRegion, MemorySoftSwitch *switches);
	bool init(void);
	void redraw(void);

private:
	void redrawText(void);
	void redrawGraphics(void);

	MemoryRegion *mainRegion;
	MemoryRegion *auxRegion;
	MemorySoftSwitch *switches;
	SDL_Surface *sdl_screen;
};

#endif
