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
#include <SDL.h>

/*
 *  XXX: I think the best way to handle switch changes
 *  (text/gfx1/gfx2) would be a simple Observer.
 */

/*
 *  The Apple //e screen's resolution is 40*7 x 24*8 (280x192)
 *  Regular text mode is 40x24 (one character is 7x8)
 *  Low-res graphics are 40x48 (one "pixel" is 7x4)
 *  Hi-res graphics are 280x192 (one "pixel" is 1x1)
 */
Screen::Screen(unsigned int width, unsigned int height, MemoryRegion *mainRegion, 
	       MemoryRegion *auxRegion, MemorySoftSwitch *switches)
	: width(width),
	  height(height),
	  zoomFactor(1),
	  windowWidth(640),	// XXX: Make this configurable.
	  windowHeight(480),
	  mainRegion(mainRegion),
	  auxRegion(auxRegion),
	  switches(switches),
	  sdl_window(NULL),
	  sdl_renderer(NULL),
	  sdl_texture(NULL),
	  sdl_surface(NULL)
{
}

/*
 *  Change the pixel size, effectively 'zooming' it
 */
bool
Screen::setZoom(unsigned int zoomFactor)
{
	this->zoomFactor = zoomFactor;

	this->height = CHARACTER_HEIGHT * CHARACTER_ROWS * zoomFactor;
	this->width = CHARACTER_WIDTH * CHARACTER_COLS * zoomFactor;

	// SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	// SDL_RenderSetLogicalSize(sdl_renderer, logicalWidth, logicalHeight);

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
    SDL_Surface *surface = sdl_surface;

    assert(x <= width && y <= height);
    Uint32 *pixels = (Uint32 *) sdl_surface->pixels;

    pixels[(y * surface->w) + x] = pixel;
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

	// sdl_putpixel(x, y, color);
}

bool
Screen::init(void)
{
	if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr,	"Couldn't initialize SDL Video: %s\n", SDL_GetError());
		return(false);
	}

	atexit(SDL_Quit);

	sdl_window = SDL_CreateWindow("Apple //e Emulator",
	                              SDL_WINDOWPOS_CENTERED,
	                              SDL_WINDOWPOS_CENTERED,
	                              windowWidth, windowHeight, 0);
	if ( sdl_window == NULL ) {
		fprintf(stderr, "Couldn't create %dx%dx8 window: %s\n", windowWidth, windowHeight, SDL_GetError());
		return(false);
	}

	sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);
	if ( sdl_renderer == NULL ) {
		fprintf(stderr, "SDL_CreateRenderer() failed: %s", SDL_GetError());
		return(false);
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(sdl_renderer, width, height);

	sdl_texture = SDL_CreateTexture(sdl_renderer,
	                                SDL_PIXELFORMAT_ARGB8888,
	                                SDL_TEXTUREACCESS_STREAMING,
	                                width, height);
	if ( sdl_texture == NULL ) {
		fprintf(stderr, "SDL_CreateTexture() failed: %s", SDL_GetError());
		return(false);
	}

	/*
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
rmask = 0xff000000;
gmask = 0x00ff0000;
bmask = 0x0000ff00;
amask = 0x000000ff;
#else
rmask = 0x000000ff;
gmask = 0x0000ff00;
bmask = 0x00ff0000;
amask = 0xff000000;
#endif
	 */

	/* XXX: The height and width should be the one of the current
	 * resolution, so that SDL/GPU can scale on its own. This implies
	 * either having two surfaces (one for low and one for hires) or
	 * re-creating them on the fly.
	 */
	sdl_surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
	if ( sdl_surface == NULL ) {
		fprintf(stderr, "SDL_CreateRGBSurface() failed: %s", SDL_GetError());
		return(false);
	}

	fontBuffer = new uint8_t[SCREEN_FONT_SIZE];

	std::string videoROM = std::string("Apple IIe Video ROM US.bin");
	if (! loadFont(videoROM)) {
		fprintf(stderr, "Couldn't open video ROM %s\n", videoROM.c_str());
		return(false);
	}

	// XXX: A few of these colors are off..
	colors[0]  = SDL_MapRGB(sdl_surface->format, 0x00, 0x00, 0x00); // Black
	colors[1]  = SDL_MapRGB(sdl_surface->format, 0xFF, 0x00, 0xFF); // Magenta
	colors[2]  = SDL_MapRGB(sdl_surface->format, 0x00, 0x00, 0x55); // Dark blue
	colors[3]  = SDL_MapRGB(sdl_surface->format, 0xFF, 0x00, 0xFF); // Purple
	colors[4]  = SDL_MapRGB(sdl_surface->format, 0x00, 0x55, 0x00); // Dark green
	colors[5]  = SDL_MapRGB(sdl_surface->format, 0x55, 0x55, 0x55); // Grey 1
	colors[6]  = SDL_MapRGB(sdl_surface->format, 0x00, 0x00, 0xA0); // Medium blue
	colors[7]  = SDL_MapRGB(sdl_surface->format, 0x55, 0x55, 0xFF); // Light blue
	colors[8]  = SDL_MapRGB(sdl_surface->format, 0xFF, 0x90, 0x00); // Brown
	colors[9]  = SDL_MapRGB(sdl_surface->format, 0xFF, 0x55, 0x55); // Orange
	colors[10] = SDL_MapRGB(sdl_surface->format, 0xA0, 0xA0, 0xA0); // Grey 2
	colors[11] = SDL_MapRGB(sdl_surface->format, 0xFF, 0xA0, 0xFF); // Pink
	colors[12] = SDL_MapRGB(sdl_surface->format, 0x55, 0xFF, 0x55); // Light green
	colors[13] = SDL_MapRGB(sdl_surface->format, 0xFF, 0xFF, 0x00); // Yellow
	colors[14] = SDL_MapRGB(sdl_surface->format, 0xA0, 0xA0, 0xFF); // Aquamarine
	colors[15] = SDL_MapRGB(sdl_surface->format, 0xFF, 0xFF, 0xFF); // White

	return(true);
}

