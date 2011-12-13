/*
 * Screen.cc - Part of Apple2-Emu, represents the display
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

#include <assert.h>
#include <SDL/SDL.h>

/*
 *  XXX: I think the best way to handle switch changes
 *  (text/gfx1/gfx2) would be a simple Observer.
 */

Screen::Screen(unsigned int width, unsigned int height, MemoryRegion *mainRegion, 
	       MemoryRegion *auxRegion, MemorySoftSwitch *switches)
	: width(width),
	  height(height),
	  zoomFactor(1),
	  mainRegion(mainRegion),
	  auxRegion(auxRegion),
	  switches(switches),
	  sdl_screen(NULL)
{
}

/*
 *  Change the pixel size, effectively 'zooming' it
 */
bool
Screen::setZoom(unsigned int zoomFactor)
{
	SDL_Surface *newSurface;

	this->zoomFactor = zoomFactor;

	height = CHARACTER_HEIGHT * CHARACTER_ROWS * zoomFactor;
	width = CHARACTER_WIDTH * CHARACTER_COLS * zoomFactor;

	newSurface = SDL_SetVideoMode(width, height, 8, SDL_SWSURFACE);
	if ( newSurface == NULL ) {
		// XXX: Is the old surface still valid though???
		fprintf(stderr, "Couldn't set %dx%dx8 video mode: %s\n", width, height, SDL_GetError());
		return(false);
	}

	sdl_screen = newSurface;

	return(true);
}

unsigned int
Screen::getZoom(void)
{
	return(this->zoomFactor);
}

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 * (Code taken from SDL examples)
 */
void
Screen::sdl_putpixel(unsigned int x, unsigned int y, Uint32 pixel)
{
	SDL_Surface *surface = sdl_screen;

	int bpp = surface->format->BytesPerPixel;

	if (x >= width || y >= height) {
		printf("ERROR: Trying to draw past the screen! (%d,%d)\n", x, y);
		return;
	}

	/* Here p is the address to the pixel we want to set */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
		case 1:
			*p = pixel;
			break;

		case 2:
			*(Uint16 *)p = pixel;
			break;

		case 3:
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
				p[0] = (pixel >> 16) & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = pixel & 0xff;
			} else {
				p[0] = pixel & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = (pixel >> 16) & 0xff;
			}
			break;

		case 4:
			*(Uint32 *)p = pixel;
			break;
	}
}

/*
 *  Draw a pixel on the screen, taking into account the zoom factor
 */
void
Screen::putZoomPixel(unsigned int x, unsigned int y, Uint32 color)
{
	unsigned int zx, zy;

	for (zx = 0; zx < zoomFactor; zx++) {
		for (zy = 0; zy < zoomFactor; zy++) {
			sdl_putpixel(x * zoomFactor + zx, y * zoomFactor + zy, color);
		}
	}
}

