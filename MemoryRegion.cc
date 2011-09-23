#include <assert.h>
#include <string.h>
#include "MemoryRegion.h"

MemoryRegion::MemoryRegion(uint16_t regionStart, uint16_t regionEnd)
	: regionStart(regionStart),
	  regionEnd(regionEnd),
	  size(regionEnd - regionStart + 1)
{
	this->data = new uint8_t[this->size];
	memset(this->data, 0, this->size);
}

MemoryRegion::~MemoryRegion(void)
{
	delete[] this->data;
}

void
MemoryRegion::setData(uint8_t data[])
{
	memcpy(this->data, data, this->size);
}

uint16_t MemoryRegion::getStart(void)
{
	return(this->regionStart);
}

uint16_t MemoryRegion::getEnd(void)
{
	return(this->regionEnd);
}

uint16_t MemoryRegion::getSize(void)
{
	return(this->size);
}

/* 
 * Translate from absolute offset (ie, offset in the entire memory) to
 * the offset for this memory region. ie, if the entire memory is 65k,
 * this region is mapped at 10k and this region's size is 2k, then the
 * real offset should be between [0 , 2k-1].
 */
uint16_t MemoryRegion::translateOffset(uint16_t offset)
{
	uint16_t realOffset;

	realOffset = offset - regionStart;

	return(realOffset);
}

/* Read a byte from this memory region */
uint8_t MemoryRegion::read(uint16_t offset)
{
	assert(offset >= regionStart && offset <= regionEnd);

	uint16_t regionOffset = translateOffset(offset);

	return(data[regionOffset]);
}

/* Write a byte to this memory region */
void MemoryRegion::write(uint16_t offset, uint8_t val)
{
	assert(offset >= regionStart && offset <= regionEnd);

	uint16_t regionOffset = translateOffset(offset);

	data[regionOffset] = val;
}