void
Screen::redraw(void)
{
        if (switches->isTextMode()) {
		redrawText();
	} else if (switches->isMixedMode()) {
		redrawGraphics();
		redrawText();		
	} else {
		redrawGraphics();
	}

	SDL_UpdateTexture(sdl_texture, NULL, sdl_surface->pixels, sdl_surface->pitch);
	SDL_RenderClear(sdl_renderer);
	SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
	SDL_RenderPresent(sdl_renderer);
}

void
Screen::redrawText()
{
	uint16_t ptr; // Base address of text memory

	if (switches->is80Col()) {
		ptr = 0x400;

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
				uint8_t c;

				if (x % 2 == 0)
					c = mainRegion->read(offset);
				else
				        c = auxRegion->read(offset);

				drawCharacter(x * CHARACTER_WIDTH, y * CHARACTER_HEIGHT, c);
			}
		}
	} else {
		/* 
		 *  The 40-col display is divided in interlaced in 3 parts: 0x400, 0x428, 0x450.
		 *  Each line is 0x80 bytes size (ie: line 0 is at 0x400, line 1 at 0x480, etc.)
		 */

		/* ptr points to the start of the low-res graphics memory */
		ptr = 0x0400;
		
		// But if page2 is enabled, then it start at 0x800
		if (switches->isPage2())
			ptr += 0x0400;

		int startPos = 0;

		if (!switches->isTextMode() && switches->isMixedMode())
			startPos = 20;

		uint16_t adj = 0x0000;

		for (int y = startPos; y < 24; y++) {
			for (int x = 0; x < 40; x++) {
				if (y < 8)
					adj = 0x0000;
				else if (y >= 8 && y < 16)
					adj = 0x0028;
				else
					adj = 0x0050;

				uint16_t offset = ptr + adj + ((y % 8) * CHARACTER_LINE_SIZE) + x;
				uint8_t c = mainRegion->read(offset);

				drawCharacter(x * CHARACTER_WIDTH, y * CHARACTER_HEIGHT, c);
			}
		}
	}
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