bool
Screen::init(void)
{
	if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr,	"Couldn't initialize SDL Video: %s\n", SDL_GetError());
		return(false);
	}

	atexit(SDL_Quit);

	sdl_screen = SDL_SetVideoMode(width, height, 8, SDL_SWSURFACE);
	if ( sdl_screen == NULL ) {
		fprintf(stderr, "Couldn't set %dx%dx8 video mode: %s\n", width, height, SDL_GetError());
		return(false);
	}

	SDL_WM_SetCaption("Apple //e Emulator", "icon");

	fontBuffer = new uint8_t[SCREEN_FONT_SIZE];

	std::string videoROM = std::string("Apple IIe Video ROM US.bin");
	if (! loadFont(videoROM)) {
		fprintf(stderr, "Couldn't open video ROM %s\n", videoROM.c_str());
		return(false);
	}

	colors[0]  = SDL_MapRGB(sdl_screen->format, 0x00, 0x00, 0x00); // Black
	colors[1]  = SDL_MapRGB(sdl_screen->format, 0xFF, 0x00, 0xFF); // Magenta
	colors[2]  = SDL_MapRGB(sdl_screen->format, 0x00, 0x00, 0x55); // Dark blue
	colors[3]  = SDL_MapRGB(sdl_screen->format, 0xFF, 0x00, 0xFF); // Purple
	colors[4]  = SDL_MapRGB(sdl_screen->format, 0xFF, 0x00, 0xFF); // Dark green
	colors[5]  = SDL_MapRGB(sdl_screen->format, 0x55, 0x55, 0x55); // Grey 1
	colors[6]  = SDL_MapRGB(sdl_screen->format, 0x00, 0x00, 0xA0); // Medium blue
	colors[7]  = SDL_MapRGB(sdl_screen->format, 0x00, 0x00, 0xFF); // Light blue
	colors[8]  = SDL_MapRGB(sdl_screen->format, 0xFF, 0x90, 0x00); // Brown
	colors[9]  = SDL_MapRGB(sdl_screen->format, 0xFF, 0x90, 0x90); // Orange
	colors[10] = SDL_MapRGB(sdl_screen->format, 0xA0, 0xA0, 0xA0); // Grey 2
	colors[11] = SDL_MapRGB(sdl_screen->format, 0xFF, 0x00, 0xFF); // Pink
	colors[12] = SDL_MapRGB(sdl_screen->format, 0xFF, 0x00, 0xFF); // Light green
	colors[13] = SDL_MapRGB(sdl_screen->format, 0xFF, 0xFF, 0x00); // Yellow
	colors[14] = SDL_MapRGB(sdl_screen->format, 0xFF, 0x00, 0xFF); // Aquamarine
	colors[15] = SDL_MapRGB(sdl_screen->format, 0xFF, 0xFF, 0xFF); // White

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

                /*
		for (int y = 0; y < 24; y++) {
			for (int x = 0; x < 40; x++) {
				printf("%c", mainRegion->read(ptr + (y * 0x80) + x) & 0x7F);
				printf("%c", auxRegion->read(ptr + (y * 0x80) + x) & 0x7F);
			}

			printf("\n");
		}
                */

		/* 
		 *  The 40-col display is divided in interlaced in 3 parts: 0x400, 0x428, 0x450.
		 *  Each line is 0x80 bytes size (ie: line 0 is at 0x400, line 1 at 0x480, etc.)
		 */
		ptr = 0x400;
		for (int y = 0; y < 24; y++) {
			for (int x = 0; x < 80; x++) {
				if (y < 8)
					ptr = 0x400;
				else if (y >= 8 && y < 16)
					ptr = 0x428;
				else
					ptr = 0x450;

				uint16_t offset = ptr + ((y % 8) * CHARACTER_LINE_SIZE) + x;


				// printf("%c", c);
				
				uint8_t c;
				if (x % 2 == 0)
					c = mainRegion->read(offset);
				else
				        c = auxRegion->read(offset);

				drawCharacter(x * CHARACTER_WIDTH, y * CHARACTER_HEIGHT, c);
			}

			//printf("\n");
		}
	} else {
		/* 
		 *  The 40-col display is divided in interlaced in 3 parts: 0x400, 0x428, 0x450.
		 *  Each line is 0x80 bytes size (ie: line 0 is at 0x400, line 1 at 0x480, etc.)
		 */
		ptr = 0x400;
		for (int y = 0; y < 24; y++) {
			for (int x = 0; x < 40; x++) {
				if (y < 8)
					ptr = 0x400;
				else if (y >= 8 && y < 16)
					ptr = 0x428;
				else
					ptr = 0x450;

				uint16_t offset = ptr + ((y % 8) * CHARACTER_LINE_SIZE) + x;
				uint8_t c = mainRegion->read(offset);

				// printf("%c", c);
				drawCharacter(x * CHARACTER_WIDTH, y * CHARACTER_HEIGHT, c);
			}
			//printf("\n");
		}
	}

/*
	for (int x = 0; x < 4096 / 8; x++)
		drawCharacter(x * 8, ((x * 8) / 480) * 8, x);
*/

	//drawCharacter(0, 0, 0x61);
	SDL_UpdateRect(sdl_screen, 0, 0, 0, 0);
}

