#include "MemoryScreen.h"

// This is retarded. Really, C++?
#ifndef NULL
#define NULL 0
#endif

#define TEXT_VIDEO_START 0x0400
#define TEXT_VIDEO_END   0x0BFF
// #define TEXT_VIDEO_SIZE ((TEXT_VIDEO_END - TEXT_VIDEO_START) + 1)

MemoryScreen::MemoryScreen()
	: MemoryRegion(TEXT_VIDEO_START, TEXT_VIDEO_END)
{
	auxData = new uint8_t[0x5FFF - 0x4000 + 1];
}

void
MemoryScreen::write(uint16_t offset, uint8_t val)
{
	// XXX: Output to the SDL screen here
}