// Rather than make a giant if, this table represents the color of a
// pixel based on 4 factors:
// Bit 0: Even (1)/Odd column(0)
// Bit 1: Bit 7 On(1)/Off(0)
// Bit 2: Adjacent pixels On(1)/Off(0)
// Bit 3: Pixel is On(1)/Off(0)
Uint32 HIRES_COLOR_MATRIX[16] = 
{
	COLOR_BLACK,  // 0xxx: Pixel is off
	COLOR_BLACK,  // 0xxx: Pixel is off
	COLOR_BLACK,  // 0xxx: Pixel is off
	COLOR_BLACK,  // 0xxx: Pixel is off
	COLOR_BLACK,  // 0xxx: Pixel is off
	COLOR_BLACK,  // 0xxx: Pixel is off
	COLOR_BLACK,  // 0xxx: Pixel is off
	COLOR_BLACK,  // 0xxx: Pixel is off
	COLOR_GREEN,  // 1000: Pixel is on, odd column, bit 7 off, adjacent pixels off
	COLOR_PURPLE, // 1001: Even column, bit 7 off, adjacent pixels off
	COLOR_ORANGE, // 1010: Odd column, bit 7 on, adjacent pixels off
	COLOR_BLUE,   // 1011: Even column, bit 7 on, adjacent pixels off
	COLOR_WHITE,  // 1100: Odd column, bit 7 off, adjacent pixels on
	COLOR_WHITE,  // 1101: Even column, bit 7 off, adjacent pixels on
	COLOR_WHITE,  // 1110: Odd column, bit 7 on, adjacent pixels on
	COLOR_WHITE,  // 1111: Even column, bit 7 on, adjacent pixels on
};

/*
  Two passes:
  Store black & white pixels in a buffer
  Redraw screen, using that information  
*/

void
Screen::redrawGraphicsHires(void)
{
	uint16_t ptr = 0x2000; // Base address of hi-res mem
	uint16_t adj = 0x0000; // Index on top of base address
	uint16_t subrow_adj;   // Each row is made of 8 "subrows"

	int endLine = SCREEN_ROWS;

	// In mixed mode, the last 4 lines of the screen remain text
	if (switches->isMixedMode())
		endLine = SCREEN_ROWS - (CHARACTER_HEIGHT * 4);

	// When page 2 is enabled, the memory base is $4000 instead
	if (switches->isPage2())
 		ptr = 0x4000;

	unsigned char row_buf[280]; // Temporary buffer for the current row
	
	/* 
	 *  The hi-res display is interlaced in 3 parts: 0x2000, 0x2028, 0x2050.
	 *  Each row is 0x80 bytes size (ie: row 0 is at 0x2000, row 1 at 0x2080, etc.)
	 *  Then after 8 rows, it goes back to 2028. 8 more then back to 2050.
	 *  For each row, there are 8 lines of pixels is separated by 0x0400
	 */

	for (int y = 0; y < endLine; y++) {
		// There are 64 lines between each interlace			
		adj = 0x28 * (y / 64);

		// printf("%d / 64 = %d\n", y, y / 64);

		// There's a 0x400 gap between each subrow
		subrow_adj = (y % 8) * 0x0400;

		uint16_t offset = ptr + adj + subrow_adj + (((y/8) % 8) * 0x80);

		// printf("y = %d, adj = $%04X, offset $%04X\n", y, adj, offset);

		unsigned int buf_pos = 0;

		// First fill a buffer
		for (int x = 0; x < SCREEN_COLS / 7; x++) {
			uint8_t c = mainRegion->read(offset + x);

			for (int bit = 0; bit < 7; bit++) {
				// Extract the bit and keep bit 7
				unsigned char mask = (0x01 << bit) | 0x80;
				unsigned char pixel = c & mask;

				// assert(buf_pos < sizeof(row_buf));
				row_buf[buf_pos++] = pixel;
			}
		}

		// Using the previously filled buffer, put actual
		// pixels on the screen with the correct color
		for (int x = 0; x < SCREEN_COLS; x++) {
			unsigned char pixel = row_buf[x];
			unsigned char adjacent = 0x00;
			unsigned char idx = 0x00;
			
			// Get the pixel on the left of this one
			if (x > 0)
				adjacent |= row_buf[x - 1];

			// Get the pixel on the right of this one
			if (x < 279)
				adjacent |= row_buf[x + 1];
			
			// Strip bit 7
			adjacent &= ~0x80;

			if (adjacent)
				idx |= HIRES_ADJ_PIXELS_ON;

			bool even = (x % 2 == 0);

			if (even)
				idx |= HIRES_EVEN_COLUMN;

			if (pixel & 0x80)
				idx |= HIRES_BIT7_ON;

			if (pixel & ~0x80)
				idx |= HIRES_PIXEL_ON;

			uint32_t color = colors[HIRES_COLOR_MATRIX[idx]];

			putZoomPixel(x, y, color);
		}
	}
}