void
Screen::redrawGraphics(void)
{
	if (switches->isHires()) {
		redrawGraphicsHires();
	} else {
		redrawGraphicsLowres();
	}
}

void
Screen::redrawGraphicsHires(void)
{
}

void
Screen::drawBlock(int x, int y, int size_x, int size_y, unsigned char color)
{
	assert(color < 16);

	/* Lock the screen for direct access to the pixels */
	if ( SDL_MUSTLOCK(sdl_screen) ) {
		if ( SDL_LockSurface(sdl_screen) < 0 ) {
			fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
			return;
		}
	}

	/* Draw each scanline, one by one */
	for (int scanline = 0; scanline < size_y; scanline++) {
		for (uint8_t b = 0; b < size_x; b++) {
			Uint32 pixel = colors[color];
			putZoomPixel(x + b, y + scanline, pixel);
		}
	}

	/* Lock the screen for direct access to the pixels */
	if ( SDL_MUSTLOCK(sdl_screen) ) {
		SDL_UnlockSurface(sdl_screen);
	}
}

void
Screen::redrawGraphicsLowres(void)
{
	uint16_t ptr;

	// XXX: Handle mixed-res

	// XXX: Which page is the current one?
	ptr = 0x400;

	/* 
	 *  The 40-col display is divided in interlaced in 3 parts: 0x400, 0x428, 0x450.
	 *  Each line is 0x80 bytes size (ie: line 0 is at 0x400, line 1 at 0x480, etc.)
	 */
	for (int y = 0; y < 48; y++) {
		for (int x = 0; x < 40; x++) {
			if (y < 8)
				ptr = 0x400;
			else if (y >= 8 && y < 16)
				ptr = 0x428;
			else
				ptr = 0x450;
			
			uint16_t offset = ptr + ((y % 8) * CHARACTER_LINE_SIZE) + x;
			uint8_t c = mainRegion->read(offset);

			uint8_t color;

			if (y % 2 == 0)
				color = c & 0x0F; // Top block
			else
				color = (c & 0xF0) >> 4; // Bottom block
			
			drawBlock(x, y, CHARACTER_WIDTH, CHARACTER_HEIGHT / 2, color);
		}
	}
}

bool
Screen::loadFont(std::string filename)
{
	FILE *f = fopen(filename.c_str(), "r");
	if (! f) {
		perror("fopen()");
		return(false);
	}

	size_t len = fread(fontBuffer, 1, SCREEN_FONT_SIZE, f);
	if (len != SCREEN_FONT_SIZE) {
		fprintf(stderr, "Error reading font file %s: Expected %d bytes but read %lu\n", filename.c_str(), SCREEN_FONT_SIZE, len);
		return(false);
	}

	fclose(f);

	return(true);
}

void
Screen::drawCharacter(int x, int y, int charIndex)
{
	Uint32 white = SDL_MapRGB(sdl_screen->format, 0xff, 0xff, 0xff);
	// Uint32 white = SDL_MapRGB(sdl_screen->format, 15, 119, 30);
	Uint32 black = SDL_MapRGB(sdl_screen->format, 0x00, 0x00, 0x00);

	/* Lock the screen for direct access to the pixels */
	if ( SDL_MUSTLOCK(sdl_screen) ) {
		if ( SDL_LockSurface(sdl_screen) < 0 ) {
			fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
			return;
		}
	}

	uint16_t charsetIndex = 0;

	if (switches->isAltCharset())
		charsetIndex = 2048;

	/* Draw each scanline, one by one */
	for (int scanline = 0; scanline < CHARACTER_HEIGHT; scanline++) {
		for (uint8_t b = 0; b < CHARACTER_WIDTH; b++) {
			uint8_t c = 0x01 << b;
			uint8_t bit = fontBuffer[charIndex * 8 + scanline + charsetIndex] & c;

			Uint32 pixel = ((bit != 0) ? white : black);
			putZoomPixel(x + b, y + scanline, pixel);
		}
	}

	/* Lock the screen for direct access to the pixels */
	if ( SDL_MUSTLOCK(sdl_screen) ) {
		SDL_UnlockSurface(sdl_screen);
	}
}
