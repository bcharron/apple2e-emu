/*
 * Screen.cc - <description>
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
 * Screen.cc - Benjamin Charron <bcharron@pobox.com>
 * Created  : Thu Sep 22 23:42:32 2011
 * Revision : $Id$
 */

#include "Screen.h"

#include <SDL/SDL.h>

Screen::Screen(MemoryRegion *mainRegion, MemoryRegion *auxRegion, MemorySoftSwitch *switches)
	: mainRegion(mainRegion),
	  auxRegion(auxRegion),
	  switches(switches),
	  sdl_screen(NULL)
{
}

bool
Screen::init(void)
{
	if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr,	"Couldn't initialize SDL Video: %s\n", SDL_GetError());
		return(false);
	}

	atexit(SDL_Quit);

	sdl_screen = SDL_SetVideoMode(640, 480, 8, SDL_SWSURFACE);
	if ( sdl_screen == NULL ) {
		fprintf(stderr, "Couldn't set 640x480x8 video mode: %s\n", SDL_GetError());
		return(false);
	}

	return(true);
}

void
Screen::redraw(void)
{
	if (switches->isTextMode()) {
		redrawText();
	} else {
		redrawGraphics();
	}
}

void
Screen::redrawText(void)
{
	uint16_t ptr;

	if (switches->is80Col()) {
		ptr = 0x400;
	} else {
		ptr = 0x400;

		for (int y = 0; y < 24; y++) {
			for (int x = 0; x < 40; x++) {
				printf("%c", mainRegion->read(ptr++) & 0x3F);
			}
			printf("\n");
		}
	}
}

void
Screen::redrawGraphics(void)
{
}