void
Screen::redrawGraphicsDoubleHires(void)
{
}

void
Screen::redrawGraphicsLowres(void)
{
	uint16_t ptr; // Base address of lowres memory

	/* ptr points to the start of the low-res graphics memory */
	ptr = 0x0400;

	// But if page2 is enabled, then it start at 0x800
	if (switches->isPage2())
		ptr += 0x0400;

	/* Because the screen is interlaced, this is the correction to
	 * apply to ptr. */
	uint16_t adj = 0x0000;

	int startLine = 0;
	int endLine = 24;

	if (switches->isMixedMode())
		endLine = 20;

	/* 
	 *  The 40-col display is divided in interlaced in 3 parts: 0x400, 0x428, 0x450.
	 *  Each line is 0x80 bytes size (ie: line 0 is at 0x400, line 1 at 0x480, etc.)
	 */
	for (int y = startLine; y < endLine; y++) {
		for (int x = 0; x < 40; x++) {
			if (y < 8)
				adj = 0x0000;
			else if (y >= 8 && y < 16)
				adj = 0x0028;
			else
				adj = 0x0050;
			
			// XXX: Why isn't adj used here??
			uint16_t offset = ptr + adj + ((y % 8) * CHARACTER_LINE_SIZE) + x;
			uint8_t c = mainRegion->read(offset);

			uint8_t colorBottomBlock;
			uint8_t colorTopBlock;

			colorTopBlock = c & 0x0F; // Top block
			colorBottomBlock = (c & 0xF0) >> 4; // Bottom block
			
			drawBlock(x * CHARACTER_WIDTH, y * CHARACTER_WIDTH, CHARACTER_WIDTH, CHARACTER_HEIGHT / 2, colorTopBlock);
			drawBlock(x * CHARACTER_WIDTH, y * CHARACTER_WIDTH + CHARACTER_HEIGHT/2, CHARACTER_WIDTH, CHARACTER_HEIGHT / 2, colorBottomBlock);
		}
	}
}

/* Draw a block of size_x by size_y at (x,y) */
void
Screen::drawBlock(int x, int y, int size_x, int size_y, unsigned char color)
{
	assert(color < 16);

	/* Lock the screen for direct access to the pixels */
	if ( SDL_MUSTLOCK(sdl_surface) ) {
		if ( SDL_LockSurface(sdl_surface) < 0 ) {
			fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
			return;
		}
	}

	// printf("Screen:drawBlock(%dx%d @ %d,%d  color = %d)\n", size_x, size_y, x, y, color); 

	/* Draw a block of size_x by size_y at (x,y) */
	for (int dy = 0; dy < size_y; dy++) {
		for (uint8_t dx = 0; dx < size_x; dx++) {
			Uint32 pixel = colors[color];
			putZoomPixel(x + dx, y + dy, pixel);
		}
	}

	/* Lock the screen for direct access to the pixels */
	if ( SDL_MUSTLOCK(sdl_surface) ) {
		SDL_UnlockSurface(sdl_surface);
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
	Uint32 white = colors[COLOR_WHITE];
	Uint32 black = colors[COLOR_BLACK];

	/* Lock the screen for direct access to the pixels */
	if ( SDL_MUSTLOCK(sdl_surface) ) {
		if ( SDL_LockSurface(sdl_surface) < 0 ) {
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
	if ( SDL_MUSTLOCK(sdl_surface) ) {
		SDL_UnlockSurface(sdl_surface);
	}
}
